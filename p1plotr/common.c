/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

#include "plt.h"
#include "pltPrivate.h"

#include <stdarg.h>
#include <string.h> // for strcpy()
#include <stdlib.h>
#include <stdint.h>   // for uint{32,64}_t
#include <fenv.h>     // for fesetround()
#include <time.h>     // for clock()
#include <sys/time.h> // for getrusage or gettimeofday
/* currently pltMicrosecondsUsed not using getrusage()
#include <sys/resource.h>  // for getrusage
*/

// see info about pltVerbose in plt.h
#ifndef NDEBUG
int pltVerbose = 0;
#endif

const char *pltBiffKey = "plt";

#if SCIVIS_REAL_IS_DOUBLE
const int pltRealIsDouble = 1;
#else
const int pltRealIsDouble = 0;
#endif

typedef union {
#if SCIVIS_REAL_IS_DOUBLE
    // for accessing bits of a 64-bit double
    uint64_t i;
    double v;
#else
    // for accessing bits of a 32-bit float
    uint32_t i;
    float v;
#endif
} Real;

typedef union {
    // only for accessing bits of a 32-bit float
    uint32_t i;
    float v;
} Float;

real
pltNan(unsigned short payload) {
    Real rr;
    /* same logic for both meanings of real: make a non-finite number by setting all the
       exponent bits, make it a NaN by making sure highest bit of fraction is on (else
       it would be an infinity), and then put the 16-bit payload in the lowest bits. */
#if SCIVIS_REAL_IS_DOUBLE
    rr.i = ((uint64_t)0x7ff << 52) | ((uint64_t)1 << 51) | ((uint64_t)payload);
#else
    rr.i = ((uint32_t)0xff << 23) | ((uint32_t)1 << 22) | ((uint32_t)payload);
#endif
    return rr.v;
}

unsigned short
pltNanPayload(real nval) {
    unsigned short pay;
    if (!isnan(nval)) {
        pay = (unsigned short)(-1);
    } else { // it really is a nan
        Real rr;
        rr.v = nval;
#if SCIVIS_REAL_IS_DOUBLE
        pay = rr.i & 0xffff;
#else
        pay = rr.i & 0xffff;
#endif
    }
    return pay;
}

static int printLogDo = AIR_FALSE;
static char *printLog = NULL;
#ifdef PTHREAD_SCOPE_SYSTEM // something pthread.h is supposed to #define
pthread_mutex_t printLogMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void
pltPrintLogStart(void) {
    printLogDo = AIR_TRUE;
    printLog = airFree(printLog);
}

static void
printLogAdd(const char *line) {
    if (!(line && strlen(line))) {
        return;
    }
#ifdef PTHREAD_SCOPE_SYSTEM
    pthread_mutex_lock(&printLogMutex);
#endif
    size_t oldLen = printLog ? strlen(printLog) : 0;
    size_t newLen = oldLen + strlen(line);
    char *buff = AIR_CALLOC(newLen + 1, char);
    assert(buff);
    buff[0] = '\0';
    if (oldLen) {
        strcpy(buff, printLog);
    }
    strcat(buff, line);
    free(printLog);
    printLog = buff;
#ifdef PTHREAD_SCOPE_SYSTEM
    pthread_mutex_unlock(&printLogMutex);
#endif
    return;
}

const char *
pltPrintLog(void) {
    return printLog;
}

void
pltPrintLogFinish(void) {
    printLog = airFree(printLog);
    printLogDo = AIR_FALSE;
}

#ifndef SCIVIS_PRINTF_NOOP
/* pltPrintf (like pltSprintf, pltFprintf) is a thin wrapper around printf; its
   main purpose is to interpret the "%g" conversion specification as a request to print
   a "real" with enough precision so that it could exactly reproduced by parsing it. The
   cleverness to re-interpret"%g" is in reformat(), which allocates a new formatting
   string.

   Note that due to the "default argument promotion" that always affects function
   arguments without a prototype (including variable arguments, i.e. the "va_list"
   below), whether real=float or real=double, the actual "real" values received via
   var-args, both here and when then passed to vsprintf or vfprintf, will be a
   **double**. The idea, however, is still to only print the double with enough precision
   for the current meaning of "real". */
