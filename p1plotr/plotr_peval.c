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

#define INFO "Evaluate a polynomial at a single location"
static char *infoLong = INFO ". The main work is done with pltPolyEval() in poly.c; "
                             "use this command to debug your implementation.";

static int
pevalMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    real *pc;
    uint pclen;
    hestOptAdd(&hopt, "p", "p0 p1", airTypeReal, 1, -1, &pc, NULL,
               "coefficients of polynomial to sample, starting with "
               "degree 0, e.g. \"-p 1 2 3\" means 1 + 2x + 3x^2",
               &pclen);
    real *xx;
    uint xxlen;
    hestOptAdd(&hopt, "x", "x0", airTypeReal, 1, -1, &xx, NULL,
               "position(s) at which to evaluate polynomial", &xxlen);
    hestParseOrDie(hopt, argc, argv, hparm, me, infoLong, AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

    if (1 == xxlen) {
        // evaluating polynomial at one location
        // . . . . . . . . . . . . . . . . . useful code here
        pltRoundSet();
        real yy = pltPolyEval(pc, pclen, xx[0]);
#ifdef SCIVIS_GRADE
        real ryy = ref_pltPolyEval(pc, pclen, xx[0]);
#endif
        pltRoundUnset();
        // ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' useful code here
#ifdef SCIVIS_GRADE
        pltPrintf("%s: trying pltPolyEval for f(%g) = ", me, xx[0]);
#else
        pltPrintf("%g = f(%g) = ", yy, xx[0]);
#endif
        pltPrintf("%g", pc[0]);
        for (uint pi = 1; pi < pclen; pi++) {
            if (pi > 1) {
                pltPrintf(" + %g*(%g)^%u", pc[pi], xx[0], pi);
            } else {
                pltPrintf(" + %g*(%g)", pc[pi], xx[0]);
            }
        }
        printf("\n");
#ifdef SCIVIS_GRADE
        pltPrintf("%s: you: %g\n", me, yy);
        pltPrintf("%s: ref: %g\n", me, ryy);
        pltGradeReal(yy, ryy, "polynomial evaluation");
#endif
    } else {
        /* xxlen >= 1; evaluating polynomial at multiple locations,
           which was implemented for the sake of grading */
        double *yy = MALLOC(xxlen, double);
        assert(yy);
        airMopAdd(mop, yy, airFree, airMopAlways);
        double mean = 0, stdv = 0;
#ifdef SCIVIS_GRADE
        double *ryy = MALLOC(xxlen, double);
        assert(ryy);
        airMopAdd(mop, ryy, airFree, airMopAlways);
        double rmean = 0, rstdv = 0;
#endif
        for (uint ii = 0; ii < xxlen; ii++) {
            pltRoundSet();
            yy[ii] = (double)pltPolyEval(pc, pclen, xx[ii]);
#ifdef SCIVIS_GRADE
            ryy[ii] = ref_pltPolyEval(pc, pclen, xx[ii]);
#endif
            pltRoundUnset();
            mean += yy[ii];
#ifdef SCIVIS_GRADE
            rmean += ryy[ii];
#endif
        }
        mean /= xxlen;
#ifdef SCIVIS_GRADE
        rmean /= xxlen;
#endif
        for (uint ii = 0; ii < xxlen; ii++) {
            stdv += (yy[ii] - mean) * (yy[ii] - mean);
#ifdef SCIVIS_GRADE
            rstdv += (ryy[ii] - rmean) * (ryy[ii] - rmean);
#endif
        }
        stdv = sqrt(stdv / xxlen);
#ifndef SCIVIS_GRADE
        pltPrintf("%g = mean of evaluations\n", mean);
        pltPrintf("%g = stdv of evaluations\n", stdv);
#else
        rstdv = sqrt(rstdv / xxlen);
        pltPrintf("%s: you: %g = mean of evaluations\n", me, mean);
        pltPrintf("%s: ref: %g = mean of evaluations\n", me, rmean);
        pltPrintf("%s: you: %g = stdv of evaluations\n", me, stdv);
        pltPrintf("%s: ref: %g = stdv of evaluations\n", me, rstdv);
        pltGradeTuple((pltGradeItem[]){(pltGradeItem){"mean", mean, rmean, 1},
                                       (pltGradeItem){"stdv", stdv, rstdv, 0.1}},
                      2, 0, 0);
#endif
    }

    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_pevalCmd = {"peval", INFO, pevalMain, AIR_FALSE};
