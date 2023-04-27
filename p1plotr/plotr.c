/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

/*
  If you are curious about how "plotr" is actually implemented, you can read through this
  file, but it will not make a difference for completing your work on this project.  In
  violation of real-world coding practices, this file is way too long, due to various
  pedagogical constraints that you don't need to worry about.

  The details: the core functionality of the project itself (including your code) is
  nicely organized in libplt.a, but most everything else (for grading, timing,
  signal-handling, handling different floating rounding modes) has to go somewhere else.
  Creating a separate library to contain all that extra functionality would be cleaner,
  but it would needlessly complicate compiling, linking, and distributing the reference
  code. Instead, some of the functionality ended up in common.c, and the rest is here.
  The separation is not very principled, but all the state about grading and timing needs
  to be static and in separate translation units than the student's code (so that
  students can't alter anything about its execution). That state needs to be writable
  from wherever environment variables are read, however, which has ended up being here,
  close to main().
*/

// for using sigaction()
#define _XOPEN_SOURCE 500

#include <sys/ioctl.h>
#include <signal.h>   // for all signal handling
#include <unistd.h>   // for read(), write(), unlink()
#include <assert.h>   // for assert()
#include <fcntl.h>    // for open()
#include <sys/time.h> // for setitimer()
#include <stdint.h>   // for uint64_t et al.

#include "plt.h"

/* copied/modified from pltPrivate.h, which isn't #included here because the idea is
   that plotr uses only the "public" API of the plt library */
#define USED(x)      (void)(x)
#define MALLOC(N, T) (T *)(malloc((N) * sizeof(T)))

// RR = random rounding of floating point computations
#define RR_DOING 0

// special value for gradePrefix that says "like grading but not really"
#define NOOP "noop"

// convenience buffer size
#define TMP_STRLEN 300

static const char *argv0 = NULL;

/* C pre-processor tricks to simplify creating the list of all
   plotr sub-commands; "plotr foo" is implemented in plotr_foo.c */
#define CMDMAP(F)                                                                       \
  F(cmpld)                                                                              \
  F(env)                                                                                \
  F(todo)                                                                               \
  F(about)                                                                              \
  F(lerp)                                                                               \
  F(itow)                                                                               \
  F(wtoi)                                                                               \
  F(klist)                                                                              \
  F(keval)                                                                              \
  F(peval)                                                                              \
  F(psamp)                                                                              \
  F(delta)                                                                              \
  F(ceval)                                                                              \
  F(csamp)                                                                              \
  F(plot)                                                                               \
  F(fp)                                                                                 \
  F(foo)
#define DECLARE(C) extern unrrduCmd plt_##C##Cmd;
#define LIST(C)    &plt_##C##Cmd,
CMDMAP(DECLARE)
static const unrrduCmd *const cmdList[] = {CMDMAP(LIST) NULL};

typedef union {
    uint *uu;
    int *ss;
    void *vv;
} ptrunion;
static int
scanInt(const char *envName, const char *str, ptrunion xx, int sgned) {
    int ret = 0;
    if (!sgned && '-' == str[0]) {
        fprintf(stderr,
                "%s:%s env var %s \"%s\" starts with \"-\", "
                "but want unsigned\n",
                argv0, __func__, envName, str);
        ret = 1;
    } else if (1 != (sgned ? sscanf(str, "%d", xx.ss) : sscanf(str, "%u", xx.uu))) {
        fprintf(stderr, "%s:%s: couldn't parse env var %s \"%s\" as %sint\n", argv0,
                __func__, envName, str, sgned ? "" : "u");
        ret = 1;
    }
    return ret;
}

static int
getenvInt(void *val, int sgned, const char *envName, int need) {
    char *envStr = getenv(envName);
    ptrunion xx = {.vv = val};
    int ret;
    if (!need) {
        if (!envStr)
            return 0; /* env var envName not set, and that's ok;
                         we don't change value */
        // else is set, so we try to parse it
        ret = scanInt(envName, envStr, xx, sgned);
    } else {
        if (!envStr || !strlen(envStr)) {
            fprintf(stderr, "%s:%s: env var %s not set, or set to empty string\n", argv0,
                    __func__, envName);
            return 1;
        }
        ret = scanInt(envName, envStr, xx, sgned);
    }
    return ret;
}

static int
getenvReal(real *_val, const char *envName, int need) {
    char *envStr = getenv(envName);
    if (!need) {
        if (!envStr)
            return 0; /* env var envName not set, and that's ok;
                         we don't change value */
    } else {
        if (!envStr || !strlen(envStr)) {
            fprintf(stderr, "%s:%s: env var %s not set, or set to empty string\n", argv0,
                    __func__, envName);
            return 1;
        }
    }
    double val;
    if (1 != sscanf(envStr, "%lf", &val)) {
        fprintf(stderr, "%s:%s env var %s \"%s\" not parsed as double\n", argv0,
                __func__, envName, envStr);
        return 1;
    }
    *_val = (real)val;
    return 0;
}

static int
getenvStr(char *str, const char *envName, int need) {
    char *envStr = getenv(envName);
    if (!need) {
        if (!envStr)
            return 0; /* env var envName not set, and that's ok;
                         we don't change value */
    } else {
        if (!envStr || !strlen(envStr)) {
            fprintf(stderr, "%s:%s: env var %s not set, or set to empty string\n", argv0,
                    __func__, envName);
            return 1;
        }
    }
    if (strlen(envStr) > TMP_STRLEN) {
        fprintf(stderr,
                "%s:%s: environment variable %s set to string "
                "with length %u larger than can be put in strlen-%u buffer\n",
                argv0, __func__, envName, (uint)strlen(envStr), TMP_STRLEN);
        return 1;
    }
    airStrcpy(str, TMP_STRLEN + 1, envStr);
    return 0;
}

// if non-zero, timeout (in seconds) for test execution
static real Timeout = 0;
static char TimeoutStr[TMP_STRLEN + 1] = "";
real
pltTimeout(void) {
    return Timeout;
}
static void
timerSet(real duration) {
    // h/t http://www.informit.com/articles/article.aspx?p=23618&seqNum=14
    struct itimerval timer;
    uint secs = (uint)(duration > 0 ? duration : 0);
    timer.it_value.tv_sec = secs;
    timer.it_value.tv_usec = (uint)(1000000 * (duration - (real)secs));
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    /* both of these alternatives will sum execution time of all running
       threads, which is more confusing (for the purposes of capping
       execution time) than useful
    setitimer(ITIMER_VIRTUAL, &timer, NULL);
    setitimer(ITIMER_PROF, &timer, NULL);
    */
    return;
}
void
pltTimeoutStart(void) {
    timerSet(Timeout);
}
void
pltTimeoutStop(void) {
    timerSet(0);
}

#ifdef SCIVIS_GRADE
#  include "pltGrade.h"
// random value
static uint gradeRandom = 0;
// some numbering of which test we're running
static uint gradeTnum = 0;
/* start of all lines generated by testing (to distinguish from other printed
   output), either NOOP or built from gradeRandom and gradeTnum */
static char gradePrefix[TMP_STRLEN + 1] = "";
// marks the *start* of a single test
static char gradeStart[TMP_STRLEN + 1] = "";
// marks the continuation (for more timing) of a test
static char gradeContinue[TMP_STRLEN + 1] = "";
// marks the *end* of a single test
static char gradeEnd[TMP_STRLEN + 1] = "";
// threshold on acceptable difference between your and reference
static real gradeThresh = 0;
// units for thresholding
static char gradeThreshUnit[TMP_STRLEN + 1] = "";
/* points for full credit; signed because sometimes we want to use
   negative points, so keeping them signed througout */
static int gradePoints = 0;
/* if more than 1 granularity of points credit */
static int gradePointStep = 1;
// factor by which student's code is allowed to be slower
static real gradeTimeAllowance = (real)2.5; // POLICY
// --------------- all gradeImage variables are for grading images
// make image triptych (you-ref,you,ref) vertically rather than horizontally
static int gradeImageVertJoin = 1;
// for the DIFFERENCE part of diff/stu/ref triptych, magnitude to map to 0 and 255 in
// output
static real gradeImageQuantCap = 30;
// when differencing images, radius of square window in which to minimize dist
static int gradeImagePixelSlop = 0;
// (next two are used for normalizing inter-pixel distances, which are in
//  units of ulps for reals, else the (integer) pixel values themselves
// distances less than than this are treated as zero
static real gradeImagePixelEps = 0;
// distances exceeding this are treated as one
static real gradeImagePixelCap = (real)0.000001;
/* grade multi-channel images as vectors (not per-component);
   pixeleps and pixelcap are still used */
static int gradeImageVector = 0;
/* (next two are used for normalizing inter-image distances, which is just the
   sum of the normalized inter-pixel distances, divided by the # pixels) */
// distances less than than this are treated as zero
static real gradeImageFracEps = 0;
// distances exceeding this are treated as one
static real gradeImageFracCap = (real)0.000001;

