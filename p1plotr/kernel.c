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

/*
Students are welcome to look at this code, but understanding it (e.g. how it constructs
at compile-time a linked-list of derivatives) is not important for doing your work. The
expressions for kernel evaluation all came from GLK's Mathematica notebooks.
*/

/* This monitoring facility is not generally useful, so we don't bother declaring it in
   plt.h; a per-source-file extern declaration suffices.  These arrays store counts of
   how many times kernels are evaluated, to make sure your code isn't doing anything too
   crazy. */
#ifdef KEC
uint pltKernelEvalCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint pltKernelApplyCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif

/* clang-format off */
static real ZeroEval(real xx) {
    USED(xx);
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    return 0;
}
static void Zero1Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = 0; }
static void Zero2Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = ww[1] = 0; }
static void Zero3Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = ww[1] = ww[2] = 0; }
static void Zero4Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = ww[1] = ww[2] = ww[3] = 0; }
static void Zero5Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = ww[1] = ww[2] = ww[3] = ww[4] = 0; }
static void Zero6Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = ww[1] = ww[2] = ww[3] = ww[4] = ww[5] = 0; }

static pltKernel Zero1Kernel = {
    "Zero", "Returns zero everywhere (\"support\" 1)",  1,
    ZeroEval, Zero1Apply, &Zero1Kernel
};
const pltKernel *const pltKernelZero = &Zero1Kernel;

static pltKernel Zero2Kernel = {
    "Zero", "Returns zero everywhere (\"support\" 2)",  2,
    ZeroEval, Zero2Apply, &Zero2Kernel
};

static pltKernel Zero3Kernel = {
    "Zero", "Returns zero everywhere (\"support\" 3)",  3,
    ZeroEval, Zero3Apply, &Zero3Kernel
};

static pltKernel Zero4Kernel = {
    "Zero", "Returns zero everywhere (\"support\" 4)",  4,
    ZeroEval, Zero4Apply, &Zero4Kernel
};

static pltKernel Zero5Kernel = {
    "Zero", "Returns zero everywhere (\"support\" 5)",  5,
    ZeroEval, Zero5Apply, &Zero5Kernel
};

static pltKernel Zero6Kernel = {
    "Zero", "Returns zero everywhere (\"support\" 6)",  6,
    ZeroEval, Zero6Apply, &Zero6Kernel
};

#define _KDEF(NAME, DESC, SUPP, DERIV)                                  \
    static const pltKernel NAME##Kernel = { #NAME, DESC, SUPP,        \
                                              NAME##Eval,  NAME##Apply, \
                                              &(DERIV##Kernel) }
#define KDEF(NAME, DESC, SUPP, DERIV)                                   \
    _KDEF(NAME, DESC, SUPP, DERIV);                                     \
    const pltKernel *const pltKernel##NAME = &(NAME##Kernel)

static real BoxEval(real xx) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    return (xx < -(real)1/2
            ? 0
            : (xx < (real)1/2
               ? 1
               : 0));
}
static void BoxApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    USED(xa); ww[0] = 1; }
KDEF(Box, "For nearest-neighbor interpolation", 1, Zero1);

static real dTentEval(real xx) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    return (xx < -1
            ? 0
            : (xx < 0
               ? 1
               : (xx < 1
                  ? -1
                  : 0)));
}
static void dTentApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    USED(xa); ww[0] = -1; ww[1] = 1; }
_KDEF(dTent, "1st deriv of Tent", 2, Zero2);

static real TentEval(real xx) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    return (xx < -1
            ? 0
            : (xx < 0
               ? xx + 1
               : (xx < 1
                  ? 1 - xx
                  : 0)));
}
static void TentApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = 1-xa; ww[1] = xa; }
KDEF(Tent, "For linear interpolation", 2, dTent);

static real ddBspln2Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = (real)-2;
    } else if (x < (real)3/2) {
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddBspln2Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0] = 1;
    ww[1] = -2;
    ww[2] = 1;
}
_KDEF(ddBspln2, "2nd deriv of quadratic B-spline", 3, Zero3);

static real dBspln2Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < (real)1/2) {
        ret = -x*2;
    } else if (x < (real)3/2) {
        x -= (real)1/2;
        ret = x - 1;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dBspln2Apply(real *ww, real _x) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    { real x=_x+(real)1/2; ww[0] = x - 1; }
    { real x=_x;           ww[1] = -x*2; }
    { real x=(real)1/2-_x; ww[2] = -(x - 1); }
}
_KDEF(dBspln2, "1st deriv of quadratic B-spline", 3, ddBspln2);

static real Bspln2Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = (real)3/4 - x*x;
    } else if (x < (real)3/2) {
        x -= 1;
        ret = (real)1/8 + x*(-(real)1/2 + x/2);
    } else {
        ret = 0;
    }
    return ret;
}
static void Bspln2Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = (real)1/8 + xa*(-(real)1/2 + xa/2);
    ww[1] = (real)3/4 - xa*xa;
    ww[2] = (real)1/8 + xa*((real)1/2 + xa/2);
}
KDEF(Bspln2, "quadratic B-spline", 3, dBspln2);

static real ToothEval(real xx) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    return (xx < -1
            ? 0
            : (xx < 1
               ? -xx
               : 0));
}
static void ToothApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = -xa; ww[1] = -xa+1;
}
KDEF(Tooth, "One tooth of a sawtooth", 2, Zero2);

// ================================== Ctmr
// ================================== Ctmr
// ================================== Ctmr

