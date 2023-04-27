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

#define BUFFSIZE 256

static void
nameGet(char *buff, const pltKernel *kern) {
    airStrcpy(buff, BUFFSIZE, kern->name);
    airToLower(buff);
}

const pltKernel *
pltKernelParse(const char *_kstr) {
    char kstr[BUFFSIZE];
    airStrcpy(kstr, BUFFSIZE, _kstr);
    airToLower(kstr);
    const pltKernel *ret = NULL;
    for (uint kidx = 0; pltKernelAll[kidx]; kidx++) {
        char kname[BUFFSIZE];
        nameGet(kname, pltKernelAll[kidx]);
        if (!strcmp(kname, kstr)) {
            ret = pltKernelAll[kidx];
            break;
        }
        const pltKernel *dkern = pltKernelAll[kidx]->deriv;
        /* would like to test for being the zero kernel, but there are
           multiple zero kernels (one for each support); so, we see if
           the derivative is the same as us; that implies it is zero */
        while (dkern->deriv != dkern) {
            nameGet(kname, dkern);
            if (!strcmp(kname, kstr)) {
                ret = dkern;
                break;
            }
            dkern = dkern->deriv;
        }
    }
    return ret;
}

static int
_pltKernelHestParse(void *ptr, const char *str, char err[AIR_STRLEN_HUGE]) {
    if (!(ptr && str)) {
        sprintf(err, "%s: got NULL pointer", __func__);
        return 1;
    }
    const pltKernel **kernP = (const pltKernel **)ptr;
    if (!(*kernP = pltKernelParse(str))) {
        sprintf(err, "%s: \"%s\" couldn't be parsed as a kernel", __func__, str);
        return 1;
    }
    return 0;
}

static hestCB _pltKernelHest = {sizeof(pltKernel *), //
                                "kernel",            //
                                _pltKernelHestParse, //
                                NULL};

hestCB *pltKernelHest = &_pltKernelHest;
