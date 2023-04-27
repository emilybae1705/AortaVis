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

#define INFO "Make a (discrete) delta function test dataset"
static char *infoLong = INFO
  ". This is like \"plotr psamp\" but simpler: the thing being "
  "sampled isn't a polynomial, but a function that will return "
  "1 for one sample nearest the origin, and 0 for the rest. This is a "
  "useful control test for convolution, because it allows you to isolate "
  "and plot the reconstruction kernel. Implemented with pltWtoI, "
  "so this won't work until that is finished (see \"plotr about\").";

static int
deltaMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    float mm[2];
    hestOptAdd(&hopt, "mm", "min max", airTypeFloat, 2, 2, mm, NULL,
               "min and max bounds over which to do sampling of polynomial; "
               "should probably be centered around 0");
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

    // . . . . . . . . . . . . . . . . . useful code here
    pltData *odata = pltDataNew();
    if (pltDataInit(odata, olen, mm[0], mm[1], center)) {
        ERROR(PLT, "trouble initializing output");
    }
    for (uint ii = 0; ii < olen; ii++) {
        odata->vv[ii] = 0;
    }
    int zi = (int)roundf(pltWtoI(odata, 0));
    zi = zi < 0 ? 0 : (zi > (int)olen - 1 ? (int)olen - 1 : zi);
    odata->vv[zi] = 1;
    char *cmdl = pltCommandLine(me, argv);
    airMopAdd(mop, cmdl, airFree, airMopAlways);
    if (pltDataSave(oname, odata, cmdl)) {
        ERROR(PLT, "trouble saving output");
    }
    pltOutput(oname);
    // ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' useful code here

    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_deltaCmd = {"delta", INFO, deltaMain, AIR_FALSE};
