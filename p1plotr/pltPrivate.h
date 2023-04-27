/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

/* This file is #included in all the source files that are compiled into the
   plt library, because it #includes or declares things relevant to
   compiling all the plt source files (i.e., how plt is implemented), but
   not relevant for what the plt library provides. The external or "public'
   API to plt is defined in plt.h

   The following four #defines are probably what you'll be seeing most from this
   header file */

/* utility macro for malloc() (memory allocation on the heap) for N things of type T, and
   casting to T* type */
#define MALLOC(N, T) (T *)(malloc((N) * sizeof(T)))

/* USED and USEd are macros for suppressing compiler "unused parameter" warnings about
   parameters that haven't *yet* been used in your code. Uses C99 compound literals in a
   variadic macro to do something inconsequential with the arguments, so that one macro
   invocation can handle multiple parameters (which is nice when clang-format wants to
   put all these on separate lines). Unfortunately with this strategy, as far as GLK
   knows, we need two different macros: one (USED) for non-pointers, and another (USEd)
   for pointers; please excuse the cutesy/weird/illogical naming of "USEd".
 */
#define USED(...) (void)sizeof((int[]){__VA_ARGS__})
#define USEd(...) (void)sizeof((const void *[]){__VA_ARGS__})

/* M_PI was dropped from the C99 standard, but it might still be available on some
  compilers; we undef and redefine for consistency */
#undef M_PI
#define M_PI ((real)3.14159265358979323846)

/* Read through the rest of this if you're curious, but understanding it is not essential
   for completing class programming work */

/* RCS is the "conversion sequence" to use to accurately print a real via printf; can be
   used with string literal concatenation.  RCD is just the digits of RCS. Assuming you
   use pltPrintf (which you should), you don't need to worry about this. */
#if SCIVIS_REAL_IS_DOUBLE
#  define RCD "17"
#else
#  define RCD "9"
#endif
#define RCS "%." RCD "g" // relying on C's string literal concatenation

/* The things #define'd here according to SCIVIS_REAL_IS_DOUBLE simplify connecting to
   other external libraries that do not know about our choice of "real", but which can
   handle either float or double. */
#if SCIVIS_REAL_IS_DOUBLE
#  define airTypeReal  airTypeDouble
#  define nrrdTypeReal nrrdTypeDouble
#else
#  define airTypeReal  airTypeFloat
#  define nrrdTypeReal nrrdTypeFloat
#endif

#include <assert.h> // for assert()
#include <wchar.h>  // for wint_t
#include <tgmath.h> // type-generic math macros
/* <tgmath.h> #includes <complex.h>, which in turn defines a macro "I", which removes "I"
   from your possible variable names (using it creates cryptic compiler error messages).
   We're allowed to undefine "I", so we do. */
#undef I

// for monitoring how many times kernels are evaluated
#define KEC_DECL                                                                        \
  extern uint pltKernelEvalCount[8];                                                    \
  extern uint pltKernelApplyCount[8]
#define KEC_PRINT                                                                       \
  {                                                                                     \
    char *_line = pltCommandLine(me, argv);                                             \
    printf("KEC: %s\n", _line);                                                         \
    free(_line);                                                                        \
    printf("KEC:");                                                                     \
    for (uint ci = 0; ci < 8; ci++)                                                     \
      printf(" %u", pltKernelEvalCount[ci]);                                            \
    printf(" =pltKernelEvalCount\nKEC:");                                               \
    for (uint ci = 0; ci < 8; ci++)                                                     \
      printf(" %u", pltKernelApplyCount[ci]);                                           \
    printf(" =pltKernelApplyCount\n");                                                  \
  }

/* ERROR is an ugly macro for doing cleanup with error handling inside the implementation
   of the plotr commands. It's ugly because it presumes a variable "mop", and because it
   "return"s, which is bad form for a macro */
#define ERROR(KEY, WHAT)                                                                \
  {                                                                                     \
    char *_err;                                                                         \
    airMopAdd(mop, _err = biffGetDone((KEY)), airFree, airMopAlways);                   \
    fprintf(stderr, "%s: " WHAT ":\n%s", me, _err);                                     \
    airMopError(mop);                                                                   \
  }                                                                                     \
  return 1

// removing Teem macros we don't need or can improve on
#undef AIR_EXISTS
#undef AIR_EXISTS_F
#undef AIR_EXISTS_D
#undef AIR_ISNAN_F
#undef AIR_MAX
#undef AIR_MIN
#undef AIR_ABS
#undef AIR_IN_OP
#undef AIR_IN_CL
#undef AIR_CLAMP
#undef AIR_MOD
#undef AIR_LERP
#undef AIR_AFFINE
#undef AIR_DELTA
#undef AIR_ROUNDUP
#undef AIR_ROUNDDOWN
#undef AIR_ROUNDUP_UI
#undef AIR_ROUNDDOWN_UI