static real dddCtmrEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = 9;
    } else if (x < 2) {
        ret = -3;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddCtmrApply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    ww[0] = -3; ww[1] = 9; ww[2] = -9; ww[3] = 3; }
_KDEF(dddCtmr, "3rd deriv of Catmull-Rom", 4, Zero4);

static real ddCtmrEval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = -5 + 9*x;
    } else if (x < 2) {
        x -= 1;
        ret = 2 - 3*x;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddCtmrApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0] = 2 - 3*xa;
    ww[1] = -5 + 9*xa;
    ww[2] = 4 - 9*xa;
    ww[3] = -1 + 3*xa;
}
_KDEF(ddCtmr, "2nd deriv of Catmull-Rom", 4, dddCtmr);

static real dCtmrEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(-5 + x*((real)9/2));
    } else if (x < 2) {
        x -= 1;
        ret = -(real)1/2 + x*(2 - x*((real)3/2));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dCtmrApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    ww[0] = (-1 + (4 - 3*xa)*xa)/2;
    ww[1] = (xa*(-10 + 9*xa))/2;
    ww[2] = -((-1 + xa)*(1 + 9*xa))/2;
    ww[3] = (xa*(-2 + 3*xa))/2;
}
_KDEF(dCtmr, "1st deriv of Catmull-Rom", 4, ddCtmr);

static real CtmrEval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = 1 + x*x*(-(real)5/2 + x*((real)3/2));
    } else if (x < 2) {
        x -= 1;
        ret = x*(-(real)1/2 + x*(1 - x/2));
    } else {
        ret = 0;
    }
    return ret;
}
static void CtmrApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = -((xa-1)*(xa-1)*xa)/2;
    ww[1] = (2 + xa*xa*(-5 + 3*xa))/2;
    ww[2] = (xa + xa*xa*(4 - 3*xa))/2;
    ww[3] = ((-1 + xa)*xa*xa)/2;
}
KDEF(Ctmr, "Catmull-Rom spline (C1, reconstructs quadratic)", 4, dCtmr);

// ================================== Bspln3
// ================================== Bspln3
// ================================== Bspln3

static real dddBspln3Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = 3;
    } else if (x < 2) {
        ret = -1;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddBspln3Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    ww[0] = -1;
    ww[1] = 3;
    ww[2] = -3;
    ww[3] = 1;
}
_KDEF(dddBspln3, "3rd deriv of cubic B-spline", 4, Zero4);

static real ddBspln3Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = -2 + 3*x;
    } else if (x < 2) {
        x -= 1;
        ret = 1-x;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddBspln3Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0] = 1 - xa;
    ww[1] = -2 + 3*xa;
    ww[2] = 1 - 3*xa;
    ww[3] = xa;
}
_KDEF(ddBspln3, "2nd deriv of cubic B-spline", 4, dddBspln3);

static real dBspln3Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(-2 + x*((real)3/2));
    } else if (x < 2) {
        x -= 1;
        ret = -(real)1/2 + x*(1 - x/2);
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dBspln3Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    ww[0] = -(real)1/2 + xa*(1 - xa/2);
    ww[1] = xa*(-2 + xa*3/2);
    ww[2] = (real)1/2 + xa*(1 - (3*xa)/2);
    ww[3] = xa*xa/2;
}
_KDEF(dBspln3, "1st deriv of cubic B-spline", 4, ddBspln3);

static real Bspln3Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = (real)2/3 + x*x*(-1 + x/2);
    } else if (x < 2) {
        x -= 1;
        ret = (real)1/6 + x*(-(real)1/2 + x*((real)1/2 - x/6));
    } else {
        ret = 0;
    }
    return ret;
}
static void Bspln3Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = (real)1/6 + xa*(-(real)1/2 + xa*((real)1/2 - xa/6));
    ww[1] = (real)2/3 + xa*xa*(-1 + xa/2);
    ww[2] = (real)1/6 + xa*((real)1/2 + xa*((real)1/2 - xa/2));
    ww[3] = xa*xa*xa/6;
}
KDEF(Bspln3, "cubic B-spline (C2, reconstructs linear)", 4, dBspln3);


// ================================== Cos, Sin
// ================================== Cos, Sin
// ================================== Cos, Sin

/* the cos and sin kernels contain 2 periods of those
   functions, weighted by bspln3 */
static real CosEval(real x) {
    real ww = Bspln3Eval(x);
    return ww*cos(x*M_PI);
}
static void CosApply(real *ww, real xa) {
    Bspln3Apply(ww, xa); /* does KEC for us */
    ww[0] *= cos((xa+1)*M_PI);
    ww[1] *= cos((xa-0)*M_PI);
    ww[2] *= cos((xa-1)*M_PI);
    ww[3] *= cos((xa-2)*M_PI);
}
KDEF(Cos, "Cosine, weighted by bspln3", 4, Zero4);

static real SinEval(real x) {
    real ww = Bspln3Eval(x);
    return -ww*sin(x*M_PI); /* flipped so that animation
                               will look right */
}
static void SinApply(real *ww, real xa) {
    Bspln3Apply(ww, xa); /* does KEC for us */
    ww[0] *= -sin((xa+1)*M_PI);
    ww[1] *= -sin((xa-0)*M_PI);
    ww[2] *= -sin((xa-1)*M_PI);
    ww[3] *= -sin((xa-2)*M_PI);
}
KDEF(Sin, "Negative Sine, weighted by bspln3", 4, Zero4);

