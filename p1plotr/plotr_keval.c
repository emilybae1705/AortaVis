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

#define INFO "Evaluate a kernel (and its derivatives) at some location"
static char *infoLong = (INFO
                         ". This is not actually required to do your work, "
                         "but having it may be helpful for debugging your own "
                         "convolution code. The string used to identify the kernel is "
                         "case-insensitive.");

static int
kevalMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    int supportDo;
    hestOptAdd(&hopt, "s", NULL, airTypeInt, 0, 0, &supportDo, NULL,
               "also print the kernel support");
    uint derivs;
    hestOptAdd(&hopt, "d", "derivs", airTypeUInt, 1, 1, &derivs, "0",
               "number of derivatives of kernel to also evaluate");
    real eps;
    hestOptAdd(&hopt, "e", "epsilon", airTypeReal, 1, 1, &eps, "0",
               "(This is only used to debug the kernels themselves, i.e., "
               "not something students have to worry about) "
               "If non-zero, and if also evaluating kernel derivatives "
               "(via \"-d\"): the size of central difference to compute "
               "to numerically estimate kernel derivative.");
    pltKernel *kern;
    hestOptAdd(&hopt, NULL, "kernel", airTypeOther, 1, 1, &kern, NULL,
               "kernel to evaluate", NULL, NULL, pltKernelHest);
    real xx;
    hestOptAdd(&hopt, NULL, "position", airTypeReal, 1, 1, &xx, NULL,
               "position at which to evaluate kernel");
    real xa;
    hestOptAdd(&hopt, "a", "alpha", airTypeReal, 1, 1, &xa, "nan",
               "if used, position given previously is moot, and instead "
               "the kernel is evaluated once per segment in support, at "
               "offset alpha from next-lower integer index position");
    hestParseOrDie(hopt, argc, argv, hparm, me, infoLong, AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

    // . . . . . . . . . . . . . . . . . useful code here
    uint ksup = kern->support;
#define SET_NAN(w)                                                                      \
  for (uint wi = 0; wi < ksup; wi++)                                                    \
    (w)[wi] = pltNan(0);
    if (supportDo) {
        printf("support(%s) = %u\n", kern->name, ksup);
    }
    /* using _lit_Printf instead of printf means that "%g" expands to either
       "%.9g" or "%.17g" depending on real==float or double, so that these
       values are printed with reproducible precision */
    if (isfinite(xa)) {
        // testing kern->apply
        printf("for each interval segment A == B shows A=apply "
               "and B=eval results\n");
        if (eps > 0) {
            printf(" and ~= D ~= E shows numerical derivative D "
                   "of apply and E of eval\n");
        }
        pltPrintf("%s->apply(ww,%g): \n", kern->name, xa);
        real ww[PLT_KERNEL_SUPPORT_MAX], dw[PLT_KERNEL_SUPPORT_MAX];
        int ilo, ihi;
        if (ksup % 2) {
            // odd;  FSV 4.36
            ilo = (1 - (int)ksup) / 2;
            ihi = -ilo;
        } else {
            // even; FSV 4.32
            ilo = 1 - (int)ksup / 2;
            ihi = (int)ksup / 2;
        }
        SET_NAN(ww);
        kern->apply(ww, xa);
        for (int ii = ilo; ii <= ihi; ii++) {
            uint wi = (uint)(ii - ilo); // wi + ilo == ii
            // using this to check: ww[wi] == eval(xa-ii) == eval(xa-(wi+ilo))
            pltPrintf("ww[%u]= %g == %g = %s(%g)", wi, ww[wi], kern->eval(xa - ii),
                      kern->name, xa - ii);
            if (derivs) {
                const pltKernel *dkern = kern;
                for (uint didx = 1; didx <= derivs; didx++) {
                    const pltKernel *dd = dkern;
                    dkern = dkern->deriv;
                    SET_NAN(dw);
                    dkern->apply(dw, xa); // yes, redundant
                    pltPrintf("\nd%u      %g == %g", didx, dw[wi], dkern->eval(xa - ii));
                    if (eps > 0) {
                        for (uint wi = 0; wi < ksup; wi++)
                            dw[wi] = pltNan(0);
                        SET_NAN(dw);
                        dd->apply(dw, xa + eps);
                        real wp = dw[wi];
                        for (uint wi = 0; wi < ksup; wi++)
                            dw[wi] = pltNan(0);
                        SET_NAN(dw);
                        dd->apply(dw, xa - eps);
                        real wm = dw[wi];
                        pltPrintf(" ~= %g ~= %g", ((wp - wm) / (2 * eps)),
                                  ((dd->eval(xa - ii + eps) - dd->eval(xa - ii - eps))
                                   / (2 * eps)));
                    }
                }
            }
            printf("\n");
        }
    } else {
        // testing regular kern->eval
        pltPrintf("%s(%g)= %g\n", kern->name, xx, kern->eval(xx));
        if (derivs) {
            const pltKernel *dkern = kern;
            for (uint didx = 1; didx <= derivs; didx++) {
                const pltKernel *dd = dkern;
                dkern = dkern->deriv;
                pltPrintf("%s(%g)= %g", dkern->name, xx, dkern->eval(xx));
                if (eps > 0) {
                    pltPrintf(" ~= %g\n",
                              ((dd->eval(xx + eps / 2) - dd->eval(xx - eps / 2)) / eps));
                } else {
                    printf("\n");
                }
            }
        }
    }
    // ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' useful code here

    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_kevalCmd = {"keval", INFO, kevalMain, AIR_FALSE};
