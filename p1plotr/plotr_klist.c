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

#define INFO "List all available kernels"

static int
klistMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    USED(argc);
    USEd(argv, me);
    static const char info[]
      = "\nBelow is a list of all kernels currently available. "
        "First is the (case insensitive) string identifying the kernel, "
        "followed by the integer support, and then "
        "a short description. Each of the reconstruction kernels, marked by "
        "\"===\", is followed by a list of its derivatives (which have the "
        "same support as the parent reconstruction kernel).\n";

    _hestPrintStr(stdout, 0, 0, hparm->columns, info, AIR_FALSE);

    uint kidx = 0;
    const pltKernel *kern = pltKernelAll[kidx];
    do {
        printf("=== %10s (support %u): %s\n", kern->name, kern->support, kern->desc);
        const pltKernel *dk = kern->deriv;
        while (dk->deriv != dk) {
            printf("   %20s: %s\n", dk->name, dk->desc);
            dk = dk->deriv;
        }
        kern = pltKernelAll[++kidx];
    } while (kern);

    return 0;
}

unrrduCmd plt_klistCmd = {"klist", INFO, klistMain, AIR_FALSE};
