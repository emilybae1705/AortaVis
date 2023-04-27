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

// sanity check on order of derivative
#define DMAX 10

/*
  pltConvoEval: compute (or try to compute) convolution-based reconstruction of data with
  kern, or some derivative of the reconstruction, at world-space location xw.  This is
  the function that implements the "field abstraction": having a continuous function
  defined over some segment of world-space. If convolution can't be performed because
  some data values are missing, then record how many were missing.  NOTE: you should call
  the pltKernel->eval method to evaluate the kernel, not pltKernel->apply (that is
  intended for later assignments).

  INPUT:
  xw: the world-space position at which to evaluate the convolution

  whichD: evaluate convolution with the whichD-th derivative of the given kernel kern.
  For no differentiation, whichD=0. To measure a first derivative, whichD=1; second
  derivative, whichD=2, etc. The differentiation is in *world* space! It is your
  responsibility to account for difference between differentiation in index space
  (convolving with a derivative kernel) vs in world space (taking into account space
  between samples).

  data: the discretely sampled data to convolve with

  kern: the kernel to convolve with

  OUTPUT:
  result: The convolution result, when it can be computed, is stored in *result.
  Otherwise (when convolution can't be computed), *result should be left unchanged.

  outside: you should set *outside with every call. If the convolution sum could not be
  computed because the kernel support didn't fit entirely within the data (that is,
  because some terms of the convolution sum required data values data->vv[i] for which i
  was outside the valid range of data indices [0,data->len-1]), then *result is not set,
  and *outside records the number missing data values (the number of required data
  indices that were outside [0,data->len-1]). Missing some of the required data values is
  NOT an actual error, in the sense of a non-zero function return value and the
  subsequent error handling.  When the convolution result can be computed (all
  the data sample indices within the kernel support were valid data indices),
  then set *outside to 0.

  Note that in practical use, this function might may be called multiple times, first
  with whichD=0, then whichD=1, ..., with the same xw, in which case there is certainly
  redundant computation being done. For the sake of this project, it is worth taking that
  efficiency hit in order to simplify the implementation.

  RETURNS:
  0 if all is well, else
  1 in case of error (using biff), because of missing pointers or
  nonfinite xw.  No additional error checking is required in your code.
*/
int
pltConvoEval(real *result, unsigned short *outside, real xw, uint whichD,
             const pltData *data, const pltKernel *kern) {
    if (!(result && outside && data && kern)) {
        biffAddf(PLT, "%s: got NULL pointer (%p,%p,%p,%p)", __func__, (void *)result,
                 (void *)outside, (const void *)data, (const void *)kern);
        return 1;
    }
    if (!isfinite(xw)) {
        biffAddf(PLT, "%s: given world-space position %g not finite", __func__, xw);
        return 1;
    }
    if (whichD > DMAX) {
        biffAddf(PLT, "%s: whichD %u > reasonable limit %u", __func__, whichD, DMAX);
        return 1;
    }
    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltConvoEval)
    
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (72L in ref)
    return 0;
}

int
pltConvoSample(pltData *odata, uint olen, real omin, real omax, pltCenter center,
               uint whichD, const pltData *idata, const pltKernel *kern) {
    if (!(odata && idata && kern)) {
        biffAddf(PLT, "%s: got NULL pointer (%p,%p,%p)", __func__, (void *)odata,
                 (const void *)idata, (const void *)kern);
        return 1;
    }
    if (pltDataInit(odata, olen, omin, omax, center)) {
        biffAddf(PLT, "%s: problem initializing output data", __func__);
        return 1;
    }
    real result;
    unsigned short outside;
    if (pltConvoEval(&result, &outside, (idata->max + idata->min) / 2, whichD, idata,
                     kern)) {
        biffAddf(PLT, "%s: trial convolution failed", __func__);
        return 1;
    }

    /* here is where pltConvoEval is computed over all the sample positions,
       and where inability to do convolution (because of non-zero outside) is
       recorded as a NaN with payload==outside. */
    for (uint si = 0; si < olen; si++) {
        real sw = pltItoW(odata, si);
        pltConvoEval(&result, &outside, sw, whichD, idata, kern);
        if (!outside) {
            odata->vv[si] = result;
        } else {
            odata->vv[si] = pltNan(outside);
        }
    }

    return 0;
}
