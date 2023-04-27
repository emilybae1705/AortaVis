/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

// ########################## Beginning of things shared between projects

/*
  NOTE: Document here (after the "begin student code" line)
  // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (other resources)
  // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (0L in ref)
  anyone or anything extra you used for this work. Besides your instructor, TA, and
  project partner (if you had one), who else did you you work with? What other code, web
  pages, or documents (outside the class readings) helped you understand what to do or
  how to do it?  This information is important for understanding sources of code
  similarity, and for improving future offerings of the class. Err on the side of listing
  more potential influences rather than fewer.
*/

#ifndef PLT_HAS_BEEN_INCLUDED
#define PLT_HAS_BEEN_INCLUDED

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE // https://man7.org/linux/man-pages/man3/asprintf.3.html
#endif
#include <stdio.h>  // for printf
#include <stdint.h> // for uint8_t
#include <stdlib.h> // for malloc, qsort
#include <assert.h> // for assert()
// the libraries from Teem that we use in this class
#include <teem/air.h>
#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/nrrd.h>
#include <teem/unrrdu.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create typedef for "real" as either a "float" (single precision) or "double"
   (double precision) floating-point number, based on whether SCIVIS_REAL_IS_DOUBLE
   is defined at compile-time. See also NOTE at bottom of this file. */
#ifdef SCIVIS_REAL_IS_DOUBLE
typedef double real;
#else
typedef float real;
#endif

/* "uint" is easier to type than any of C99's uint8_t, uint16_t, etc., and
   there is often no need for any specific size; just an "unsigned int" */
typedef unsigned int uint;
/* "uint8_t" is easy enough to type, but the idiom of using "unsigned char"
   for an 8-bit unsigned integer is entrenched enough that "uchar" seems
   like the better abbreviation */
typedef uint8_t uchar;

/* You can ignore this: the program for grading, grade/plotr, depends on
   librplt-{osx,linux}.a (the library of all the reference code), in which
   all the symbols have been prefixed by "ref_" thanks to this header. */
#ifdef SCIVIS_MANGLE
#  include "pltMangle.h"
#endif

/* common.c: (nothing for you to implement) things that are common across projects.
   Perhaps the most important: pltVerbose is a global flag you should use to control
   whether or not your code prints debugging messages, and how verbose they are. It is
   set at the beginning of main() in plotr.c, based on the SCIVIS_VERBOSE environment
   variable. Or, by compiling with "make CFLAGS=-DNDEBUG", the same way you turn off
   assert()s, pltVerbose is #define'd as "0", so tests like "if (pltVerbose)" turn
   into "if (0)", which the compiler can then remove completely (for a potentially
   significant speed-up).

   NOTE: your code should compile and run correctly with:
   make CFLAGS=-DNDEBUG
*/
#ifndef NDEBUG
extern int pltVerbose;
#else
#  define pltVerbose 0
#endif
extern const char *pltBiffKey;
#define PLT pltBiffKey // identifies this library in biff error messages
extern const int pltRealIsDouble;
extern real pltNan(unsigned short payload);
extern unsigned short pltNanPayload(real nval);
extern void pltPrintLogStart(void);
extern const char *pltPrintLog(void);
extern void pltPrintLogFinish(void);
#ifndef SCIVIS_PRINTF_NOOP
// Stand-ins for printf, sprintf, fprintf that print reals with %g
extern int pltPrintf(const char *restrict format, ...)
  __attribute__((format(printf, 1, 2)));
extern int pltSprintf(char *restrict dest, const char *restrict format, ...)
  __attribute__((format(printf, 2, 3)));
extern int pltFprintf(FILE *file, const char *restrict format, ...)
  __attribute__((format(printf, 2, 3)));
/* for the curious: the __attribute__ annotations tell the compiler how these
   functions work like printf, which facilitates very helpful warnings that
   can flag otherwise hard-to-find bugs in the use of these functions. */
#else
/* To do thoughtfulness grading on use of reals (grade/clean impf), turn all
   these functions into no-ops, to silence warnings about the promotion of
   float to double that always happens with variable argument lists */
#  define pltPrintf(FMT, ...)     (0)
#  define pltSprintf(FMT, ...)    (0)
#  define pltFprintf(FMT, ...)    (0)
#  define biffAddf(KEY, FMT, ...) (0)
/* printf() itself is not turned into a no-up when SCIVIS_PRINTF_NOOP is
   defined; instead use pltPrintf and friends to print reals */
#endif
extern unsigned int pltFloatDist(float A, float B);
extern char *pltCommandLine(const char *me, const char **argv);
extern char *pltTodoStr(const char **strs);
extern long int pltMicrosecondsUsed(void);
extern void pltWallTime(long SecMicrosec[2]);
extern long int pltMicrosecondsWallTimeDiff(const long smsEnd[2],
                                            const long smsStart[2]);