static char *
reformat(const char *restrict fmtOld, va_list vargs) {
    char *restrict fmtNew;

    assert(fmtOld);
    /* allocate the new format string, which at worst needs to convert a string
       consisting of only "%g" to "%.17g", i.e. length-2 string to length-5 string plus
       0-termination.  So, being sloppy, make buffer 3x bigger */
    fmtNew = MALLOC(3 * strlen(fmtOld), char);
    assert(fmtNew);

    /* create new format string, including consuming by va_arg() all the var-args */
    const char *restrict ii = fmtOld; // pointer into input string
    char *restrict oo = fmtNew;       // pointer into output string
    while (*ii) {
        *oo++ = *ii; // copy in to out; increment only out
        if ('%' == *ii) {
            // it's either an escaped % ("%%") or a conversion sequence
            ii++; // increment input past %
            while (*ii) {
                if ('%' == *ii) { // it's an escaped %
                    *oo++ = *ii++;
                    break; // out of inner loop
                }
                // else it's a conversion sequence
                if ('g' == ii[0] && '%' == ii[-1]) {         // it's "%g" exactly
                    real valG = (real)va_arg(vargs, double); // val given
                    if (!isfinite(valG)) {
                        // we got a special FP value; no cleverness
                        *oo++ = *ii++; // copy 'g' of "%g"
                        break;         // out of inner loop
                    }
                    // else we do try some cleverness
                    char buff[128];
                    real valP; // value parsed
                    sprintf(buff, "%g", valG);
                    sscanf(buff,
#  if SCIVIS_REAL_IS_DOUBLE
                           "%lf",
#  else
                           "%f",
#  endif
                           &valP);
                    if (valG == valP) {
                        /* actually "%g" by itself fully captured the
                           value, no need for more precision */
                        *oo++ = *ii++; // copy 'g' of "%g"
                        break;         // out of inner loop
                    }
                    /* else need to increase precision (17 and 9 here may
                       be overkill, but anything else means more tests) */
#  if SCIVIS_REAL_IS_DOUBLE
                    // conversion sequence is ".17g"
                    *oo++ = '.';
                    *oo++ = '1';
                    *oo++ = '7';
#  else
                    // conversion sequence is ".9g"
                    *oo++ = '.';
                    *oo++ = '9';
#  endif
                    *oo++ = *ii++; // copy 'g' of "%g"
                    // end of: was given %g exactly
                } else {
                    // somewhere in conv seq (just not the g of "%g")
                    int dstI;
                    real dstR;
                    char *dstP;
                    *oo++ = *ii; // copy to output
                    int done = 0;
                    switch (ii[0]) {
                        // ------------- %...diouxX: int
                    case 'd':
                    case 'i':
                    case 'o':
                    case 'u':
                    case 'x':
                    case 'X':
                        // modifiers:
                        //    ll: long long
                        //     l: long
                        //     j: intmax_t
                        //     t: ptrdiff_t
                        //     z: size_t
                        if (ii - fmtOld >= 2 && 'l' == ii[-2] && 'l' == ii[-1]) {
                            dstI = (int)va_arg(vargs, long long int);
                        } else if ('l' == ii[-1]) {
                            dstI = (int)va_arg(vargs, long int);
                        } else if ('j' == ii[-1]) {
                            dstI = (int)va_arg(vargs, intmax_t);
                        } else if ('t' == ii[-1]) {
                            dstI = (int)va_arg(vargs, ptrdiff_t);
                        } else if ('z' == ii[-1]) {
                            dstI = (int)va_arg(vargs, size_t);
                        } else { // no modifers
                            dstI = (int)va_arg(vargs, int);
                        }
                        done = 1;
                        break;
                        // ------------- %DOU: long int
                    case 'D':
                    case 'O':
                    case 'U':
                        dstI = (int)va_arg(vargs, long int);
                        done = 1;
                        break;
                        // ------------- %...c: int
                    case 'c':
                        // modifier:  l: wint_t
                        if ('l' == ii[-1]) {
                            dstI = (int)va_arg(vargs, wint_t);
                        } else {
                            dstI = (int)va_arg(vargs, int);
                        }
                        done = 1;
                        break;
                        // ------------- %...C: wint_t
                    case 'C':
                        dstI = (int)va_arg(vargs, long int);
                        done = 1;
                        break;
                        // ------------- %...aAeEfFgG: double
                    case 'a':
                    case 'A':
                    case 'e':
                    case 'E':
                    case 'f':
                    case 'F':
                    case 'g':
                    case 'G':
                        // modifier:  L: long double
                        if ('L' == ii[-1]) {
                            dstR = (real)va_arg(vargs, long double);
                        } else {
                            dstR = (real)va_arg(vargs, double);
                        }
                        done = 1;
                        break;
                        // ------------- %s: char *
                        // %ls or %S: wchar_t *
                        // %p: void *
                        // %n: int *
                    case 's':
                    case 'S':
                    case 'p':
                    case 'n':
                        // all pointers are same size, right?
                        dstP = (char *)va_arg(vargs, char *);
                        done = 1;
                        break;
                    }
                    ii++;
                    USED(dstI, dstR);
                    USEd(dstP);
                    if (done) break; // out of inner loop
                }
            }     // inner while (*ii)
        } else {  // outer loop *ii != %
            ii++; // increment input past char not in conv seq
        }
    }           // outer while (*ii)
    *oo = '\0'; // 0-terminate output string
    return fmtNew;
}