// ================================== Bspln4
// ================================== Bspln4
// ================================== Bspln4

static real ddddBspln4Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[4]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = 6;
    } else if (x < (real)3/2) {
        ret = -4;
    } else if (x < (real)5/2) {
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddddBspln4Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[4]++;
#endif
    ww[0] = 1;
    ww[1] = -4;
    ww[2] = 6;
    ww[3] = -4;
    ww[4] = 1;
}
_KDEF(ddddBspln4, "4th deriv of quartic B-spline", 5, Zero5);

static real dddBspln4Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < (real)1/2) {
        ret = 6*x;
    } else if (x < (real)3/2) {
        x -= 1;
        ret = 1 - 4*x;
    } else if (x < (real)5/2) {
        x -= 2;
        ret = -(real)1/2 + x;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddBspln4Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    ww[0] = -(real)1/2 + xa;
    ww[1] = 1 - 4*xa;
    ww[2] = 6*xa;
    ww[3] = -1 - 4*xa;
    ww[4] = (real)1/2 + xa;
}
_KDEF(dddBspln4, "3rd deriv of quartic B-spline", 4, ddddBspln4);

static real ddBspln4Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = -(real)5/4 + 3*x*x;
    } else if (x < (real)3/2) {
        x -= 1;
        ret = (real)1/2 + x*(1 - 2*x);
    } else if (x < (real)5/2) {
        x -= 2;
        ret = (real)1/8 + x*(-(real)1/2 + x/2);
    } else {
        ret = 0;
    }
    return ret;
}
static void ddBspln4Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0] = (real)1/8 + xa*(-(real)1/2 + xa/2);
    ww[1] = (real)1/2 + xa*(1 - 2*xa);
    ww[2] = -(real)5/4 + 3*xa*xa;
    ww[3] = (real)1/2 + xa*(-1 - 2*xa);
    ww[4] = (real)1/8 + xa*((real)1/2 + xa/2);
}
_KDEF(ddBspln4, "2nd deriv of quartic B-spline", 5, dddBspln4);

static real dBspln4Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < (real)1/2) {
        ret = x*(-(real)5/4 + x*x);
    } else if (x < (real)3/2) {
        x -= 1;
        ret = -(real)11/24 + x*((real)1/2 + x*((real)1/2 - 2*x/3));
    } else if (x < (real)5/2) {
        x -= 2;
        ret = -(real)1/48 + x*((real)1/8 + x*(-(real)1/4 + x/6));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dBspln4Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    ww[0] = -(real)1/48 + xa*((real)1/8 + xa*(-(real)1/4 + xa/6));
    ww[1] = -(real)11/24 + xa*((real)1/2 + xa*((real)1/2 - 2*xa/3));
    ww[2] = xa*(-(real)5/4 + xa*xa);
    ww[3] = (real)11/24 + xa*((real)1/2 + xa*(-(real)1/2 - 2*xa/3));
    ww[4] = (real)1/48 + xa*((real)1/8 + xa*((real)1/4 + xa/6));
}
_KDEF(dBspln4, "1st deriv of quartic B-spline", 4, ddBspln4);

static real Bspln4Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = (real)115/192 + x*x*(-(real)5/8 + x*x/4);
    } else if (x < (real)3/2) {
        x -= 1;
        ret = (real)19/96 + x*(-(real)11/24 + x*((real)1/4 + x*((real)1/6 - x/6)));
    } else if (x < (real)5/2) {
        x -= 2;
        ret = (real)1/384 + x*(-(real)1/48 + x*((real)1/16 + x*(-(real)1/12 + x/24)));
    } else {
        ret = 0;
    }
    return ret;
}
static void Bspln4Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = (real)1/384 + xa*(-(real)1/48 + xa*((real)1/16 + xa*(-(real)1/12 + xa/24)));
    ww[1] = (real)19/96 + xa*(-(real)11/24 + xa*((real)1/4 + xa*((real)1/6 - xa/6)));
    ww[2] = (real)115/192 + xa*xa*(-(real)5/8 + xa*xa/4);
    ww[3] = (real)19/96 + xa*((real)11/24 + xa*((real)1/4 + xa*(-(real)1/6 - xa/6)));
    ww[4] = (real)1/384 + xa*((real)1/48 + xa*((real)1/16 + xa*((real)1/12 + xa/24)));
}
KDEF(Bspln4, "quartic B-spline (C3, reconstructs linear)", 5, dBspln4);

// ================================== Bspln5
// ================================== Bspln5
// ================================== Bspln5

static real dddddBspln5Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[5]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = -10;
    } else if (x < 2) {
        ret = 5;
    } else if (x < 3) {
        ret = -1;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddddBspln5Apply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[5]++;
#endif
    ww[0] = -1;
    ww[1] = 5;
    ww[2] = -10;
    ww[3] = 10;
    ww[4] = -5;
    ww[5] = 1;
}
_KDEF(dddddBspln5, "3rd deriv of quintic B-spline", 6, Zero6);

static real ddddBspln5Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[4]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = 6 - 10*x;
    } else if (x < 2) {
        x -= 1;
        ret = -4 + 5*x;
    } else if (x < 3) {
        x -= 2;
        ret = 1 - x;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddddBspln5Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[4]++;
#endif
    ww[0] = 1 - xa;
    ww[1] = -4 + 5*xa;
    ww[2] = 6 - 10*xa;
    ww[3] = -4 + 10*xa;
    ww[4] = 1 - 5*xa;
    ww[5] = xa;
}
_KDEF(ddddBspln5, "4th deriv of quintic B-spline", 6, dddddBspln5);