extern void pltOutput(const char *fname);
extern int pltOutputRenamePrefix(const char *prefix);
extern void pltRoundGetenv(void);
extern int pltRoundDir(void);
extern int pltRoundBracket(void);
extern void pltRoundDirSet(int dir);
extern void pltRoundSet(void);
extern void pltRoundUnset(void);
extern uint pltSprintLenMax(uint oldmax, const real *vv, uint vvNum);

/* The pltKernel stores everything about a reconstruction kernel. The kernel
   is non-zero only within [-support/2,support/2], for integer "support"
   which may be odd or even (but always positive). The kernels are set up at
   compile-time in such a way that each kernel knows its own derivative; the
   derivative of pltKernel *k is k->deriv. */
typedef struct pltKernel_t {
    const char *name;      // short identifying string
    const char *desc;      // short descriptive string
    unsigned int support;  // # samples needed for convolution
    real (*eval)(real xx); // evaluate kernel once
    /* The "apply" function evaluates the kernel "support" times, saving the
       results into ww, as needed for one convolution (FSV section 4.2.2).
       If the kernel support is even, then xa must satisfy 0 <= xa < 1,
       and let ilo = 1 - support/2, see FSV (4.32).
       If support is odd, then xa must satisfy -0.5 <= xa < 0.5,
       and let ilo = (1 - support)/2, see FSV (4.36).
       Then, ww[i] = eval(xa-(i+ilo)) for i=0, 1, ... support-1.
       This may be faster than calling eval() "support" times. */
    void (*apply)(real *ww, real xa);
    const struct pltKernel_t *deriv; /* derivative of this kernel; will point
                                          back to itself when kernel is zero */
} pltKernel;

/* kernel.c: gives compile-time definition of some reconstruction
   kernels and their derivatives (nothing for you to implement) */
extern const pltKernel *const pltKernelZero;
extern const pltKernel *const pltKernelBox;
extern const pltKernel *const pltKernelSin;
extern const pltKernel *const pltKernelCos;
extern const pltKernel *const pltKernelTooth;
extern const pltKernel *const pltKernelTent;
extern const pltKernel *const pltKernelBspln2;
extern const pltKernel *const pltKernelBspln3;
extern const pltKernel *const pltKernelBspln4;
extern const pltKernel *const pltKernelBspln5;
extern const pltKernel *const pltKernelCtmr;
extern const pltKernel *const pltKernelSpark;
extern const pltKernel *const pltKernelLuna;
extern const pltKernel *const pltKernelCelie;
extern const pltKernel *const pltKernelAll[];
// the largest support of any of the above kernels
#define PLT_KERNEL_SUPPORT_MAX 6

/* kparse.c: for parsing kernels from strings (nothing for you to
   implement) */
extern const pltKernel *pltKernelParse(const char *kstr);
extern hestCB *pltKernelHest;

// ########################## End of things shared between projects
// ########################## Beginning of things specific to plt

// Look for "TODO"s that highlight which functions you have to finish

/* The pltCenter C enum represents the two different strategies of uniformly sampling an
   interval. pltCenter_ae is an airEnum around the same values. NOTE: with this or any
   other enum, do not ever use hard-coded constants 1 or 2 in your code because that
   would be illegible; use pltCenterNode or pltCenterCell instead. */
typedef enum {
    pltCenterUnknown = 0, // sample centering not known
    pltCenterNode,        // (1) node-centered samples
    pltCenterCell,        // (2) cell-centered samples
} pltCenter;

/* The pltMark C enum represents the different things ("marks") to be drawn in the image
   generated by pltPLot; each has its own RGB color. */
typedef enum {
    pltMarkUnknown = 0,  // mark unknown
    pltMarkBackground,   // (1) background behind all marks
    pltMarkAxes,         // (2) axes along Y=0 and X=0
    pltMarkPolynomial,   // (3) plot of an exact polynomial
    pltMarkOutsideOne,   // (4) where (on Y=0 axis) one data sample missing for convo
    pltMarkOutsideMore,  // (5) where (on Y=0 axis) > 1 data samples missing for convo
    pltMarkConvolution,  // (6) plot of the convolution result
    pltMarkDot,          // (7) dots showing underlying data samples
    pltMarkZeroCrossing, // (8) zero crossings (by lerp) between data samples
} pltMark;

/* misc.c: other miscellaneous things (nothing for you to implement). An airEnum (defined
   in teem/air.h) is a gadget that simplifies converting between integer C enum values
   and the strings (like "node" for pltCenterNode) that might represent those values
   on the command-line or in a GUI.  */
extern const airEnum *const pltCenter_ae;
extern const airEnum *const pltMark_ae;