char *
pltGradeStart(char dst[TMP_STRLEN + 1]) {
    airStrcpy(dst, TMP_STRLEN + 1, gradeStart);
    return dst;
}
char *
pltGradeContinue(char dst[TMP_STRLEN + 1]) {
    airStrcpy(dst, TMP_STRLEN + 1, gradeContinue);
    return dst;
}
char *
pltGradeEnd(char dst[TMP_STRLEN + 1]) {
    airStrcpy(dst, TMP_STRLEN + 1, gradeEnd);
    return dst;
}
real
pltGradeThresh(void) {
    return gradeThresh;
}
char *
pltGradeThreshUnit(char dst[TMP_STRLEN + 1]) {
    airStrcpy(dst, TMP_STRLEN + 1, gradeThreshUnit);
    return dst;
}
int
pltGradePoints(void) {
    return gradePoints;
}
int
pltGradePointStep(void) {
    return gradePointStep;
}
real
pltGradeTimeAllowance(void) {
    return gradeTimeAllowance;
}
int
pltGradeImageVertJoin(void) {
    return gradeImageVertJoin;
}
real
pltGradeImageQuantCap(void) {
    return gradeImageQuantCap;
}
int
pltGradeImagePixelSlop(void) {
    return gradeImagePixelSlop;
}
real
pltGradeImagePixelEps(void) {
    return gradeImagePixelEps;
}
real
pltGradeImagePixelCap(void) {
    return gradeImagePixelCap;
}
int
pltGradeImageVector(void) {
    return gradeImageVector;
}
real
pltGradeImageFracEps(void) {
    return gradeImageFracEps;
}
real
pltGradeImageFracCap(void) {
    return gradeImageFracCap;
}

static int
getenvGrade(void) {
    int sgned;
    int noop = 0;
    /* If you are debugging your code and want to be able to run one of the plotr
       commands used for grading (without all the set-up and post-processing
       machinery of the \"go\" script), you can set the env var SCIVIS_GRADE_NOOP
       to 1. This allows grade/plotr to call both your code and the reference code,
       and to print the results, but it doesn't compute any points. For commands
       generating images, the side-by-side comparison image is generated, but
       otherwise (for commands not generating images) your vs ref results are not
       numerically compared. */
    if (getenvInt(&noop, sgned = 1, "SCIVIS_GRADE_NOOP", 0)) {
        fprintf(stderr, "%s:%s: trouble with a SCIVIS_GRADE_NOOP env var\n", argv0,
                __func__);
        return 1;
    }
    if (noop) {
        // not really grading, just going through the motions
        strcpy(gradePrefix, NOOP);
    } else {
        // we're really doing grading
        static const char rfn[] = "/dev/urandom";
        int rfd = open(rfn, O_RDONLY);
        if (rfd < 0) {
            fprintf(stderr, "%s:%s: open(%s) failed: %s\n", argv0, __func__, rfn,
                    strerror(errno));
            return 1;
        }
        uint32_t rval;
        if (4 != read(rfd, &rval, 4)) {
            fprintf(stderr, "%s:%s couldn't get 4 bytes from %s", argv0, __func__, rfn);
            return 1;
        }
        if (close(rfd) < 0) {
            fprintf(stderr, "%s:%s: close(%s) failed: %s\n", argv0, __func__, rfn,
                    strerror(errno));
            return 1;
        }
        gradeRandom = (uint)rval;
        if (getenvInt(&gradeTnum, sgned = 0, "SCIVIS_GRADE_TNUM", 1)
            || getenvStr(gradeStart, "SCIVIS_GRADE_START", 1)
            || getenvStr(gradeContinue, "SCIVIS_GRADE_CONTINUE", 1)
            || getenvStr(gradeEnd, "SCIVIS_GRADE_END", 1)
            || getenvInt(&gradePoints, sgned = 1, "SCIVIS_GRADE_POINTS", 1)
            || getenvInt(&gradePointStep, sgned = 1, "SCIVIS_GRADE_POINT_STEP", 0)
            || getenvReal(&gradeThresh, "SCIVIS_GRADE_THRESH", 1)
            || getenvStr(gradeThreshUnit, "SCIVIS_GRADE_THRESH_UNIT", 0)
            || getenvReal(&gradeTimeAllowance, "SCIVIS_GRADE_TIME_ALLOWANCE", 0)
            || getenvInt(&gradeImageVertJoin, sgned = 0, "SCIVIS_GRADE_IMAGE_VERT_JOIN",
                         0)
            || getenvReal(&gradeImageQuantCap, "SCIVIS_GRADE_IMAGE_QUANT_CAP", 0)
            || getenvInt(&gradeImagePixelSlop, sgned = 0,
                         "SCIVIS_GRADE_IMAGE_PIXEL_SLOP", 0)
            || getenvReal(&gradeImagePixelEps, "SCIVIS_GRADE_IMAGE_PIXEL_EPS", 0)
            || getenvReal(&gradeImagePixelCap, "SCIVIS_GRADE_IMAGE_PIXEL_CAP", 0)
            || getenvInt(&gradeImageVector, sgned = 0, "SCIVIS_GRADE_IMAGE_VECTOR", 0)
            || getenvReal(&gradeImageFracEps, "SCIVIS_GRADE_IMAGE_FRAC_EPS", 0)
            || getenvReal(&gradeImageFracCap, "SCIVIS_GRADE_IMAGE_FRAC_CAP", 0)) {
            fprintf(stderr, "%s:%s: trouble with a SCIVIS_GRADE env var\n", argv0,
                    __func__);
            return 1;
        }
        sprintf(gradePrefix, "__%03u__%08x__", gradeTnum, gradeRandom);
    }
    return 0;
}
#endif // SCIVIS_GRADE

typedef struct {
    int num;
    const char name[TMP_STRLEN + 1];
    const char desc[TMP_STRLEN + 1];
} sigDesc_t;
// commented out ones not consistently available across platforms
static const sigDesc_t sigTable[]
  = {{SIGHUP, "SIGHUP", "terminal line hangup"},
     {SIGINT, "SIGINT", "interrupt program (e.g. via ^C)"},
     {SIGQUIT, "SIGQUIT", "quit program"},
     {SIGILL, "SIGILL", "illegal instruction"},
     {SIGTRAP, "SIGTRAP", "trace trap"},
     {SIGABRT, "SIGABRT", "abort() or assert()"},
     {SIGBUS, "SIGBUS", "bus error"},
     {SIGFPE, "SIGFPE", "floating-point or arithmetic exception"},
     {SIGKILL, "SIGKILL", "kill program"},
     {SIGUSR1, "SIGUSR1", "User defined signal 1"},
     {SIGSEGV, "SIGSEGV", "segmentation violation (segfault)"},
     {SIGUSR2, "SIGUSR2", "User defined signal 2"},
     {SIGPIPE, "SIGPIPE", "write on a pipe with no reader (broken pipe)"},
     {SIGALRM, "SIGALRM", "real-time timer expired"},
     {SIGTERM, "SIGTERM", "software termination signal"},
     // {SIGSTKFLT, "SIGSTKFLT", "Stack fault."}, // not on osx?
     {SIGCHLD, "SIGCHLD", "child status has changed"},
     {SIGCONT, "SIGCONT", "continue after stop"},
     {SIGSTOP, "SIGSTOP", "stop (cannot be caught or ignored)"},
     {SIGTSTP, "SIGTSTP", "stop signal generated from keyboard"},
     {SIGTTIN, "SIGTTIN", "background read attempted from control terminal"},
     {SIGTTOU, "SIGTTOU", "background write attempted to control terminal"},
     {SIGURG, "SIGURG", "urgent condition present on socket"},
     {SIGXCPU, "SIGXCPU", "cpu time limit exceeded"},  // c.f. setrlimit(2)
     {SIGXFSZ, "SIGXFSZ", "file size limit exceeded"}, // c.f. setrlimit(2)
     {SIGVTALRM, "SIGVTALRM", "virtual time alarm"},   // c.f. setitimer(2)
     {SIGPROF, "SIGPROF", "profiling timer alarm"},    // c.f. setitimer(2)
     // {SIGWINCH,  "SIGWINCH",  "Window size change"},
     // {SIGPOLL, "SIGPOLL", "Pollable event occurred"},
     // {SIGIO      "SIGIO",     "I/O is possible on a descriptor"},
     // {SIGPWR,    "SIGPWR",    "Power failure restart"},
     {SIGSYS, "SIGSYS", "bad system call"},
     // {SIGEMT,    "SIGEMT",    "emulate instruction executed"},
     // {SIGINFO,   "SIGINFO",   "status request from keyboard"},
     {0, "SIG????", "(unknown signal)"}}; // fake stand-in for signal

// note -1: this is length of vector of actual signals
static const uint sigTableLen = (uint)(sizeof(sigTable) / sizeof(sigTable[0])) - 1;