static real dddBspln5Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(6 - 5*x);
    } else if (x < 2) {
        x -= 1;
        ret = 1 + x*(-4 + 5*x/2);
    } else if (x < 3) {
        x -= 2;
        ret = -(real)1/2 + x*(1 - x/2);
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddBspln5Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    ww[0] = -(real)1/2 + xa*(1 - xa/2);
    ww[1] = 1 + xa*(-4 + 5*xa/2);
    ww[2] = xa*(6 - 5*xa);
    ww[3] = -1 + xa*(-4 + 5*xa);
    ww[4] = (real)1/2 + xa*(1 - 5*xa/2);
    ww[5] = xa*xa/2;
}
_KDEF(dddBspln5, "3rd deriv of quintic B-spline", 6, ddddBspln5);

static real ddBspln5Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = -1 + x*x*(3 - 5*x/3);
    } else if (x < 2) {
        x -= 1;
        ret = (real)1/3 + x*(1 + x*(-2 + 5*x/6));
    } else if (x < 3) {
        x -= 2;
        ret = (real)1/6 + x*(-(real)1/2 + x*((real)1/2 - x/6));
    } else {
        ret = 0;
    }
    return ret;
}
static void ddBspln5Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0] = (real)1/6 + xa*(-(real)1/2 + xa*((real)1/2 - xa/6));
    ww[1] = (real)1/3 + xa*(1 + xa*(-2 + 5*xa/6));
    ww[2] = -1 + xa*xa*(3 - 5*xa/3);
    ww[3] = (real)1/3 + xa*(-1 + xa*(-2 + 5*xa/3));
    ww[4] = (real)1/6 + xa*((real)1/2 + xa*((real)1/2 - 5*xa/6));
    ww[5] = xa*xa*xa/6;
}
_KDEF(ddBspln5, "2nd deriv of quintic B-spline", 6, dddBspln5);

static real dBspln5Eval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(-1 + x*x*(1 - 5*x/12));
    } else if (x < 2) {
        x -= 1;
        ret = -(real)5/12 + x*((real)1/3 + x*((real)1/2 + x*(-(real)2/3 + 5*x/24)));
    } else if (x < 3) {
        x -= 2;
        ret = -(real)1/24 + x*((real)1/6 + x*(-(real)1/4 + x*((real)1/6 - x/24)));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dBspln5Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    ww[0] = -(real)1/24 + xa*((real)1/6 + xa*(-(real)1/4 + xa*((real)1/6 - xa/24)));
    ww[1] = -(real)5/12 + xa*((real)1/3 + xa*((real)1/2 + xa*(-(real)2/3 + 5*xa/24)));
    ww[2] = xa*(-1 + xa*xa*(1 - 5*xa/12));
    ww[3] = (real)5/12 + xa*((real)1/3 + xa*(-(real)1/2 + xa*(-(real)2/3 + 5*xa/12)));
    ww[4] = (real)1/24 + xa*((real)1/6 + xa*((real)1/4 + xa*((real)1/6 - 5*xa/24)));
    ww[5] = xa*xa*xa*xa/24;
}
_KDEF(dBspln5, "1st deriv of quintic B-spline", 6, ddBspln5);

static real Bspln5Eval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = (real)11/20 + x*x*(-(real)1/2 + x*x*((real)1/4 - x/12));
    } else if (x < 2) {
        x -= 1;
        ret = (real)13/60 + x*(-(real)5/12 + x*((real)1/6 + x*((real)1/6
                                             + x*(-(real)1/6 + x/24))));
    } else if (x < 3) {
        x -= 2;
        ret = (real)1/120 + x*(-(real)1/24 + x*((real)1/12 + x*(-(real)1/12
                                            + x*((real)1/24 - x/120))));
    } else {
        ret = 0;
    }
    return ret;
}
static void Bspln5Apply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = (real)1/120 + xa*(-(real)1/24 + xa*((real)1/12 + xa*(-(real)1/12
                                               + xa*((real)1/24 - xa/120))));
    ww[1] = (real)13/60 + xa*(-(real)5/12 + xa*((real)1/6 + xa*((real)1/6
                                                + xa*(-(real)1/6 + xa/24))));
    ww[2] = (real)11/20 + xa*xa*(-(real)1/2 + xa*xa*((real)1/4 - xa/12));
    ww[3] = (real)13/60 + xa*((real)5/12 + xa*((real)1/6 + xa*(-(real)1/6
                                                + xa*(-(real)1/6 + xa/12))));
    ww[4] = (real)1/120 + xa*((real)1/24 + xa*((real)1/12 + xa*((real)1/12
                                                + xa*((real)1/24 - xa/24))));
    ww[5] = xa*xa*xa*xa*xa/120;
}
KDEF(Bspln5, "quintic B-spline (C3, reconstructs linear)", 6, dBspln5);

// ================================== Spark
// ================================== Spark
// ================================== Spark

