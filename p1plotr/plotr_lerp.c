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
#ifdef SCIVIS_GRADE
#  include "rplt.h"
#  include "pltGrade.h"
#endif

#define INFO "Linear interpolation with lerp()"
static char *infoLong = INFO ". The main work is done with pltLerp() in util.c; "
                             "use this command to debug your implementation.";

static int
lerpMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    real y0;
    hestOptAdd(&hopt, NULL, "y0", airTypeReal, 1, 1, &y0, NULL,
               "first end-point of output interval");
    real y1;
    hestOptAdd(&hopt, NULL, "y1", airTypeReal, 1, 1, &y1, NULL,
               "second end-point of output interval");
    real x0;
    hestOptAdd(&hopt, NULL, "x0", airTypeReal, 1, 1, &x0, NULL,
               "first end-point of input interval");
    real xx;
    hestOptAdd(&hopt, NULL, "x", airTypeReal, 1, 1, &xx, NULL, "input value");
    real x1;
    hestOptAdd(&hopt, NULL, "x1", airTypeReal, 1, 1, &x1, NULL,
               "second end-point of input interval");
    hestParseOrDie(hopt, argc, argv, hparm, me, infoLong, AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

#ifdef SCIVIS_GRADE
    pltPrintf("%s: calling pltLerp(%g, %g, %g, %g, %g)\n", me, y0, y1, x0, xx, x1);
#endif
    // . . . . . . . . . . . . . . . . . useful code here
    pltRoundSet();
    real yy = pltLerp(y0, y1, x0, xx, x1);
    pltRoundUnset();
    // ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' useful code here

#ifdef SCIVIS_GRADE
    pltRoundSet();
    real ryy = ref_pltLerp(y0, y1, x0, xx, x1);
    pltRoundUnset();
    pltPrintf("%s: you: %g\n", me, yy);
    pltPrintf("%s: ref: %g\n", me, ryy);
    pltGradeReal(yy, ryy, "lerp output");
#else
    pltPrintf("%g = pltLerp(%g, %g, %g, %g, %g)\n", yy, y0, y1, x0, xx, x1);
#endif

    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_lerpCmd = {"lerp", INFO, lerpMain, AIR_FALSE};