static const char *
sigName(int sig) {
    const char *ret = sigTable[sigTableLen].name; // initialize to last
    for (uint si = 0; si < sigTableLen; si++)
        if (sig == sigTable[si].num) ret = sigTable[si].name;
    return ret;
}
static const char *
sigDesc(int sig) {
    const char *ret = sigTable[sigTableLen].desc; // initialize to last
    for (uint si = 0; si < sigTableLen; si++)
        if (sig == sigTable[si].num) ret = sigTable[si].desc;
    return ret;
}
// for more on async-safe functions:
// http://man7.org/linux/man-pages/man7/signal-safety.7.html

// like sprintf(_buff, "%d", nn) but async-safe
static char *
itos(char *_buff, int nn) {
    char *buff = _buff;
    // write negative, and treat rest as positive
    if (nn < 0) {
        buff[0] = '-';
        nn *= -1;
        buff++;
    }
    uint numdig = 1, tmp = nn;
    while (tmp >= 10) {
        numdig += 1;
        tmp /= 10;
    }
    char *p = buff + numdig;
    *p-- = 0; // terminate string
    while (p >= buff) {
        *p-- = nn % 10 + '0';
        nn /= 10;
    } // write digits
    return _buff;
}

// for writing a bunch of strings in an async-safe way
static void
swrite(const char **sps, int fd) {
    const char **sp = sps;
    while (*sp) {
        ssize_t wret;
        do {
            wret = write(fd, *sp, strlen(*sp));
            // will re-try if interrupted
        } while (-1 == wret && errno == EINTR);
        /* (not currently testing for short writes; could use
            rio_writen from Bryant & O'Hallaron) */
        sp++;
    }
    return;
}

/* some aspects of code below based on (public domain) code at
   https://gist.github.com/jvranish/4441299 */

static void
sigHandle(int sig, siginfo_t *siginfo, void *context) {
    USED(siginfo); // can't use USEd here: not defined
    USED(context);
    char buff[TMP_STRLEN + 1];
    // leading \n may help with parsing by grade/sum during crashes
    swrite((const char *[]){"\n", argv0, ": got signal ", itos(buff, sig), " (",
                            sigName(sig), ": ", sigDesc(sig),
                            ") before finishing normally\n", NULL},
           STDERR_FILENO);
    if (SIGALRM == sig || SIGVTALRM == sig || SIGPROF == sig) {
        swrite((const char *[]){argv0, ": got timeout from setitimer(",
                                (SIGALRM == sig ? "ITIMER_REAL"
                                                : (SIGVTALRM == sig ? "ITIMER_VIRTUAL"
                                                                    : "ITIMER_PROF")),
                                ") after ", TimeoutStr, " seconds of execution\n", NULL},
               STDERR_FILENO);
    }
    // posix_print_stack_trace();
#ifdef SCIVIS_GRADE
    int points = 0;
    if (SIGILL == sig || SIGSEGV == sig || SIGBUS == sig) { // POLICY
        points = -gradePoints / 2;                          // POLICY
    }
    // leading \n may help with parsing by grade/sum
    swrite((const char *[]){"\n", gradePrefix, " ", gradeEnd, " ", itos(buff, points),
                            " (from signal ", sigName(sig), ")\n", NULL},
           STDOUT_FILENO);
#endif // SCIVIS_GRADE
    _Exit(1);
}

/* whether to do signal handling, by default
   (prior to looking at SCIVIS_HANDLE_SIGNALS) */
static int handleSignals =
#ifdef SCIVIS_GRADE
  1 // yes when grading
#else
  0 // not when not grading
#endif
  ;
int
pltHandleSignals(void) {
    return handleSignals;
}

static void
setupSigHandle(void) {
    int envval = -42;
    static const char *envname = "SCIVIS_HANDLE_SIGNALS";
    if (getenvInt(&envval, 0 /* not signed */, envname, 0)) {
        fprintf(stderr, "%s:%s: couldn't parse env var \"%s\"\n", argv0, __func__,
                envname);
        exit(1);
    }
    if (-42 != envval) { // that envvar really was set
        handleSignals = !!envval;
    }

    static const char *tnvname = "SCIVIS_TIMEOUT";
    if (getenvReal(&Timeout, tnvname, 0)) {
        fprintf(stderr, "%s:%s: couldn't parse env var \"%s\"\n", argv0, __func__,
                tnvname);
        exit(1);
    }
    handleSignals |= Timeout > 0;
    if (Timeout > 0) {
        // save string, to avoid sprintf'ing it again in signal handler
        strcpy(TimeoutStr, getenv(tnvname));
    }

    if (handleSignals) {
        struct sigaction sig_action = {.sa_sigaction = sigHandle};
        sigemptyset(&sig_action.sa_mask);
        int err = 0;
        for (uint si = 0; si < sigTableLen; si++) {
            /* this includes SIGALRM (via setitimer(ITIMER_REAL)),
               SIGVTALRM (via setitimer (ITIMER_VIRTUAL)), and
               SIGPROF (via setitimer(ITIMER_PROF)) */
            int sig = sigTable[si].num;
            if (SIGKILL == sig || SIGSTOP == sig) {
                // can't handle this
                continue;
            }
            if (SIGUSR2 == sig) {
                /* could handle this, but it is used internally by valgrind
                   (which won't allow us to set a different handler) */
                continue;
            }
            err |= sigaction(sig, &sig_action, NULL);
            if (err) {
                fprintf(stderr, "%s:%s: sigaction(%d=%s) failed\n", argv0, __func__,
                        sigTable[si].num, sigTable[si].name);
                exit(1);
            }
        }
    }
    return;
}

#ifdef SCIVIS_GRADE

static real // POLICY
timeAdjustCredit(real crct, char buff[TMP_STRLEN + 1], long stutime, long reftime) {
    real ret = crct;
    if (stutime && reftime) {
        // Factor by which student's code is Slower
        real fs = (real)stutime / reftime;
        // Allowable Factor slower
        real af = gradeTimeAllowance;
        real scl = 1;
        if (fs < 1) {
            /* extra credit (up to 3x) for being faster than reference but
               only if correctness is (effectively) perfect: there's no value
               in quickly doing badly */
            if (crct == 1) {
                real sfs = fs / 0.95; // not just a fluke
                if (sfs > 1) sfs = 1;
                scl = 1 + 2 * (1 - sfs);
            }
        } else if (fs > af) {
            // credit diminishes as fs/af increases (above 1)
            scl *= af / fs;
        }
        // else 1 <= fs <= af: no adjustment
        ret = crct * scl;
        if (crct) {
            pltSprintf(buff,
                       "%s time(you,ref) = %ld,%ld ==> "
                       "speed factor %g vs %g ==> "
                       "scaling correctness %g by %g ==> %g",
                       (1 != crct
                          ? ";"
                          : (scl > 1 ? ", AND (nice!)" : (scl < 1 ? ", HOWEVER" : ";"))),
                       stutime, reftime, (real)stutime / reftime, gradeTimeAllowance,
                       crct, scl, ret);
        } else {
            sprintf(buff, " (time(you,ref) = %ld,%ld moot)", stutime, reftime);
        }
    } else {
        // no timing, so credit is correctness
        ret = crct;
        strcpy(buff, "");
    }
    return ret;
}

static real                       // from distance to credit
creditDist(real dist, real scl) { // POLICY
    if (!gradeThresh) {
        // the grading script failed to set a threshold
        pltPrintf("%s: gradeThresh=0; can't work; bye\n", __func__);
        exit(1);
    }
    // normalized distance: 1 at thresh, 2 at 2*thresh, etc
    real ndist = dist / (scl * gradeThresh);
    // somewhere between 1 and 0
    real credit = (ndist < 1 ? 1 : (ndist <= 8 ? 1 / ndist : 0));
    return credit;
}

static int                  // from credit to points
pointsCredit(real credit) { // POLICY
    int max = gradePoints;
    int step = gradePointStep;
    // rounding to granularity of step
    int ret = step * ((int)floor((real)1 / 2 + max * credit / step));
    if (credit <= 1) {
        /* make sure we didn't exceed max; extra credit
           should not be so clamped */
        ret = (ret > max ? max : ret);
    }
    return ret;
}

static const char descFail[] = "fail";
static const char descPartial[] = "partial";
static const char descPass[] = "pass";
static const char descExtra[] = "extra";
static const char *
describe(int got, int max) {
    const char *ret;
    if (!got) {
        ret = descFail;
    } else if (got < max) {
        ret = descPartial;
    } else if (got == max) {
        ret = descPass;
    } else {
        ret = descExtra;
    }
    return ret;
}

/* gradeMulti is for when there needs to be a sequence of grading
   calls to evaluate a result (e.g. pltGradeReal and then
   pltGradeVector), which is different than grading a big list
   of scalars with pltGradeTuple.  */
static int gradeMulti = 0;
/* if this non-zero, we take the MEAN of the multiple parts,
   else we take the MIN */