static real dddddSparkEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[5]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = -36;
    } else if (x < 2) {
        ret = 12;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddddSparkApply(real *ww, real xa) { USED(xa);
#ifdef KEC
    pltKernelApplyCount[5]++;
#endif
    ww[0]=12;
    ww[1]=-36;
    ww[2]=36;
    ww[3]=-12;
}
_KDEF(dddddSpark, "5th deriv of a smaller quintic", 4, Zero4);

static real ddddSparkEval(real x) {
#ifdef KEC
    pltKernelEvalCount[4]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = 18 - 36*x;
    } else if (x < 2) {
        x -= 1;
        ret = -6 + 12*x;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddddSparkApply(real *ww, real x) {
#ifdef KEC
    pltKernelApplyCount[4]++;
#endif
    ww[0]=-6 + 12*x;
    ww[1]=18 - 36*x;
    x=1-x;
    ww[2]=18 - 36*x;
    ww[3]=-6 + 12*x;
}
_KDEF(ddddSpark, "4th deriv of a smaller quintic", 4, dddddSpark);

static real dddSparkEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(18 - x*18);
    } else if (x < 2) {
        x -= 1;
        ret = x*(-6 + 6*x);
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddSparkApply(real *ww, real x) {
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    ww[0]=x*(-6 + 6*x);
    ww[1]=x*(18 - x*18);
    x=1-x;
    ww[2]=-(x*(18 - x*18));
    ww[3]=-(x*(-6 + 6*x));
}
_KDEF(dddSpark, "3rd deriv of a smaller quintic", 4, ddddSpark);

static real ddSparkEval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = -2 + x*x*(9 - 6*x);
    } else if (x < 2) {
        x -= 1;
        ret = 1 + x*x*(-3 + 2*x);
    } else {
        ret = 0;
    }
    return ret;
}
static void ddSparkApply(real *ww, real x) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0]=1 + x*x*(-3 + 2*x);
    ww[1]=-2 + x*x*(9 - 6*x);
    x=1-x;
    ww[2]=-2 + x*x*(9 - 6*x);
    ww[3]=1 + x*x*(-3 + 2*x);
}
_KDEF(ddSpark, "2nd deriv of a smaller quintic", 4, dddSpark);

static real dSparkEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(-2 + x*x*(3 - x*3/2));
    } else if (x < 2) {
        x -= 1;
        ret = -(real)1/2 + x*(1 + x*x*(-1 + x/2));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dSparkApply(real *ww, real x) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    ww[0]= -(real)1/2 + x*(1 + x*x*(-1 + x/2));
    ww[1]= x*(-2 + x*x*(3 - x*3/2));
    x=1-x;
    ww[2]= -(x*(-2 + x*x*(3 - x*3/2)));
    ww[3]= -(-(real)1/2 + x*(1 + x*x*(-1 + x/2)));
}
_KDEF(dSpark, "1st deriv of a smaller quintic", 4, ddSpark);

static real SparkEval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = (real)7/10 + x*x*(-1 + x*x*((real)3/4 - x*3/10));
    } else if (x < 2) {
        x -= 1;
        ret = (real)3/20 + x*(-(real)1/2 + x*((real)1/2
                                   + x*x*(-(real)1/4 + x/10)));
    } else {
        ret = 0;
    }
    return ret;
}
static void SparkApply(real *ww, real x) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0]= (real)3/20 + x*(-(real)1/2 + x*((real)1/2 + x*x*(-(real)1/4 + x/10)));
    ww[1]= (real)7/10 + x*x*(-1 + x*x*((real)3/4 - x*3/10));
    x=1-x;
    ww[2]= (real)7/10 + x*x*(-1 + x*x*((real)3/4 - x*3/10));
    ww[3]= (real)3/20 + x*(-(real)1/2 + x*((real)1/2 + x*x*(-(real)1/4 + x/10)));
}
KDEF(Spark,
     "a smaller (C3 continuous, reconstructs linear) piece-wise quintic",
     4, dSpark);

// ================================== Luna
// ================================== Luna
// ================================== Luna

static real dddddLunaEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[5]++;
#endif
    real ret, x = fabs(_x);
    if (x < (real)1/2) {
        ret = 0;
    } else if (x < (real)3/2) {
        ret = (real)75/2;
    } else if (x < (real)5/2) {
        ret = -(real)75/4;
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddddLunaApply(real *ww, real x) { USED(x);
#ifdef KEC
    pltKernelApplyCount[5]++;
#endif
    ww[0]=-(real)75/4;
    ww[1]=(real)75/2;
    ww[2]=0;
    ww[3]=-(real)75/2;
    ww[4]=(real)75/4;
}
_KDEF(dddddLuna, "5th deriv of a nice quintic kernel", 5, Zero5);

static real ddddLunaEval(real x) {
#ifdef KEC
    pltKernelEvalCount[4]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = (real)99/4;
    } else if (x < (real)3/2) {
        x -= (real)1/2;
        ret = -(real)141/4 + x*((real)75/2);
    } else if (x < (real)5/2) {
        x -= (real)3/2;
        ret = (real)27/2 - x*((real)75/4);
    } else {
        ret = 0;
    }
    return ret;
}
static void ddddLunaApply(real *ww, real _x) {
#ifdef KEC
    pltKernelApplyCount[4]++;
#endif
    { real x=_x+(real)1/2; ww[0] = (real)27/2 - x*75/4; }
    { real x=_x+(real)1/2; ww[1] = -(real)141/4 + x*75/2; }
    {                      ww[2] = (real)99/4; }
    { real x=(real)1/2-_x; ww[3] = -(real)141/4 + x*75/2; }
    { real x=(real)1/2-_x; ww[4] = (real)27/2 - x*75/4; }
}
_KDEF(ddddLuna, "4th deriv of a nice quintic kernel", 5, dddddLuna);