int
pltPrintf(const char *restrict fmtOld, ...) {
    va_list vargs;
    va_start(vargs, fmtOld);
    char *fmtNew = reformat(fmtOld, vargs);
    va_end(vargs);

    va_start(vargs, fmtOld);
    int ret = vprintf(fmtNew, vargs);
    va_end(vargs);

    if (printLogDo) {
        char *line = NULL;
        va_start(vargs, fmtOld);
        int vret = vasprintf(&line, fmtNew, vargs);
        va_end(vargs);
        if (-1 != vret) {
            printLogAdd(line);
            free(line);
        }
    }
    free(fmtNew);
    return ret;
}

// pltSprintf is a thin wrapper around sprintf; same purpose as pltPrintf
// alas, copy-pasta
int
pltSprintf(char *restrict dest, const char *restrict fmtOld, ...) {
    va_list vargs;
    va_start(vargs, fmtOld);
    char *fmtNew = reformat(fmtOld, vargs);
    va_end(vargs);

    va_start(vargs, fmtOld);
    int ret = vsprintf(dest, fmtNew, vargs);
    va_end(vargs);
    free(fmtNew);
    return ret;
}

// pltFprintf is a thin wrapper around fprintf; same purpose as pltPrintf
// same copy-pasta
int
pltFprintf(FILE *file, const char *restrict fmtOld, ...) {
    va_list vargs;
    va_start(vargs, fmtOld);
    char *fmtNew = reformat(fmtOld, vargs);
    va_end(vargs);

    va_start(vargs, fmtOld);
    int ret = vfprintf(file, fmtNew, vargs);
    va_end(vargs);
    free(fmtNew);
    return ret;
}
#endif
// #else compiling with SCIVIS_PRINTF_NOOP defined

// clamps vv to mm*7/8 with two linear ramps
#define QLAMP(vv, mm) ((vv) < (mm) / 2 ? (vv) : (mm) / 2 + 3 * ((vv) - (mm) / 2) / 4)