static int gradeMultiMean = 1;
static uint gradeMultiNum = 0;
#  define GRADE_MULTI_MAXNUM 16
static int gradeMultiPoints[GRADE_MULTI_MAXNUM];
#  define GRADE_MULTI_ADD                                                               \
    if (gradeMulti) {                                                                   \
      gradeMultiPoints[gradeMultiNum++] = points;                                       \
      if (GRADE_MULTI_MAXNUM == gradeMultiNum) {                                        \
        fprintf(stderr, "%s:%s: gradeMultiNum==%u max! bye.\n", argv0, __func__,        \
                gradeMultiNum);                                                         \
        exit(1);                                                                        \
      }                                                                                 \
    }

// if there are any differences among the multiple points
static int
gradeMultiUnequal(void) {
    for (uint ii = 0; ii < gradeMultiNum; ii++) {
        for (uint jj = ii + 1; jj < gradeMultiNum; jj++) {
            if (gradeMultiPoints[ii] != gradeMultiPoints[jj]) {
                return 1;
            }
        }
    }
    return 0;
}

void
pltGradeMultiStart(int mean) {
    gradeMulti = 1;
    gradeMultiMean = !!mean;
}

void
pltGradeMultiFinish(void) {
    int pp;
    if (gradeMultiMean) {
        pp = 0;
        for (uint ii = 0; ii < gradeMultiNum; ii++) {
            pp += gradeMultiPoints[ii];
        }
        pp /= gradeMultiNum;
    } else {
        pp = gradeMultiNum ? gradeMultiPoints[0] : 0;
        for (uint ii = 0; ii < gradeMultiNum; ii++) {
            int gg = gradeMultiPoints[ii];
            pp = gg < pp ? gg : pp;
        }
    }
    pltPrintf("%s %s %d (%s of points above)\n", gradePrefix, gradeEnd, pp,
              gradeMultiMean ? "average" : "min");
    gradeMultiNum = 0;
    gradeMulti = 0;
}

void
pltGradeReal(real yy, real ryy, const char *desc) {
    if (!strcmp(gradePrefix, NOOP)) return;
    uint dist = pltFloatDist(yy, ryy);
    int points = pointsCredit(creditDist(dist, 1));
    const char *expl = describe(points, gradePoints);
    int units = !!strlen(gradeThreshUnit);
    if (!dist) {
        pltPrintf("%s %s%s: your %s (%g) exactly same as reference\n", gradePrefix,
                  gradeMulti ? ". . " : "", expl, desc ? desc : "result", yy);
    } else {
        pltPrintf("%s %s%s: your (%g) and reference (%g) %s differ by "
                  "%u %s %g%s%s%s\n",
                  gradePrefix, gradeMulti ? ". . " : "", expl, yy, ryy,
                  desc ? desc : "result", dist, dist <= gradeThresh ? "<=" : ">",
                  gradeThresh, units ? " " : "", units ? gradeThreshUnit : "",
                  units ? "s" : "");
    }
    pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
              expl);
    GRADE_MULTI_ADD;
    return;
}

static inline uint
distVector(const real *svv, const real *rvv, uint len) {
    double rvlen = 0, dvlen = 0;
    for (uint ii = 0; ii < len; ii++) {
        double rr = rvv[ii], dd = svv[ii] - rvv[ii];
        rvlen += rr * rr;
        dvlen += dd * dd;
    }
    rvlen = sqrt(rvlen); // length of reference vector
    dvlen = sqrt(dvlen); // length of stu-ref vector
    /* the distance between stu and ref vectors is computed as the (scalar)
       distance between |ref| and |ref|+|stu-ref|.  So, even if ref and stu
       have individual components that straddle 0, the graded distance is
       numerically meaningful, AS LONG AS the length |ref| is far from 0 */
    return pltFloatDist(rvlen, rvlen + dvlen);
}

/* For grading a vector of reals, in a way that is more
   than just grading a list of individual coefficients */
void
pltGradeVector(const real *svv, const real *rvv, uint len, const char *desc) {
    if (!strcmp(gradePrefix, NOOP)) return;
    uint dist = distVector(svv, rvv, len);
    int points = pointsCredit(creditDist(dist, 1));
    const char *expl = describe(points, gradePoints);
    int units = !!strlen(gradeThreshUnit);
    char svstr[TMP_STRLEN + 1] = "", rvstr[TMP_STRLEN + 1] = "";
    for (uint ii = 0; ii < len; ii++) {
        char tmp[64];
        if (ii) {
            strcat(svstr, ",");
            strcat(rvstr, ",");
        }
        pltSprintf(tmp, "%g", svv[ii]);
        strcat(svstr, tmp);
        pltSprintf(tmp, "%g", rvv[ii]);
        strcat(rvstr, tmp);
    }
    if (!dist) {
        pltPrintf("%s %s%s: "
                  "your %s (%s) exactly same as reference\n",
                  gradePrefix, gradeMulti ? ". . " : "", expl, desc ? desc : "result",
                  svstr);
    } else {
        pltPrintf("%s %s%s: "
                  "with ref=(%s) and your=(%s) %s,"
                  "|ref| and |ref|+|your-ref| differ by "
                  "%u %s %g"
                  "%s%s%s\n",
                  gradePrefix, gradeMulti ? ". . " : "", expl, rvstr, svstr,
                  desc ? desc : "result", dist, dist <= gradeThresh ? "<=" : ">",
                  gradeThresh, units ? " " : "", units ? gradeThreshUnit : "",
                  units ? "s" : "");
    }
    pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
              expl);
    GRADE_MULTI_ADD;
    return;
}

void
pltGradeInt(int yy, int ryy, const char *desc) {
    if (!strcmp(gradePrefix, NOOP)) return;
    int points = (yy == ryy ? gradePoints : 0);
    const char *expl = describe(points, gradePoints);
    if (yy == ryy) {
        pltPrintf("%s %s%s: your %s (%d) same as reference\n", gradePrefix,
                  gradeMulti ? ". . " : "", expl, desc ? desc : "result", yy);
    } else {
        pltPrintf("%s %s%s: your (%d) and reference (%d) %s differ\n", gradePrefix,
                  gradeMulti ? ". . " : "", expl, yy, ryy, desc ? desc : "result");
    }
    pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
              expl);
    GRADE_MULTI_ADD;
    return;
}

// HEY copy-paste from pltGradeInt
void
pltGradeEnum(int yy, int ryy, const airEnum *enm, const char *desc) {
    if (!strcmp(gradePrefix, NOOP)) return;
    int points = (yy == ryy ? gradePoints : 0);
    const char *expl = describe(points, gradePoints);
    if (yy == ryy) {
        pltPrintf("%s %s%s: your %s (%s) same as reference\n", gradePrefix,
                  gradeMulti ? ". . " : "", expl, desc ? desc : "result",
                  airEnumStr(enm, yy));
    } else {
        pltPrintf("%s %s%s: your (%s) and reference (%s) %s differ\n", gradePrefix,
                  gradeMulti ? ". . " : "", expl, airEnumStr(enm, yy),
                  airEnumStr(enm, ryy), desc ? desc : "result");
    }
    pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
              expl);
    GRADE_MULTI_ADD;
    return;
}

void
pltGradeItemSet(pltGradeItem *itm, const char *desc, real stu, real ref, real thrscl) {
    airStrcpy(itm->desc, TMP_STRLEN + 1, desc);
    itm->stu = stu;
    itm->ref = ref;
    itm->thrscl = thrscl;
    return;
}

pltGradeItem *
pltGradeItemZip(real *stu, real *ref, uint len) {
    pltGradeItem *itm;
    itm = MALLOC(len, pltGradeItem);
    assert(itm);
    for (uint ii = 0; ii < len; ii++) {
        sprintf(itm[ii].desc, "v[%u]", ii);
        itm[ii].stu = stu[ii];
        itm[ii].ref = ref[ii];
        itm[ii].thrscl = 1;
    }
    return itm;
}