static real dddLunaEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < (real)1/2) {
        ret = x*((real)99/4);
    } else if (x < (real)3/2) {
        x -= (real)1/2;
        ret = (real)99/8 + x*(-(real)141/4 + x*((real)75/4));
    } else if (x < (real)5/2) {
        x -= (real)3/2;
        ret = -(real)33/8 + x*((real)27/2 - x*((real)75/8));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddLunaApply(real *ww, real _x) {
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    { real x=_x+(real)1/2; ww[0] = -(real)33/8 + x*((real)27/2 - x*75/8); }
    { real x=_x+(real)1/2; ww[1] = (real)99/8 + x*(-(real)141/4 + x*75/4); }
    { real x=_x;           ww[2] = x*99/4; }
    { real x=(real)1/2-_x; ww[3] = -((real)99/8 + x*(-(real)141/4 + x*75/4)); }
    { real x=(real)1/2-_x; ww[4] = -(-(real)33/8 + x*((real)27/2 - x*75/8)); }
}
_KDEF(dddLuna, "3rd deriv of a nice quintic kernel", 5, ddddLuna);

static real ddLunaEval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = -(real)115/32 + x*x*((real)99/8);
    } else if (x < (real)3/2) {
        x -= (real)1/2;
        ret = -(real)1/2 + x*((real)99/8 + x*(-(real)141/8 + x*((real)25/4)));
    } else if (x < (real)5/2) {
        x -= (real)3/2;
        ret = (real)1/2 + x*(-(real)33/8 + x*((real)27/4 - x*((real)25/8)));
    } else {
        ret = 0;
    }
    return ret;
}
static void ddLunaApply(real *ww, real _x) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    { real x=_x+(real)1/2; ww[0] = (real)1/2 + x*(-(real)33/8 + x*((real)27/4 - x*((real)25/8))); }
    { real x=_x+(real)1/2; ww[1] = -(real)1/2 + x*((real)99/8 + x*(-(real)141/8 + x*((real)25/4))); }
    { real x=_x;           ww[2] = -(real)115/32 + x*x*((real)99/8); }
    { real x=(real)1/2-_x; ww[3] = -(real)1/2 + x*((real)99/8 + x*(-(real)141/8 + x*((real)25/4))); }
    { real x=(real)1/2-_x; ww[4] = (real)1/2 + x*(-(real)33/8 + x*((real)27/4 - x*((real)25/8))); }
}
_KDEF(ddLuna, "2nd deriv of a nice quintic kernel", 5, dddLuna);

static real dLunaEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < (real)1/2) {
        ret = x*(-(real)115/32 + x*x*33/8);
    } else if (x < (real)3/2) {
        x -= (real)1/2;
        ret = -(real)41/32 + x*(-(real)1/2 + x*((real)99/16 + x*(-(real)47/8 + x*25/16)));
    } else if (x < (real)5/2) {
        x -= (real)3/2;
        ret = (real)3/32 + x*((real)1/2 + x*(-(real)33/16 + x*((real)9/4 - x*25/32)));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dLunaApply(real *ww, real _x) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    { real x=_x+(real)1/2; ww[0] = (real)3/32 + x*((real)1/2 + x*(-(real)33/16
                                                + x*((real)9/4 - x*25/32))); }
    { real x=_x+(real)1/2; ww[1] = -(real)41/32 + x*(-(real)1/2 + x*((real)99/16
                                              + x*(-(real)47/8 + x*25/16))); }
    { real x=_x;           ww[2] = x*(-(real)115/32 + x*x*33/8); }
    { real x=(real)1/2-_x; ww[3] = -(-(real)41/32 + x*(-(real)1/2
                         + x*((real)99/16 + x*(-(real)47/8 + x*25/16)))); }
    { real x=(real)1/2-_x; ww[4] = -((real)3/32 + x*((real)1/2 + x*(-(real)33/16
                                            + x*((real)9/4 - x*25/32)))); }
}
_KDEF(dLuna, "1st deriv of a nice quintic kernel", 5, ddLuna);

static real LunaEval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < (real)1/2) {
        ret = (real)485/512 + x*x*(-(real)115/64 + x*x*33/32);
    } else if (x < (real)3/2) {
        x -= (real)1/2;
        ret = (real)9/16 + x*(-(real)41/32 + x*(-(real)1/4 + x*((real)33/16
                                             + x*(-(real)47/32 + x*5/16))));
    } else if (x < (real)5/2) {
        x -= (real)3/2;
        ret = -(real)1/16 + x*((real)3/32 + x*((real)1/4 + x*(-(real)11/16
                                       + x*((real)9/16 - x*((real)5/32)))));
    } else {
        ret = 0;
    }
    return ret;
}
static void LunaApply(real *ww, real _x) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    { real x=_x+(real)1/2; ww[0] = -(real)1/16 + x*((real)3/32 + x*((real)1/4
                            + x*(-(real)11/16 + x*((real)9/16 - x*5/32)))); }
    { real x=_x+(real)1/2; ww[1] = (real)9/16 + x*(-(real)41/32 + x*(-(real)1/4
                           + x*((real)33/16 + x*(-(real)47/32 + x*5/16)))); }
    { real x=_x;           ww[2] = (real)485/512 + x*x*(-(real)115/64 + x*x*33/32); }
    { real x=(real)1/2-_x; ww[3] = (real)9/16 + x*(-(real)41/32 + x*(-(real)1/4
                     + x*((real)33/16 + x*(-(real)47/32 + x*(real)5/16)))); }
    { real x=(real)1/2-_x; ww[4] = -(real)1/16 + x*((real)3/32 + x*((real)1/4
                            + x*(-(real)11/16 + x*((real)9/16 - x*5/32)))); }
}
KDEF(Luna, "A nice (C3 continuous, reconstructs quadratics) "
     "piece-wise quintic kernel", 5, dLuna);