/* pltFloatDist tries to compute the distance between two floats, in units of ULPs.
   When the two floats are both finite and have the same sign, this is easy: it really is
   just the difference beween the values interpreted as ints.  This function also handles
   the messier cases of having two finite values of opposite sign, or one or two
   non-finite values.

   This is specific to floats, rather than general for reals, because reporting the ULP
   difference between two doubles can require a uint64. Having the return type depend on
   the meaning of real seems complicated, so we want to return one kind of int, which
   would have to be uint64, but uint64s are more annoying to printf.  We could have the
   return type itself be real, but then values printed in scientific notation will be
   harder for downstream grading scripts to interpret.
*/
uint
pltFloatDist(float A, float B) {
    int Anf = !isfinite(A);
    int Bnf = !isfinite(B);
    Float FA, FB; // not "Real": this needs to work even with real==double
    uint ret;
    /* biggest finite value is with expo just shy of all 1s, and frac field all
       1s: ((2^8 - 2) << 23) + 2^23 - 1 == 2139095039 > 2000000000. But, if A
       and B have this big magnitude with opposite signs, so diff can be twice
       that. Choose a round number for visual recogition. */
    uint maxd = 4000000000;
    switch (Anf + Bnf) {
    case 0:
        // both values finite; computing distance is straightforward
        FA.v = fabs(A);
        FB.v = fabs(B);
        uint Ai = FA.i;
        uint Bi = FB.i;
        if ((A >= 0) == (B >= 0)) {
            // easy case: two finite values of same sign
            uint diff = Ai > Bi ? Ai - Bi : Bi - Ai;
            ret = QLAMP(diff, maxd);
        } else {
            /* harder: two finite values of different sign. The
               density of values around zero makes this a little goofy,
               hence the very adhoc /2 to lessen the difference */
            ret = QLAMP(Ai / 2, maxd / 2) + QLAMP(Bi / 2, maxd / 2);
        }
        break;
    case 1:
        // only one non-finite value: say simply they're very different
        ret = maxd;
        break;
    case 2:
        // two non-finite values
        if (isnan(A) && isnan(B)) {
            ret = 0; // really, no meaningful difference between 2 NaNs
        } else if (isnan(A) || isnan(B)) {
            // one NaN and one inf, which seems like a big difference
            ret = maxd;
        } else {
            // two infs
            if ((A > 0) == (B > 0)) {
                // two infs of same sign; call them equal
                ret = 0;
            } else {
                // two infs of different sign: very different
                ret = maxd;
            }
        }
        break;
    }
    return ret;
}
#undef QLAMP

/*
  Allocates and returns a single string storing the command-line that got passed via argv
*/
char *
pltCommandLine(const char *me, const char **argv) {
    size_t clen = strlen(me) + strlen(" ");
    for (uint ai = 0; argv[ai]; ai++) {
        clen += strlen(argv[ai]) + strlen(" ");
        if (strchr(argv[ai], ' ')) clen += 2;
    }
    clen += 1; // 0-termination
    char *cstr = MALLOC(clen, char);
    assert(cstr);
    cstr[0] = '\0';
    sprintf(cstr, "%s ", me);
    for (uint ai = 0; argv[ai]; ai++) {
        if (strchr(argv[ai], ' ')) strcat(cstr, "\"");
        strcat(cstr, argv[ai]);
        if (strchr(argv[ai], ' ')) strcat(cstr, "\"");
        strcat(cstr, " ");
    }
    return cstr;
}

/*
  This allocates and fills a string suitable for printing, containing the formatting of
  the given list of strings, which is really a 2D array of strings. Each input "row" ends
  with "\", and the whole thing ends with a NULL
*/
#define END  "\\" // single backslash, quoted
#define MARG 4
char *
pltTodoStr(const char **strs) {
    char *ret = NULL;
    const char **ss = strs;
    const char alsoTime[] = "  (Correctness grading also includes "
                            "running \"grade/go time\")\n";
    if (!ss) {
        fprintf(stderr, "%s: got NULL!\n", __func__);
        return NULL;
    }
    // count # columns
    uint colnum = 0;
    while (*ss && strcmp(*ss, END)) {
        colnum++;
        ss++;
    }
    if (!(*ss)) {
        fprintf(stderr, "%s: hit NULL before row ending \"%s\"\n", __func__, END);
        return NULL;
    }
    // count # rows
    uint rownum = 1;
    do {
        rownum++;
        ss += colnum + 1;
        if (*ss && strcmp(*ss, END)) {
            fprintf(stderr, "%s: got \"%s\" != \"%s\" ending row %u\n", __func__, *ss,
                    END, rownum);
            return NULL;
        }
    } while (*ss);
    if (rownum < 3) {
        fprintf(stderr, "%s: number of rows %u too low\n", __func__, rownum);
        return NULL;
    }
    // count # rows, and learn biggest width cwid[ci] in each column ci
    uint *cwid = MALLOC(colnum, uint);
    assert(cwid);
    uint rowlen = 0;
    for (uint ci = 0; ci < colnum; ci++) {
        uint maxlen = 0;
        for (uint ri = 0; ri < rownum; ri++) {
            ss = strs + ci + (colnum + 1) * ri;
            uint len = (uint)strlen(*ss);
            if (len > maxlen) maxlen = len;
        }
        cwid[ci] = maxlen;
        rowlen += maxlen + MARG;
    }
    // allocate and set the output
    uint retLen = ((rowlen + 1) * (rownum + 1) + strlen(alsoTime) + 1);
    ret = MALLOC(retLen, char);
    assert(ret);
    char *rr = ret;
    for (uint ri = 0; ri < rownum; ri++) {
        for (uint ci = 0; ci < colnum; ci++) {
            ss = strs + ci + (colnum + 1) * ri;
            for (uint pi = 0; pi < MARG; pi++)
                *rr++ = ' '; // before column
            uint len = (uint)strlen(*ss);
            strcpy(rr, *ss);
            rr += len; // column entry
            // space after entry
            for (uint pi = len; pi < cwid[ci]; pi++)
                *rr++ = ' ';
        }
        *rr++ = '\n';
        if (!ri) {
            // after first row, a row of -x
            for (uint pi = 0; pi < MARG; pi++)
                *rr++ = ' ';
            for (uint pi = MARG; pi < rowlen; pi++)
                *rr++ = '-';
            *rr++ = '\n';
        }
    }
    /* HACK: see if alsoTime should actually be printed
       (not printed for pcpb) */
    FILE *gtime = fopen("grade/T-time", "r");
    if (gtime) {
        strcpy(rr, alsoTime);
        rr += strlen(alsoTime);
        fclose(gtime);
    }
    *rr++ = '\0';
    free(cwid);
    return ret;
}
#undef END
#undef MARG