void
pltGradeTuple(const pltGradeItem *itm, uint itmLen, long stutime, long reftime) {
    if (!strcmp(gradePrefix, NOOP)) return;
    uint mpi = (uint)(-1); // index of item with minimum points
    uint dist = 0;
    uint alen = (itmLen + 1 > 6 ? 6 : itmLen + 1);
    char *desc = MALLOC(alen * (2 + (TMP_STRLEN + 1)), char);
    assert(desc);
    strcpy(desc, "");
    for (uint ii = 0; ii < itmLen; ii++) {
        uint dd = pltFloatDist(itm[ii].stu, itm[ii].ref);
        // POLICY : looking at *biggest* difference across tuple
        if (!ii || dd > dist) {
            dist = dd;
            mpi = ii;
        }
        if (ii < 5) {
            if (ii) strcat(desc, ", ");
            strcat(desc, itm[ii].desc);
        } else if (5 == ii) {
            strcat(desc, ", ...");
        }
    }
    real credit = creditDist(dist, itm[mpi].thrscl);
    char texpl[TMP_STRLEN + 1];
    credit = timeAdjustCredit(credit, texpl, stutime, reftime);
    int points = pointsCredit(credit);
    int units = !!strlen(gradeThreshUnit);
    char expl[TMP_STRLEN];
    strcpy(expl, describe(points, gradePoints));
    if (points >= gradePoints) {
        char ldsc[TMP_STRLEN + 1];
        sprintf(ldsc, "all %u", itmLen);
        pltPrintf("%s %s%s: %s %s within %g%s%s%s of reference%s\n", gradePrefix,
                  gradeMulti ? ". . " : "", expl, desc, 2 == itmLen ? "both" : ldsc,
                  gradeThresh, units ? " " : "", units ? gradeThreshUnit : "",
                  units ? "s" : "", texpl);
    } else {
        pltPrintf("%s %s%s: your (%g) and reference (%g) %s "
                  "differ by %u %s %g%s%s%s%s\n",
                  gradePrefix, gradeMulti ? ". . " : "", expl, itm[mpi].stu,
                  itm[mpi].ref, itm[mpi].desc, dist,
                  dist > gradeThresh ? ">" : "<=", gradeThresh, units ? " " : "",
                  units ? gradeThreshUnit : "", units ? "s" : "", texpl);
    }
    pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
              expl);
    GRADE_MULTI_ADD;
    free(desc);
    return;
}

// https://www.benjaminwuethrich.dev/2015-03-07-sorting-strings-in-c-with-qsort.html
static int
cmpstr(void const *a, void const *b) {
    char const *aa = *(char const **)a;
    char const *bb = *(char const **)b;
    return strcmp(aa, bb);
}

static char *
lineProc(const char *str, uint headLines, int sortDo) {
    if (!str) {
        // return NULL if we got NULL
        return NULL;
    }
    uint slen = (uint)strlen(str);
    /* copy input str to output ret; allocating one
       extra character to add final \n if needed */
    char *ret = MALLOC(slen + 1, char);
    assert(ret);
    strcpy(ret, str);
    uint lnum = 0;
    for (uint ii = 0; ii < slen; ii++) {
        // count number of lines (newlines)
        if ('\n' == str[ii]) lnum++;
    }
    if (!lnum) {
        // um, actually no lines? return copy of input
        return ret;
    }
    if (1 == lnum && '\n' == str[slen - 1]) {
        // input is a single line; return a copy of it
        return ret;
    }
    // else input is (or wants to be) more than one line
    // buf: copy of input, with \n turned into \0
    // ret: re-ordering of lines in buf
    char *buf = MALLOC(slen + 1, char);
    assert(buf);
    strcpy(buf, str);
    if ('\n' != str[slen - 1]) {
        // input str didn't end with '\n', but we pretend it did
        slen++; // slen now length of extended string
        buf[slen - 1] = '\n';
        buf[slen] = '\0';
        lnum++;
    }
    // lin: points into buf at line starts
    char **lin = MALLOC(lnum, char *);
    assert(lin);
    uint lidx = 0;
    char *ptr = buf; // start of current line
    for (uint ii = 0; ii < slen; ii++) {
        if ('\n' == buf[ii]) {
            lin[lidx++] = ptr;
            buf[ii] = '\0';
            ptr = buf + ii + 1;
        }
    }
    // effective number of lines, given headLines
    uint fnum = (headLines ? AIR_MIN(headLines, lnum) : lnum);
    if (sortDo) { // sort the lines
        qsort(lin, fnum, sizeof(char *), cmpstr);
    }
    // copy (possibly sorted) lines into output
    ptr = ret;
    for (uint ii = 0; ii < fnum; ii++) {
        uint ll = strlen(lin[ii]);
        memcpy(ptr, lin[ii], ll);
        ptr[ll] = '\n';
        ptr += ll + 1;
    }
    if (headLines) {
        // terminate the now-shorter output string
        *ptr = '\0';
    }
    free(buf);
    free(lin);
    return ret;
}

// HEY copy-paste from pltGradeInt
// assumes that rstr is non-NULL
void
pltGradeString(const char *_sstr, const char *_rstr, uint headLines, int sortLines,
               const char *desc) {
    const char *sstr, *rstr;
    char *sstrProc = NULL, *rstrProc = NULL;
    if (!strcmp(gradePrefix, NOOP)) return;
    if (headLines || sortLines) {
        sstr = sstrProc = lineProc(_sstr, headLines, sortLines);
        rstr = rstrProc = lineProc(_rstr, headLines, sortLines);
    } else {
        sstr = _sstr;
        rstr = _rstr;
    }
    int refnull = !rstr;
    int stunull = !sstr;
    int same
      = (refnull
           ? refnull == stunull
           : (stunull
                ? 0 // else didn't want null, but got null
                    // unfortunately there is no partial credit on the strings (yet)
                : !strcmp(sstr, rstr)));
    int points = (same ? gradePoints : 0);
    const char *expl = describe(points, gradePoints);
    if (!same) {
        if (refnull) {
            pltPrintf("%s %s%s: your %s string was not empty when it should have been\n",
                      gradePrefix, gradeMulti ? ". . " : "", expl,
                      desc ? desc : "result");
        } else /* !refnull */
            if (stunull) {
                pltPrintf("%s %s%s: your %s string was empty\n", gradePrefix,
                          gradeMulti ? ". . " : "", expl, desc ? desc : "result");
            } else {
                char tbuf[256];
                if (headLines) {
                    sprintf(tbuf, "first %u of ", headLines);
                } else {
                    strcpy(tbuf, "");
                }
                pltPrintf("%s %s%s: %syour and reference %s strings%s differ\n",
                          gradePrefix, gradeMulti ? ". . " : "", expl, tbuf,
                          desc ? desc : "result", sortLines ? " (lines sorted)" : "");
            }
    }
    pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
              expl);
    GRADE_MULTI_ADD;
    if (headLines || sortLines) {
        if (sstrProc) free(sstrProc);
        free(rstrProc);
    }
    return;
}

/*
  How to measure distance between two image pixels.

  The "raw" distance (computed directly from channel differences, or
  difference as graded by vector) is saved in *rawdist if rawdist, and then
  the effective distance (taking into account gradeImagePixelEps and
  gradeImagePixelCap) is returned. If exact, *exact is updated as a record
  of whether all measured raw distances have been zero.
 */
static inline real
pixdist(int *exact, real *rawdist, const real *spix, const real *rpix, uint chan,
        int isUChar) {
    real pixEps = gradeImagePixelEps;
    real pixCap = gradeImagePixelCap;
    real dist = 0;
    int isvec = gradeImageVector;
    /* (HACK!) project 3 only (pltBiffKey[0] == "plt"[0] == 'r'): special
       treatment of 4-channel real pixels, which we assume(!) to be RGBA values.
       Since colors are meaningless with zero opacity, what really matters
       are the colors with premultiplied alpha, and, values very near zero can't
       be meaningfully graded, so we add 1.0 */
    if ('r' == pltBiffKey[0] && 4 == chan && !isUChar && !isvec) {
        for (uint cc = 0; cc < 3; cc++) {
            real dd = pltFloatDist(1 + spix[cc] * spix[3], 1 + rpix[cc] * rpix[3]);
            dist = dd > dist ? dd : dist; // POLICY: max dist over channels
        }
    } else {
        // regular (non-p3-specific) differencing */
        if (isvec && chan > 1) {
            dist = distVector(spix, rpix, chan);
        } else {
            for (uint cc = 0; cc < chan; cc++) {
                real dd;
                if (isUChar) {
                    dd = fabs(spix[cc] - rpix[cc]);
                } else {
                    dd = pltFloatDist(spix[cc], rpix[cc]);
                }
                dist = dd > dist ? dd : dist; // POLICY: max dist over channels
            }
        }
    }
    if (rawdist) *rawdist = dist;
    if (exact) {
        *exact = (*exact && (dist == 0));
    }
    if (pixCap > pixEps) {
        dist = (dist - pixEps) / (pixCap - pixEps);
        dist = dist < 0 ? 0 : (dist > 1 ? 1 : dist);
    } else {
        dist = dist ? (real)1 : (real)0;
    }
    return dist;
}

// for doing per-channel quantization of stu and ref images
static inline unsigned char
bit8q(real min, real val, real max) {
    int di;
    if (isfinite(val)) {
        di = 256 * (val - min) / (max - min);
        di = di < 0 ? 0 : (di > 255 ? 255 : di);
    } else {
        /* not finite, but don't here get to create different colors for the
           different kinds of non-finite values (since this is called from
           a per-channel computation), but maybe at least be explicit */
        if (val < 0) {        // negative infinity
            di = 255;         // pretend to wrap-around
        } else if (val > 0) { // positive infinity
            di = 0;
        } else {
            di = 154; // some random value
        }
    }
    return (uchar)di;
}

