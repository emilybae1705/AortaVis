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
#include "pltTime.h"

#define INFO "Repeatedly evaluate (sample) a polynomial over an interval"
static char *infoLong = INFO
  ". The main work is done with pltPolySample(); use this command "
  "to debug your implementation. If the given output (\"-o\") filename ends "
  "with \".txt\", the values are saved, one per line, to a plain text file. "
  "The \"#\" lines at the start of the text file document the meta-data "
  "required to locate the samples in some world-space interval. If the "
  "output filename ends with .nrrd, the NRRD file format is used.";

static int
psampMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    real *pc;
    uint pclen;
    hestOptAdd(&hopt, "p", "p0 p1", airTypeReal, 1, -1, &pc, NULL,
               "coefficients of polynomial to sample, starting with "
               "degree 0, e.g. \"-p 1 2 3\" means 1 + 2x + 3x^2",
               &pclen);
    real mm[2];
    hestOptAdd(&hopt, "mm", "min max", airTypeReal, 2, 2, mm, NULL,
               "min and max bounds over which to do sampling of polynomial");
    uint olen;
    hestOptAdd(&hopt, "n", "#samples", airTypeUInt, 1, 1, &olen, NULL,
               "number of samples to take over -mm interval");
    pltCenter center;
    hestOptAdd(&hopt, "c", "center", airTypeEnum, 1, 1, &center, "cell",
               "whether to sample -mm interval with cell-centered "
               "(\"-c cell\") or node-centered (\"-c node\") samples",
               NULL, pltCenter_ae);
    char *oname;
    hestOptAdd(&hopt, "o", "output", airTypeString, 1, 1, &oname, NULL,
               "output file, ending either with \".txt\" or \".nrrd\"");
    hestParseOrDie(hopt, argc, argv, hparm, me, infoLong, AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

#ifndef SCIVIS_GRADE
    // . . . . . . . . . . . . . . . . . useful code here
    pltData *odata = pltDataNew();
    airMopAdd(mop, odata, (airMopper)pltDataNix, airMopAlways);
    char *cmdl = pltCommandLine(me, argv);
    airMopAdd(mop, cmdl, airFree, airMopAlways);
    /* the (foo(), 0) idiom is for when a function foo() returns nothing, but you want to
       put in a short-circuit chain that has to stop as soon as there's an error. In C
       you can connect a sequence of of expressions with commas, and the last expression
       is the value of the whole sequence. */
    if ((pltRoundSet(), 0)                                             //
        || pltPolySample(odata, olen, mm[0], mm[1], center, pc, pclen) //
        || (pltRoundUnset(), 0) ||                                     //
        pltDataSave(oname, odata, cmdl)) {
        ERROR(PLT, "trouble computing or saving output");
    }
    pltOutput(oname);
    // ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' useful code here
#else
    pltData *sdata = pltDataNew();
    airMopAdd(mop, sdata, (airMopper)pltDataNix, airMopAlways);
    pltData *rdata = pltDataNew();
    airMopAdd(mop, rdata, (airMopper)pltDataNix, airMopAlways);
    int E = 0;
    pltRoundSet();
    TIME_START(stu);
    if (!E) E |= pltPolySample(sdata, olen, mm[0], mm[1], center, pc, pclen);
    TIME_END(stutime, stu);
    TIME_START(ref);
    if (!E) E |= ref_pltPolySample(rdata, olen, mm[0], mm[1], center, pc, pclen);
    TIME_END(reftime, ref);
    pltRoundUnset();
    if (E) {
        ERROR(PLT, "trouble computing student or ref output");
    }
    /* the meta-data in sdata and rdata should be the same, but it is possible that
       student's code altered something; it will be really annoying if lengths differ, so
       bail if that happened */
    if (sdata->len != rdata->len) {
        fprintf(stderr,
                "%s: your data->len %u != ref data->len %u; "
                "stopping\n",
                me, sdata->len, rdata->len);
        fflush(stderr);
        airMopError(mop);
        exit(1);
    }
    /* 0 min, 1 max, 2 center, 3 ... values */
    pltGradeItem *itm = MALLOC(3 + rdata->len, pltGradeItem);
    assert(itm);
    airMopAdd(mop, itm, airFree, airMopAlways);
    pltGradeItemSet(itm + 0, "data->min", sdata->min, rdata->min, 1);
    pltGradeItemSet(itm + 1, "data->max", sdata->max, rdata->max, 1);
    pltGradeItemSet(itm + 2, "data->center", sdata->center, rdata->center, 1);
    real dsum = 0;
    for (uint ii = 0; ii < rdata->len; ii++) {
        char desc[512];
        sprintf(desc, "vv[%u]", ii);
        pltGradeItemSet(itm + 3 + ii, desc, sdata->vv[ii], rdata->vv[ii], 1);
        dsum += fabs(sdata->vv[ii] - rdata->vv[ii]);
    }
    pltPrintf("%s: %g = L1 distance between your and ref values "
              "(should be ~= 0)\n",
              me, dsum);
    pltGradeTuple(itm, 3 + rdata->len, stutime, reftime);
#endif

    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_psampCmd = {"psamp", INFO, psampMain, AIR_FALSE};