// ================================== Celie
// ================================== Celie
// ================================== Celie

static real ddddddCelieEval(real x) {
#ifdef KEC
    pltKernelEvalCount[6]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = 45;
    } else if (x < 2) {
        ret = -(real)135/2;
    } else if (x < 3) {
        ret = (real)45/2;
    } else {
        ret = 0;
    }
    return ret;
}
static void ddddddCelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[6]++;
#endif
    USED(xa);
    ww[0] = (real)45/2;
    ww[1] = -(real)135/2;
    ww[2] = 45;
    ww[3] = 45;
    ww[4] = -(real)135/2;
    ww[5] = (real)45/2;
}
_KDEF(ddddddCelie, "6th deriv of a nice hexic kernel", 6, Zero6);

static real dddddCelieEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[5]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = -70 + 45*x;
    } else if (x < 2) {
        x -= 1;
        ret = (real)115/2 - x*((real)135/2);
    } else if (x < 3) {
        x -= 2;
        ret = -16 + x*((real)45/2);
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddddCelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[6]++;
#endif
    ww[0] = 45*(-(real)32/45 + xa)/2;
    ww[1] = -135*(-(real)23/27 + xa)/2;
    ww[2] = -70 + 45*xa;
    ww[3] = 5*(5 + 9*xa);
    ww[4] = -135*(-(real)4/27 + xa)/2;
    ww[5] = 45*(-(real)13/45 + xa)/2;
}
_KDEF(dddddCelie, "5th deriv of a nice hexic kernel", 6, ddddddCelie);

static real ddddCelieEval(real x) {
#ifdef KEC
    pltKernelEvalCount[4]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = (real)57/2 + x*(-70 + x*45/2);
    } else if (x < 2) {
        x -= 1;
        ret = -19 + x*((real)115/2 - x*135/4);
    } else if (x < 3) {
        x -= 2;
        ret = (real)19/4 + x*(-16 + x*45/4);
    } else {
        ret = 0;
    }
    return ret;
}
static void ddddCelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[4]++;
#endif
    ww[0] = ((real)45/4)*((real)19/45 + (-(real)64/45 + xa)*xa);
    ww[1] = -((real)135/4)*((real)76/135 + (-(real)46/27 + xa)*xa);
    ww[2] = ((real)45/2)*((real)19/15 + (-(real)28/9 + xa)*xa);
    ww[3] = ((real)45/2)*(-(real)38/45 + xa*((real)10/9 + xa));
    ww[4] = -((real)135/4)*(-(real)19/135 + (-(real)8/27 + xa)*xa);
    ww[5] = ((real)45/4)*(xa*(-(real)26/45 + xa));
}
_KDEF(ddddCelie, "4th deriv of a nice hexic kernel", 6, dddddCelie);

static real dddCelieEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[3]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*((real)57/2 + x*(-35 + x*15/2));
    } else if (x < 2) {
        x -= 1;
        ret = 1 + x*(-19 + x*((real)115/4 - x*45/4));
    } else if (x < 3) {
        x -= 2;
        ret = -(real)1/2 + x*((real)19/4 + x*(-8 + x*15/4));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dddCelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[3]++;
#endif
    ww[0] = ((real)15/4)*(-(real)2/15 + xa*((real)19/15 + (-(real)32/15 + xa)*xa));
    ww[1] = -((real)45/4)*(-(real)4/45 + xa*((real)76/45 + (-(real)23/9 + xa)*xa));
    ww[2] = ((real)15/2)*xa*((real)19/5 + (-(real)14/3 + xa)*xa);
    ww[3] = ((real)15/2)*(-1 + xa)*((real)2/15 + xa*((real)8/3+xa));
    ww[4] = -((real)45/4)*(-(real)2/45 + xa*(-(real)19/45 + (-(real)4/9 + xa)*xa));
    ww[5] = ((real)15/4)*(xa*xa*(-(real)13/15 + xa));
}
_KDEF(dddCelie, "3rd deriv of a nice hexic kernel", 6, ddddCelie);

static real ddCelieEval(real x) {
#ifdef KEC
    pltKernelEvalCount[2]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = -(real)23/8 + x*x*((real)57/4 + x*(-(real)35/3 + x*15/8));
    } else if (x < 2) {
        x -= 1;
        ret = (real)19/12 + x*(1 + x*(-(real)19/2 + x*((real)115/12 - x*45/16)));
    } else if (x < 3) {
        x -= 2;
        ret = -(real)7/48 + x*(-(real)1/2 + x*((real)19/8 + x*(-(real)8/3 + x*15/16)));
    } else {
        ret = 0;
    }
    return ret;
}
static void ddCelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[2]++;
#endif
    ww[0] = -(real)7/48 + xa*(-(real)1/2 + xa*((real)19/8 + (-(real)8/3 + xa*15/16)*xa));
    ww[1] = (real)19/12 + xa*(1 + xa*(-(real)19/2 + ((real)115/12 - (real)45/16*xa)*xa));
    ww[2] = -(real)23/8 + xa*xa*((real)57/4 + xa*(-(real)35/3 + (real)15/8*xa));
    ww[3] = (real)19/12 + xa*(-1 + xa*(-(real)19/2 + xa*((real)25/6 + xa*15/8)));
    ww[4] = -(real)7/48 + xa*((real)1/2 + xa*((real)19/8 + ((real)5/3 - xa*45/16)*xa));
    ww[5] = xa*xa*xa*(-(real)13/12 + xa*15/16);
}
_KDEF(ddCelie, "2nd deriv of a nice hexic kernel", 6, dddCelie);