/*
  what determines the output 8-bit pixel value for the
  difference part of the triptych (stu - ref)
  if isUChar: use gradeImageQuantCap for per-channel diffs
      (spix and rpix are reals but holding uchars)
  else: use dist (normalized already) to determine quantization
*/
static inline void
dpixout(uchar *out, const real *spix, const real *rpix, uint chan, int isUChar,
        real dist, uint ii, uint jj) {
    real qcap = gradeImageQuantCap;
    if (isUChar) {
        for (uint cc = 0; cc < chan; cc++) {
            real dd = spix[cc] - rpix[cc];
            int di = 256 * (dd + qcap) / (2 * qcap);
            di = di < 0 ? 0 : (di > 255 ? 255 : di);
            out[cc] = (uchar)di;
        }
    } else {
        real maxd = 0;
        for (uint cc = 0; cc < chan; cc++) {
            real dd = fabs(spix[cc] - rpix[cc]);
            if (dd > maxd) maxd = dd;
        }
        if (!maxd) maxd = 1;
        for (uint cc = 0; cc < chan; cc++) {
            real dd = dist * (spix[cc] - rpix[cc]) / maxd;
            /* for non-finite values, draw diagonal stripes
               (otherwise end up getting constant gray) */
            out[cc] = (isfinite(spix[cc]) ? bit8q(-1, dd, 1)
                                          : 255 * (((ii + jj) / 5) % 2));
        }
    }
    return;
}

void
pltGradeImage(const char *_oname, const char *content, const void *_stu,
              const void *_ref, int isUChar, uint chan, uint isz0, uint isz1,
              long stutime, long reftime) {
    uint vertJoin = gradeImageVertJoin;
    int slop = gradeImagePixelSlop;
    real fracEps = gradeImageFracEps;
    real fracCap = gradeImageFracCap;
    size_t osz[3];
    /* pointers to stu or ref image data;
       depending on isUChar; only one pair of these is useful */
    const uchar *stuUC = _stu;
    const uchar *refUC = _ref;
    const real *stuRL = _stu;
    const real *refRL = _ref;
    uint osz0;
    if (chan > 1) {
        osz[0] = chan;
        osz[1] = (vertJoin ? 1 : 3) * isz0;
        osz0 = (uint)osz[1];
        osz[2] = (vertJoin ? 3 : 1) * isz1;
    } else {
        osz[0] = (vertJoin ? 1 : 3) * isz0;
        osz0 = (uint)osz[0];
        osz[1] = (vertJoin ? 3 : 1) * isz1;
    }
    airArray *mop = airMopNew();
    Nrrd *nout = nrrdNew();
    airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
    // "out" is always uchar image, regardless of isUChar
    if (nrrdMaybeAlloc_nva(nout, nrrdTypeUChar, 2 + (chan > 1), osz)) {
        char *err = biffGetDone(NRRD);
        fprintf(stderr, "%s: ERROR: couldn't allocate output image:\n%s", __func__, err);
        airMopError(mop);
        exit(1);
    }
    nout->content = airStrdup(content);
    /* in case of real-valued images, student and reference parts of triptych
       are 8-bit quantizations of stuRL and refRL; so have to determine the
       range [iqmin, iqmax] of that quantization; it is NOT based on
       gradeImageQuantCap (that is only for difference image) */
    real iqmin, iqmax;
    if (isUChar) {
        iqmin = iqmax = 0; // not needed
    } else {
        iqmin = iqmax = pltNan(0);
        uint inp = chan * isz0 * isz1;
        for (uint ii = 0; ii < inp; ii++) {
            real vv = refRL[ii];
            if (isfinite(vv)) {
                if (!isfinite(iqmin) || vv < iqmin) iqmin = vv;
                if (!isfinite(iqmax) || vv > iqmax) iqmax = vv;
            }
        }
        // in case got no finite values, or all equal values
        if (!isfinite(iqmin)) iqmin = 0;
        if (!isfinite(iqmax)) iqmax = 1;
        if (iqmin == iqmax) iqmax += 1;
    }
    unsigned char *out = (unsigned char *)(nout->data);
    real dist = 0, // distance with slop
      dist0 = 0;   // distance with zero slop
    int exact = 1;
    uint worstP[2] = {(uint)-1, (uint)-1};
    real worstD = 0;
    for (uint i1 = 0; i1 < isz1; i1++) { // the slower image index
        uint o1D, o1S, o1R;              // output slower index for Diff, Stu, Ref
        if (vertJoin) {
            o1D = i1;
            o1S = isz1 + i1;
            o1R = 2 * isz1 + i1;
        } else {
            o1D = o1S = o1R = i1;
        }
        for (uint i0 = 0; i0 < isz0; i0++) { // the faster image index
            uint o0D, o0S, o0R;              // output faster index for Diff, Stu, Ref
            if (vertJoin) {
                o0D = o0S = o0R = i0;
            } else {
                o0D = i0;
                o0S = isz0 + i0;
                o0R = 2 * isz0 + i0;
            }
            /* set the Stu, Ref parts of output (they are easily copied or
               computed from inputs) and set the single pixel arrays
               (variable-length array, on stack!) for student (spix[]) and
               reference (rpix[]) code */
            real rdist, rr, ss, rpix[chan], spix[chan];
            for (uint cc = 0; cc < chan; cc++) {
                uint iidx = cc + chan * (i0 + isz0 * i1);
                if (isUChar) {
                    ss = stuUC[iidx];
                    rr = refUC[iidx];
                    out[cc + chan * (o0S + osz0 * o1S)] = (unsigned char)ss;
                    out[cc + chan * (o0R + osz0 * o1R)] = (unsigned char)rr;
                } else {
                    ss = stuRL[iidx];
                    rr = refRL[iidx];
                    // for student part: stripes if non-finite
                    out[cc + chan * (o0S + osz0 * o1S)]
                      = (isfinite(ss) ? bit8q(iqmin, ss, iqmax)
                                      : 255 * (((i0 + i1) / 5) % 2));
                    out[cc + chan * (o0R + osz0 * o1R)] = bit8q(iqmin, rr, iqmax);
                }
                spix[cc] = ss;
                rpix[cc] = rr;
            }
            dist0 += pixdist(&exact, &rdist, spix, rpix, chan, isUChar);
            if (rdist > worstD) {
                worstD = rdist;
                worstP[0] = i0;
                worstP[1] = i1;
            }
            /* doing slop: with student pixel spix known, search in
               *reference* image within a [-slop,slop]x[-slop,slop] window to
               find min(|stu-ref|). For now this is all done even if slop=0;
               the end result is that minD is set to normalized distance
               between stu and ref */
            real minD = 2;                           // bigger than (normalized) max 1
            for (int s1 = -slop; s1 <= slop; s1++) { // slop along slower index
                int j1 = (int)i1 + s1;               // slopped slower index
                j1 = j1 < 0 ? 0 : (j1 > (int)isz1 - 1 ? (int)isz1 - 1 : j1);
                for (int s0 = -slop; s0 <= slop; s0++) { // slop along faster index
                    int j0 = (int)i0 + s0;               // slopped faster index
                    j0 = j0 < 0 ? 0 : (j0 > (int)isz0 - 1 ? (int)isz0 - 1 : j0);
                    real rp[chan]; // ref pix values, at slop test location
                    for (uint cc = 0; cc < chan; cc++) {
                        uint iidx = cc + chan * (j0 + isz0 * j1);
                        rp[cc] = isUChar ? refUC[iidx] : refRL[iidx];
                    }
                    // not place to update exact or care about rawdist
                    real dd = pixdist(NULL, NULL, spix, rp, chan, isUChar);
                    if (dd < minD) {
                        minD = dd;
                        /* could set rpix to ref pixel that minimized dist
                        for (uint cc=0; cc<chan; cc++) rpix[cc] = rp[cc];
                        but that complicates interpretation of diff image */
                    }
                }
            }
            // total image distance dist uses minD from slop
            dist += minD;
            // set output difference pixel
            dpixout(out + 0 + chan * (o0D + osz0 * o1D), spix, rpix, chan, isUChar, minD,
                    i0, i1);
        } // for i0
    }     // for i1
    // per-image distances are mean of per-pixel distances
    dist /= isz0 * isz1;
    dist0 /= isz0 * isz1;
    real dcredit;
    if (fracCap > fracEps) {
        dcredit = (dist - fracEps) / (fracCap - fracEps);
        dcredit = 1 - (dcredit < 0 ? 0 : (dcredit > 1 ? 1 : dcredit));
    } else {
        dcredit = dist ? (real)0 : (real)1;
    }
    char *oname = MALLOC(strlen("diff-") + 3 /* tnum */
                           + strlen("-") + strlen(_oname) + 1,
                         char);
    assert(oname);
    airMopAdd(mop, oname, airFree, airMopAlways);
    sprintf(oname, "diff-%03u-%s", gradeTnum, _oname);
    if (!isUChar && airEndsWith(oname, ".nrrd")) {
        /* real-valued output normally saved in a .nrrd file, but for
           grading the output comparison image is 8-bit: save as PNG */
        strcpy(oname + strlen(oname) - strlen(".nrrd"), ".png");
    }
    int saveOut;
    if (!strcmp(gradePrefix, NOOP)) {
        // always save, never delete
        saveOut = 1;
    } else {
        // not no-op grading, doing real grading
        real fcredit; // final credit
        char texpl[TMP_STRLEN + 1];
        fcredit = timeAdjustCredit(dcredit, texpl, stutime, reftime);
        int points = pointsCredit(fcredit);
        const char *expl = describe(points, gradePoints);
        if (!dist0) {
            pltPrintf("%s %s%s: "
                      "your image %s same as reference%s\n",
                      gradePrefix, gradeMulti ? ". . " : "", expl,
                      exact ? "exactly" : "effectively", texpl);
        } else if (!dist) {
            pltPrintf("%s %s%s: "
                      "your image effectively same as reference "
                      "(thanks to slop %d; worst pixel %u %u)%s\n",
                      gradePrefix, gradeMulti ? ". . " : "", expl, slop, worstP[0],
                      worstP[1], texpl);
        } else if (points >= gradePoints) {
            char sxpl[TMP_STRLEN + 1];
            if (fracEps) {
                pltSprintf(sxpl,
                           "pixel tolerance, point granularity, or "
                           "%g%% image tolerance",
                           fracEps);
            } else {
                pltSprintf(sxpl, "pixel tolerance or point granularity");
            }
            pltPrintf("%s %s%s: "
                      "your and ref images differ by %g%% "
                      "(worst pixel %u %u) "
                      "but close enough given %s %s\n",
                      gradePrefix, gradeMulti ? ". . " : "", expl, 100 * dist, worstP[0],
                      worstP[1], sxpl, texpl);
        } else {
            pltPrintf("%s %s%s: your and ref images differ by "
                      "%g%% -> %g%% credit (worst pixel %u %u; see %s)%s\n",
                      gradePrefix, gradeMulti ? ". . " : "", expl, 100 * dist,
                      100 * dcredit, worstP[0], worstP[1], oname, texpl);
        }
        pltPrintf("%s %s %d (%s)\n", gradePrefix, gradeMulti ? ". ." : gradeEnd, points,
                  expl);
        GRADE_MULTI_ADD;
        /* how to know when to save difference image; earlier versions
           tested if dcredit < 1, but that meant that difference images were
           being saved even when full points were given */
        saveOut = ((points < gradePoints) // if not getting full points
                   ||                     /* OR, if are getting full points on image,
                                             but not some previous part of this multi
                                             (which would imply gradeMultiUnequal) */
                   (gradeMulti && gradeMultiUnequal()));
    }
    if (saveOut) {
        /* if interesting, record [iqmin,iqmax], which describes only the stu
           and ref parts of the image, but not difference; see dpixout() */
        if (!isUChar) {
            nout->oldMin = iqmin;
            nout->oldMax = iqmax;
        }
        char kvbuff[TMP_STRLEN + 1];
        pltSprintf(kvbuff, "%g", gradeImageQuantCap);
        if (nrrdKeyValueAdd(nout, "gradeImageQuantCap", kvbuff)
            || nrrdSave(oname, nout, NULL)) {
            char *err = biffGetDone(NRRD);
            fprintf(stderr, "%s: ERROR: couldn't save output image:\n%s", __func__, err);
            airMopError(mop);
            exit(1);
        }
        pltOutput(oname);
    } else {
        // file might not exist (ENOENT), and that's ok
        if (unlink(oname) && errno != ENOENT) {
            fprintf(stderr,
                    "%s: ERROR unlink(%s) (cleaning up file from "
                    "passing test) failed: %s\n",
                    __func__, oname, strerror(errno));
            airMopError(mop);
            exit(1);
        }
    }
    airMopOkay(mop);
    return;
}