/* The pltData struct stores len sampled data values in array vv[], and the meta-data
   about the sampling needed to unambiguously define the world-space locations of each
   sample: the interval [min,max] that was sampled (if min < max), and (center) whether
   the samples are node- or cell-centered. It is possible that min > max: "min" is
   technically the world-space end-point of the sampling interval closer to where vv[0]
   was sampled, and "max" is the end-point of the interval closer to where vv[len-1] was
   sampled. */
typedef struct {
    real *vv;         // data values
    uint len;         // length of array vv
    real min, max;    // values sampled in interval [min,max] (or [max,min])
    pltCenter center; // either pltCenterNode or pltCenterCell
} pltData;

/* data.c: functions for creating, destroying, writing, and reading the pltData
   (nothing for you to implement) */
extern pltData *pltDataNew(void);
extern pltData *pltDataNix(pltData *data);
extern int pltDataInit(pltData *data, uint len, real min, real max, pltCenter center);
extern int pltDataSave(const char *fname, const pltData *data, const char *content);
extern int pltDataLoad(pltData *data, const char *fname);

/* util.c: How to lerp, and to convert between index space and world space.
   TODO: finish implementing these three functions */
extern real pltLerp(real omin, real omax, real imin, real xx, real imax);
extern real pltItoW(const pltData *data, real indexPos);
extern real pltWtoI(const pltData *data, real worldPos);

/* poly.c: evaluating and sampling a polynomial.
   TODO: you will finish implementing these two functions */
extern real pltPolyEval(const real *pc, uint pclen, real xx);
extern int pltPolySample(pltData *data, uint len, real imin, real imax, pltCenter center,
                         const real *pc, uint pclen);

/* convo.c: evaluating convolution at one location or at array of locations.
   TODO: finish pltConvoEval */
extern int pltConvoEval(real *result, unsigned short *outside, real xx, uint whichD,
                        const pltData *data, const pltKernel *kern);
extern int pltConvoSample(pltData *odata, uint olen, real omin, real omax,
                          pltCenter center, uint whichD, const pltData *din,
                          const pltKernel *kern);

/* pplot.c: for doing the final graph drawing as a raster image.
   TODO: finish pltPlot */
extern int pltPlot(unsigned char *const rgb, uint xsize, uint ysize, real xmin,
                   real xmax, real ymin, real ymax, uint whichD, const pltData *idata,
                   const pltKernel *kern, const real *pc, uint pclen,
                   real thicknessConvo, real thicknessPoly, int apcoth,
                   real thicknessAxes, real dotRadius, real heightZC);

// ########################## End of things specific to plt
// ########################## Beginning of things shared between projects

/*
  The "plotr" executable is compiled from plotr.c, which relies on the various plt_XCmd
  objects (each defined in plotr_X.c) for the plotr commands X={about, klist, ...}.  None
  of those objects need declaring here, because plotr is the only thing using them, and
  plotr.c generates (via C pre-processor tricks) its own extern declarations of them.

  You should run "rplotr" to see all the commands, then run "rplotr about" to learn more
  about what work is required to finish this project (or just "rplotr todo" to see the
  basic list). If you're curious about how "plotr X" is implemented, you can look at
  its source plotr_X.c. This will also show you how the plt library functions above are
  called, and for some commands, how they are graded.
*/

/*
  More on SCIVIS_REAL_IS_DOUBLE:

  You can choose the meaning of "real" at compile time, in order to experiment with the
  performance/accuracy trade-off associated with single- versus double-precision floating
  point arithmetic. This is enabled by the inclusion in pltPrivate.h of <tgmath.h>.
  tgmath.h, which was introduced in C99, and is an incredible hack (see
  http://goo.gl/dEQE4R), but it succeeds in making the math functions behave more like
  operators, i.e., "cos(x)" becomes cosf(x) for float x and cos(x) for double x (see "man
  cos"), as well as handling the complex-valued functions (see "man ccos"), though we
  won't be coding with complex values in this class). For us, tgmath.h greatly
  facilitates writing code that is generic w.r.t. changes in floating point
  precision.

  To make "real" mean "double", compile with:

    make clean; make CFLAGS=-DSCIVIS_REAL_IS_DOUBLE

  otherwise (with regular "make") "real" means "float".

  NOTE: Your code should compile and work with real either as float or double, and with
  real as double, it should give results that are at least as accurate (if not much
  better) than with real as float.

  NOTE: Be sure to "make clean" if you want to change the meaning of real, since "make"
  only acts on *when* things were compiled, not *how*. You will get very confusing
  crashes if some .o files were compiled with a different meaning of "real" than others.
*/

#ifdef __cplusplus
}
#endif
#endif // PLT_HAS_BEEN_INCLUDED