static real dCelieEval(real _x) {
#ifdef KEC
    pltKernelEvalCount[1]++;
#endif
    real ret, x = fabs(_x);
    if (x < 1) {
        ret = x*(-(real)23/8 + x*x*((real)19/4 + x*(-(real)35/12 + x*3/8)));
    } else if (x < 2) {
        x -= 1;
        ret = -(real)2/3 + x*((real)19/12 + x*((real)1/2 + x*(-(real)19/6
                                             + x*((real)115/48 - x*9/16))));
    } else if (x < 3) {
        x -= 2;
        ret = (real)1/12 + x*(-(real)7/48 + x*(-(real)1/4 + x*((real)19/24
                                               + x*(-(real)2/3 + x*3/16))));
    } else {
        ret = 0;
    }
    return (_x < 0) ? -ret : ret;
}
static void dCelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[1]++;
#endif
    ww[0] = (real)1/12 + xa*(-(real)7/48 + xa*(-(real)1/4 + xa*((real)19/24
                                             + (-(real)2/3 + xa*3/16)*xa)));
    ww[1] = -(real)2/3 + xa*((real)19/12 + xa*((real)1/2 + xa*(-(real)19/6
                                           + ((real)115/48 - xa*9/16)*xa)));
    ww[2] = xa*(-(real)23/8 + xa*xa*((real)19/4 + (-(real)35/12 + xa*3/8)*xa));
    ww[3] = (real)2/3 + xa*((real)19/12 + xa*(-(real)1/2 + xa*(-(real)19/6
                                             + ((real)25/24 + xa*3/8)*xa)));
    ww[4] = -(real)1/12 + xa*(-(real)7/48 + xa*((real)1/4 + xa*((real)19/24
                                       + ((real)5/12 - (real)9/16*xa)*xa)));
    ww[5] = xa*xa*xa*xa*(-(real)13/48 + xa*3/16);
}
_KDEF(dCelie, "1st deriv of a nice hexic kernel", 6, ddCelie);

static real CelieEval(real x) {
#ifdef KEC
    pltKernelEvalCount[0]++;
#endif
    real ret;
    x = fabs(x);
    if (x < 1) {
        ret = (real)69/80 + x*x*(-(real)23/16 + x*x*((real)19/16
                                      + x*(-(real)7/12 + x/16)));
    } else if (x < 2) {
        x -= 1;
        ret = (real)11/120 + x*(-(real)2/3 + x*((real)19/24 + x*((real)1/6
                     + x*(-(real)19/24 + x*((real)23/48 - x*((real)3/32))))));
    } else if (x < 3) {
        x -= 2;
        ret = -(real)11/480 + x*((real)1/12 + x*(-(real)7/96 + x*(-(real)1/12
                                + x*((real)19/96 + x*(-(real)2/15 + x/32)))));
    } else {
        ret = 0;
    }
    return ret;
}
static void CelieApply(real *ww, real xa) {
#ifdef KEC
    pltKernelApplyCount[0]++;
#endif
    ww[0] = -(real)11/480 + xa*((real)1/12 + xa*(-(real)7/96 + xa*(-(real)1/12
                              + xa*((real)19/96 + (-(real)2/15 + xa/32)*xa))));
    ww[1] = (real)11/120 + xa*(-(real)2/3 + xa*((real)19/24 + xa*((real)1/6
                            + xa*(-(real)19/24 + ((real)23/48 - xa*3/32)*xa))));
    ww[2] = (real)69/80 + xa*xa*(-(real)23/16 + xa*xa*((real)19/16 + (-(real)7/12
                                                                 + xa/16)*xa));
    ww[3] = (real)11/120 + xa*((real)2/3 + xa*((real)19/24 + xa*(-(real)1/6
                              + xa*(-(real)19/24 + ((real)5/24 + xa/16)*xa))));
    ww[4] = -(real)11/480 + xa*(-(real)1/12 + xa*(-(real)7/96 + xa*((real)1/12
                            + xa*((real)19/96 + ((real)1/12 - xa*3/32)*xa))));
    ww[5] = xa*(xa*(xa*(xa*((-(real)13/240 + xa/32)*xa))));
}
KDEF(Celie, "A nice (C4 continuous, reconstructs cubics) piece-wise hexic "
     "kernel", 6, dCelie);

const pltKernel *const pltKernelAll[] = {
    &Zero1Kernel,
    &BoxKernel,
    &CosKernel,
    &SinKernel,
    &ToothKernel,
    &TentKernel,
    &Bspln2Kernel,
    &Bspln3Kernel,
    &Bspln4Kernel,
    &Bspln5Kernel,
    &CtmrKernel,
    &SparkKernel,
    &LunaKernel,
    &CelieKernel,
    NULL
};
/* clang-format on */