#endif // SCIVIS_GRADE

/* if non-zero, do timings of execution. Estimated execution time
   will be the median of this many runs */
static uint timeMedian = 0;
// how verbose to be in reporting timing progress
static int timeVerbose = 0;
#define TIME_MEDIAN_MAX 501
/* number of runs beyond pltTimeMedian that should be measured;
   keeping the timeMedian shortest */
static uint timeExtra = 0;
/* if non-zero, demand that the relative error (interquartile range divided
   by median) go below this threshold; by re-running (at least) timeExtra
   more times */
static real timeSpread = 0;
uint
pltTimeDo(void) {
    return !!timeMedian;
}
uint
pltTimeMedian(void) {
    return timeMedian;
}
int
pltTimeVerbose(void) {
    return timeVerbose;
}
uint
pltTimeExtra(void) {
    return timeExtra;
}
real
pltTimeSpread(void) {
    return timeSpread;
}

static int
getenvTime(void) {
    int sgned;
    if (getenvInt(&timeMedian, sgned = 0, "SCIVIS_TIME_MEDIAN", 0)
        || getenvInt(&timeVerbose, sgned = 0, "SCIVIS_TIME_VERBOSE", 0)
        || getenvInt(&timeExtra, sgned = 0, "SCIVIS_TIME_EXTRA", 0)
        || getenvReal(&timeSpread, "SCIVIS_TIME_SPREAD", 0)) {
        fprintf(stderr, "%s:%s: trouble with a SCIVIS_TIME_* env vars\n", argv0,
                __func__);
        return 1;
    }
    if (timeMedian > TIME_MEDIAN_MAX) {
        fprintf(stderr,
                "%s: requested timeMedian %u exceeds internal "
                "limit %u; can't continue\n",
                __func__, timeMedian, TIME_MEDIAN_MAX);
        exit(1);
    }
    return 0;
}

// state for managing timing info
typedef struct {
    uint num; /* nominally: number of timings that contributed data
                 stored in dt[] array, but can be artificially lowered
                 to force more timing iterations */
    /* array (in *descending* order) of *fastest* timings found
       (easy to have extraneously large timing; we ignore those) */
    long dt[TIME_MEDIAN_MAX + 1];
} timeBuffer;

/* currently only using timeData[0], but keeping indexing ability since it
   might later be useful */
static timeBuffer timeData[1];
// statically allocated but not initialized

#define TIME_CHECK                                                                      \
  if (!timeMedian) {                                                                    \
    fprintf(stderr, "%s: timeMedian == 0; nothing to do!\n", __func__);                 \
    return 1;                                                                           \
  } else {                                                                              \
    if (timeSpread > 0) {                                                               \
      if (!(timeMedian >= 3)) {                                                         \
        pltFprintf(stderr,                                                              \
                   "%s: need timeMedian >= 3 (not %u) "                                 \
                   "for timeSpread %g\n",                                               \
                   __func__, timeMedian, timeSpread);                                   \
        exit(1);                                                                        \
      }                                                                                 \
      if (!(timeExtra >= timeMedian)) {                                                 \
        pltFprintf(stderr,                                                              \
                   "%s: need timeExtra >= timeMedian %u "                               \
                   "(not %u) for timeSpread %g\n",                                      \
                   __func__, timeMedian, timeExtra, timeSpread);                        \
        exit(1);                                                                        \
      }                                                                                 \
    }                                                                                   \
  }

/* This is returning something only because we want to use TIME_CHECK across
   functions, but pltTimeAdd actually needs to return something, and, it needs to
   return a non-zero value to mean "we're done, we have enough data to estimate the
   time" (so don't loop back and re-run function).  Hence all the "return 1" for
   non-error conditions. */
uint
pltTimeInit(void) {
    TIME_CHECK;
    timeData->num = 0;
    for (uint ti = 0; ti <= timeMedian; ti++) {
        timeData->dt[ti] = 0;
    }
    return 1;
}

static long
theMedian(const long *tt, uint num) {
    long ret;
    if (num % 2) { // odd
        ret = tt[num / 2];
    } else { // even
        ret = (tt[num / 2] + tt[num / 2 - 1]) / 2;
    }
    return ret;
}

/* "spread" is the interquartile range
   https://en.wikipedia.org/wiki/Interquartile_range
   divided by median */
static real
theSpread(const long *tt, uint num) {
    long loq = tt[3 * num / 4]; // the values are descending
    long hiq = tt[num / 4];
    return (real)(hiq - loq) / (real)theMedian(tt, num);
}

