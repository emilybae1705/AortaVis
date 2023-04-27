/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

// All functions defined in plotr.c

// Not for timing execution, but limiting execution time
extern real pltTimeout(void);
extern void pltTimeoutStart(void);
extern void pltTimeoutStop(void);

// Functions and macros used for timing execution
extern uint pltTimeDo(void);
extern int pltTimeVerbose(void);
extern uint pltTimeMedian(void);
extern uint pltTimeExtra(void);
extern real pltTimeSpread(void);
extern uint pltTimeInit(void);
extern long pltTimeAdd(long newtime);

#define TIME_EXTRA_LIMIT 100

// for timing cpu usage
#define _UTIME0_DECL(PFX) long PFX##time0 = 0
#define _UTIME0_SET(PFX)  PFX##time0 = pltMicrosecondsUsed()
#define _UTIME1_SET(PFX)  long PFX##time1 = pltMicrosecondsUsed()
#define _UTIME_DIFF(PFX)  (PFX##time1 - PFX##time0)

// for timing w.r.t wall time
#define _WTIME0_DECL(PFX) long PFX##time0[2] = {0, 0}
#define _WTIME0_SET(PFX)  pltWallTime(PFX##time0)
#define _WTIME1_SET(PFX)                                                                \
  long PFX##time1[2];                                                                   \
  pltWallTime(PFX##time1)
#define _WTIME_DIFF(PFX) pltMicrosecondsWallTimeDiff(PFX##time1, PFX##time0)

#define _TIME_START(PFX, D0, S0)                                                        \
  D0(PFX);                                                                              \
  uint PFX##GotoCount = 0;                                                              \
  if (pltTimeDo()) {                                                                    \
    pltTimeInit();                                                                      \
    PFX##start : pltTimeoutStart();                                                     \
    S0(PFX);                                                                            \
  } else {                                                                              \
    pltTimeoutStart();                                                                  \
  }

#define TIME_START(PFX)  _TIME_START(PFX, _UTIME0_DECL, _UTIME0_SET)
#define WTIME_START(PFX) _TIME_START(PFX, _WTIME0_DECL, _WTIME0_SET)

#define _TIME_END(OUT, PFX, S1, DD)                                                     \
  long OUT = 0;                                                                         \
  if (pltTimeDo()) {                                                                    \
    S1(PFX);                                                                            \
    pltTimeoutStop();                                                                   \
    OUT = pltTimeAdd(DD(PFX));                                                          \
    if (!OUT) {                                                                         \
      PFX##GotoCount++;                                                                 \
      if (PFX##GotoCount > pltTimeMedian() + TIME_EXTRA_LIMIT * pltTimeExtra()) {       \
        printf("\n\n");                                                                 \
        printf("%s(TIME_END): timing not converged "                                    \
               "after %u > %u + %u*%u = %u iters. "                                     \
               "SORRY, quitting.\n",                                                    \
               __func__, PFX##GotoCount, pltTimeMedian(), TIME_EXTRA_LIMIT,             \
               pltTimeExtra(), pltTimeMedian() + TIME_EXTRA_LIMIT * pltTimeExtra());    \
        printf("\n\n");                                                                 \
        exit(1);                                                                        \
      } else {                                                                          \
        goto PFX##start;                                                                \
      }                                                                                 \
    }                                                                                   \
    if (pltTimeVerbose()) {                                                             \
      pltPrintf("%s: timing = %g secs\n", me, (real)OUT / 1000000);                     \
    }                                                                                   \
  } else {                                                                              \
    pltTimeoutStop();                                                                   \
  }

#define TIME_END(OUT, PFX)  _TIME_END(OUT, PFX, _UTIME1_SET, _UTIME_DIFF)
#define WTIME_END(OUT, PFX) _TIME_END(OUT, PFX, _WTIME1_SET, _WTIME_DIFF)