long int
pltMicrosecondsUsed(void) {
    /*
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    long ret = (long)(ru.ru_utime.tv_sec*1000000 + ru.ru_utime.tv_usec +
                      ru.ru_stime.tv_sec*1000000 + ru.ru_stime.tv_usec);
    */
    long ret = (long)(clock() / (CLOCKS_PER_SEC / 1000000));
    return ret;
}

void
pltWallTime(long SecMicrosec[2]) {
    /* This should be using a more modern timer like clock_gettime, but that isn't
       available on all OSX versions, and the mach-specific alternatives on OSX seem
       overly complex.  The risk of time changing on us because of NTP (c.f.
       http://goo.gl/Y4eDGz ) are less of a problem for us because in the context of
       repeated timing, spuriously fast or slow times will be discarded. */
    struct timeval tv;
    gettimeofday(&tv, NULL /* no time zone info requested */);
    // alas, no error handling
    SecMicrosec[0] = (long)tv.tv_sec;  // seconds since 1970
    SecMicrosec[1] = (long)tv.tv_usec; // microseconds
    return;
}

long int
pltMicrosecondsWallTimeDiff(const long sms1[2], const long sms0[2]) {
    return 1000 * 1000 * (sms1[0] - sms0[0]) + sms1[1] - sms0[1];
}

#define OUTPUT_NUM   10
#define FNAME_STRLEN 300
static char output[OUTPUT_NUM][FNAME_STRLEN + 1];

/* if fname is non-NULL and non-zero length: declare an output
   else: reset list of outputs */
void
pltOutput(const char *fname) {
    if (fname && strlen(fname)) {
        uint ii = 0;
        for (ii = 0; ii < OUTPUT_NUM; ii++) {
            if (!strlen(output[ii])) break;
        }
        if (ii < OUTPUT_NUM) airStrcpy(output[ii], FNAME_STRLEN + 1, fname);
    } else {
        for (uint ii = 0; ii < OUTPUT_NUM; ii++) {
            strcpy(output[ii], "");
        }
    }
    return;
}

// rename outputs to have given prefix
int
pltOutputRenamePrefix(const char *prefix) {
    int E = 0;
    for (uint ii = 0; !E && ii < OUTPUT_NUM; ii++) {
        if (!strlen(output[ii])) break;
        if (!strcmp("-", output[ii])) {
            fprintf(stderr, "%s: can't rename output[%u] stdout \"-\"\n", __func__, ii);
            continue;
        }
        char *tmp = MALLOC(strlen(prefix) + strlen(output[ii]) + 1, char);
        assert(tmp);
        char *slsh = strrchr(output[ii], '/');
        if (!slsh) {
            sprintf(tmp, "%s%s", prefix, output[ii]);
        } else {
            char out[FNAME_STRLEN + 1];
            strcpy(out, output[ii]);
            slsh += out - output[ii];
            *slsh = '\0';
            sprintf(tmp, "%s/%s%s", out, prefix, slsh + 1);
        }
        E = rename(output[ii], tmp);
        fprintf(stderr, "%s: rename %s --> %s\n", __func__, output[ii], tmp);
        if (E) {
            fprintf(stderr, "%s: trouble with rename(%s,%s): %s\n", __func__, output[ii],
                    tmp, strerror(errno));
        }
        free(tmp);
    }
    return E;
}
#undef OUTPUT_NUM
#undef FNAME_STRLEN