long
pltTimeAdd(long newtime) {
    TIME_CHECK;
    uint num = timeData->num;
    long *tt = timeData->dt;
    if (timeVerbose > 2) {
        printf("%s(%ld): HELLO; num=%u\n", __func__, newtime, num);
        if (timeVerbose > 3) {
            for (uint ti = 0; ti < timeMedian; ti++)
                printf("   %ld=tt[%u]", tt[ti], ti);
            printf("\n");
        }
    }
    if (!newtime) {
        /* can't allow 0 to represent a real timing: the median could then be
           0, and then the timing spread (interquartile width divided by
           median) would be NaN */
        newtime = 1;
    }
    uint ni = 0; // index of newtime
    while (newtime < tt[ni])
        ni++;
    if (num < timeMedian) {
        // shift lesser times towards higher indices
        for (uint si = num; si > ni; si--)
            tt[si] = tt[si - 1];
        tt[ni] = newtime;
    } else if (ni) {
        /* if !ni then newtime >= current slowest (biggest) time, so we
           don't store that even slower time.  Otherwise (if ni) then
           newtime < current slowest; shift bigger times to lower indices */
        ni--;
        for (uint si = 0; si < ni; si++)
            tt[si] = tt[si + 1];
        tt[ni] = newtime;
    }
    num++;
    char expl[TMP_STRLEN + 1] = "";
    long ret;
    if (num < timeMedian + timeExtra) {
        sprintf(expl,
                "because more timing data needed: "
                "num timings %u < %u+%u",
                num, timeMedian, timeExtra);
        ret = 0;
    } else {
        real error = theSpread(tt, timeMedian);
        if (!(timeSpread > 0) || error < timeSpread) {
            if (timeSpread > 0) {
                pltSprintf(expl,
                           "done (spread %g < %g) after "
                           "%u = %u+%u timings",
                           error, timeSpread, num, timeMedian, timeExtra);
            } else {
                sprintf(expl,
                        "done (w/out minimizing error) "
                        "after %u = %u+%u timings",
                        num, timeMedian, timeExtra);
            }
            ret = theMedian(tt, timeMedian);
        } else {
            /* we want to minimize timing error, but we haven't yet converged
               to an accurate estimation. As a hack, we force more work by
               pretending we don't have enough data, so that we'll need
               timeExtra more timings. Also, to mix things up, we drop the
               fastest time, since a spuriously fast time will otherwise never
               get replaced by subsequent data. */
            num = timeMedian - 1; // safe because of TIME_CHECK
            tt[num] = 0;
            pltSprintf(expl, "because more timing data needed: spread %g > %g", error,
                       timeSpread);
            ret = 0;
#ifdef SCIVIS_GRADE
            pltPrintf("%s %s %s\n", gradePrefix, gradeContinue, expl);
            fflush(stdout);
#endif
        }
    }
    timeData->num = num;
    if (timeVerbose > 2) {
        printf("%s(%ld): bye; num=%u\n", __func__, newtime, timeData->num);
        if (timeVerbose > 3) {
            for (uint ti = 0; ti < timeMedian; ti++)
                printf("   %ld=tt[%u]", tt[ti], ti);
            printf("\n");
        }
    }
    if ((timeVerbose && ret) || timeVerbose > 1) {
        printf("%s: returning %ld (%s)\n", __func__, ret, expl);
    }
    return ret;
}

/* The main function of plotr. This sets up thigs related to signal handling,
   grading, timing, and command-line parsing, but doesn't do any of the real
   work of plotr. The command-line parsing for "plotr foo" happens in
   plotr_foo.c; and that will in turn call other functions to do the real work
   (and the grading of the work). */
int
main(int argc, const char **argv) {
    USED(getenvStr);
    pltOutput(NULL);
    argv0 = argv[0];
    setupSigHandle();
#ifdef SCIVIS_GRADE
    if (!(argc == 2 && !strcmp("cmpld", argv[1]))) {
        /* i.e., NOT running "plotr cmpld" (for which we don't actually
           need any of the grading env vars to be set and parsed) */
        if (getenvGrade()) {
            fprintf(stderr, "%s: trouble with SCIVIS_GRADE_... env vars\n", argv[0]);
            fprintf(stderr,
                    "%s: If you want to run grade/plotr (in order to\n"
                    "execute both your and reference code), but without all\n"
                    "env var machinery set up by grade/go, you can set the\n"
                    "env var \"SCIVIS_GRADE_NOOP\" to \"1\".\n",
                    argv[0]);
            exit(1);
        }
        if (!(argc == 2 && !strcmp("env", argv[1]))) {
            /* i.e., NOT running "plotr env" (for which we don't need to
               print starting line) */
            /* before doing anything else, print the line that starts the
               logging of the execution of this program */
            if (strcmp(gradePrefix, NOOP)) { // but only if not noop grading
                /* start with newline, for legibility and in case
                   previous output didn't end with newline */
                printf("\n%s %s (%d)", gradePrefix, gradeStart, gradePoints);
                printf(" plotr"); // to clarify args that follow
                for (int ii = 1; ii < argc; ii++) {
                    printf(" %s", argv[ii]);
                }
                printf("\n");
                /* without this fflush, the gradeStart might not be seen
                   before a segfault (the above "\n" doesn't actually force
                   a flush) */
                fflush(stdout);
            }
        }
    }
#endif
    // set pltVerbose based on environment variable SCIVIS_VERBOSE
#ifndef NDEBUG
    if (getenvInt(&pltVerbose, 1 /* signed */, "SCIVIS_VERBOSE", 0)) {
        fprintf(stderr, "%s trouble with the SCIVIS_VERBOSE env var\n", argv0);
        exit(1);
    }
#else
    const char *verbs = getenv("SCIVIS_VERBOSE");
    if (verbs) {
        fprintf(stderr,
                "%s: environment variable SCIVIS_VERBOSE is \"%s\", "
                "but we're compiled with NDEBUG so there's no global "
                "pltVerbose variable to set "
                "(in fact, pltVerbose is #define'd to \"0\").\n",
                argv[0], verbs);
    }
#endif
    if (pltVerbose) {
        fprintf(stderr, "%s: pltVerbose set to %d\n", argv[0], pltVerbose);
    }
#if RR_DOING
    getenvInt(&RR_seed, 0 /* sgned */, "SCIVIS_RR_SEED", 0);
    if (!RR_seed) {
        fprintf(stderr, "\n%s: Random-rounding version of plotr\n\n", argv0);
        fprintf(stderr, "To use, set environment variable "
                        "SCIVIS_RR_SEED to a non-zero seed value,\n");
        fprintf(stderr,
                "to initialize a length-%u buffer of per-operation "
                "rounding directions:\n",
                RR_NUM);
        fprintf(stderr, "* Seeds 1 or 2 always set 0 (downward) or 1 (upward), "
                        "respectively.\n");
        fprintf(stderr, "* Seeds 3 or 4 set alternating 0 1 or 1 0, "
                        "respectively.\n");
        fprintf(stderr, "* Seeds 5 and above initialize a random-number generator, "
                        "which determines\n  the rounding direction values.\n");
        exit(1);
    }
    _RR_init(1);
#else
    pltRoundGetenv();
#endif
    getenvTime();
    airArray *mop = airMopNew();
    hestParm *hparm = hestParmNew();
    hparm->cleverPluralizeOtherY = AIR_TRUE;
    hparm->dieLessVerbose = AIR_TRUE;
    hparm->noBlankLineBeforeUsage = AIR_TRUE;
    struct winsize wsz;
    ioctl(0, TIOCGWINSZ, &wsz);
    /* -2 because else "\" for continuation can wrap when
       it shouldn't (may be a hest bug) */
    hparm->columns = AIR_MAX(59, wsz.ws_col - 2);
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
    const char rcmd[] = "rplotr";
    const char rcmdm[] = "rplotr-osx";
    const char rcmdl[] = "rplotr-linux";
    const char cmd[] = "plotr";
    const char *cmds;
    if (!strcmp(rcmd, argv[0] + strlen(argv[0]) - strlen(rcmd))
        || !strcmp(rcmdm, argv[0] + strlen(argv[0]) - strlen(rcmdm))
        || !strcmp(rcmdl, argv[0] + strlen(argv[0]) - strlen(rcmdl))) {
        /* if we seem to be being run as (some kind of) rplotr, then
           that is in fact how we should self-identify */
        cmds = rcmd;
    } else {
        cmds = cmd;
    }
    const char banner[] = "SciVis-2023 Project 1"
#ifdef SCIVIS_GRADE
                          " (For Grading)"
#endif
      ;
    int E = 0;
    if (!pltRoundBracket()) {
        // normal operation; run once
        E = unrrduCmdMain(argc, argv, cmds, banner, cmdList, hparm, stderr);
    } else {
        /* bracketing: run twice w/ opposite rounding modes,
           renaming output files to add "dn-" and "up-" prefixes */
        fprintf(stderr, "%s: with round DOWNWARD\n", argv0);
        pltRoundDirSet(-1);
        pltOutput(NULL);
        E = unrrduCmdMain(argc, argv, cmds, banner, cmdList, hparm, stderr);
        if (E) goto done;
        E = pltOutputRenamePrefix("dn-");
        if (E) goto done;
        fprintf(stderr, "%s: with round UPWARD\n", argv0);
        pltRoundDirSet(+1);
        pltOutput(NULL);
        E = unrrduCmdMain(argc, argv, cmds, banner, cmdList, hparm, stderr);
        if (E) goto done;
        E = pltOutputRenamePrefix("up-");
    }
done:
    airMopOkay(mop);
    return E;
}