/* roundDir just represents FE_UPWARD (dir>0), FE_DOWNWARD
   (dir<0), or FE_TONEAREST (dir==0), but without the #include
   <fenv.h> etc. */
static int roundDir = 0;
void
pltRoundDirSet(int dir) {
    if (dir > 0) {
        roundDir = 1;
    } else if (dir < 0) {
        roundDir = -1;
    } else {
        roundDir = 0;
    }
    return;
}
static int roundBracket = 0; // whether to do bracketing
int
pltRoundDir(void) {
    return roundDir;
}
int
pltRoundBracket(void) {
    return roundBracket;
}

/* If environment variable "SCIVIS_ROUND" is one of: "NEAREST",  "UPWARD", "DOWNWARD",
   or "BRACKET", then set roundDir and roundBracket accordingly */
void
pltRoundGetenv(void) {
    static const char envName[] = "SCIVIS_ROUND";
    char *envStr = getenv(envName);
    if (!envStr) { // that environment variable not set
        return;
    }
    if (!strcmp(envStr, "NEAREST")) {
        roundDir = 0;
        roundBracket = 0;
    } else if (!strcmp(envStr, "UPWARD")) {
        roundDir = 1;
        roundBracket = 0;
    } else if (!strcmp(envStr, "DOWNWARD")) {
        roundDir = -1;
        roundBracket = 0;
    } else if (!strcmp(envStr, "BRACKET")) {
        /* this isn't a rounding mode, but the indicator that we should run the program
           twice, with both upwards and downwards; see plotr.c */
        roundDir = 0;
        roundBracket = 1;
        /* except not if SCIVIS_ROUND_BRACKET_FREEZE is set, and set to "1" then that is
           the is the same as no bracketing */
        envStr = getenv("SCIVIS_ROUND_BRACKET_FREEZE");
        if (envStr) {
            // was set, but is it 1?
            if (!strcmp(envStr, "1")) {
                roundBracket = 0;
            }
        }
    } else {
        fprintf(stderr,
                "%s: WARNING env var %s has unrecognized value "
                "\"%s\" (not one of NEAREST, UPWARD, DOWNWARD, or BRACKET)\n",
                __func__, envName, envStr);
    }
    return;
}

// wrapper around fesetround(); prints some debugging messages
static void
fesetroundWrap(int dir) {
    int round;
    const char *str;
    if (dir < 0) {
        round = FE_DOWNWARD;
        str = "FE_DOWNWARD";
    } else if (dir > 0) {
        round = FE_UPWARD;
        str = "FE_UPWARD";
    } else {
        round = FE_TONEAREST;
        str = "FE_TONEAREST";
    }
    // not supporting FE_TOWARDZERO
    int ret = fesetround(round);
    if (ret) {
        fprintf(stderr, "%s: WARNING fesetround(%d=%s) returned %d\n", __func__, round,
                str, ret);
        return;
    }
    // else rounding mode was established
    if (pltVerbose) {
        printf("%s: FP rounding mode successfully set to %s\n", __func__, str);
    }
    return;
}

/* public interfaces for fesetroundWrap */
void
pltRoundSet(void) {
    fesetroundWrap(roundDir);
}
void
pltRoundUnset(void) {
    fesetroundWrap(0);
}

/* silly little function that returns the max of oldmax and the longest length of the
   string holding an accurate decimal representation of any vv[i] from real array vv */
uint
pltSprintLenMax(uint oldmax, const real *vv, uint vvNum) {
    USEd(vv); // since pltSprintf is turned into a no-op for impf test
    char buff[128];
    uint ret = oldmax;
    for (uint ii = 0; ii < vvNum; ii++) {
        pltSprintf(buff, "%g", vv[ii]);
        uint len = (uint)strlen(buff);
        ret = len > ret ? len : ret;
    }
    return ret;
}
