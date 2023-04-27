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
  pltPolyEval: evaluates at xx a polynomial with coefficients pc

  INPUT: xx: position at which to evaluate polynomial pc, pclen: pclen is the length of
  the array pc; valid values are pc[0], pc[1], ... pc[pclen-1]. The pclen coefficients of
  pc are stored in increasing degree, i.e., the polynomial to evaluate is: p(x) = pc[0] +
  x*pc[1] + (x^2)*pc[2] ... + (x^(pclen-1))*pc[pclen-1] The polynomial is of degree
  pclen-1.

  OUTPUT:
  Returns p(x). There is no error checking. Assume pc is non-NULL, and that pclen >= 1.

  NOTE: Use nested multiplication (sometimes called "Horner's method") to evaluate the
  polynomial, use type "real" to store all intermediate results, and do not use pow() for
  exponentiation.
*/
real
pltPolyEval(const real *pc, uint pclen, real xx) {
    real ret = 0;
    USEd(pc);
    USED(pclen, xx);
    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltPolyEval)
    for (uint pi = pclen; pi-- > 0; ) {
      pltPrintf("pi=%u", pi);
      ret = ret * xx + pc[pi];
    }
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (3L in ref)
    return ret;
}

/*
  pltPolySample: evaluate polynomial p (given by pc, pclen) with len samples in the
  interval [imin, imax], either cell-centered or node-centered depending on center.

  INPUT:
  len: the number of samples at which to evaluate the polynomial

  imin, imax: len samples are located interval [imin, imax] (with either cell- or
  node-centered sampling), and the polynomial is evaluated at these samples.

  center: either pltCenterCell or pltCenterNode, to request either cell- or node-centered
  sampling.

  pc, pclen: defines a polynomial p(x), same as with pltPolyEval

  OUTPUT:
  data: the given code below will initialize this container to store len samples.  The
  results of sampling p(x) (via repeated calls to pltPolyEval) should be stored in
  data->vv[]. data->vv[0] should hold the results of evaluating p(x) for x nearest imin,
  regardless of whether imin is less than or greater than imax. NOTE: you should be using
  another function you've written to manage the mapping between world and index space.

  RETURNS:
  0 if all is well, 1 if there is an error, which is detected with the given code. No
  additional error handling should be in the student code.
*/
int
pltPolySample(pltData *data, uint len, real imin, real imax, pltCenter center,
              const real *pc, uint pclen) {
    if (!(data && pc)) {
        biffAddf(PLT, "%s: got NULL pointer (%p,%p)", __func__, (void *)data,
                 (const void *)pc);
        return 1;
    }
    if (!pclen) {
        biffAddf(PLT, "%s: got zero polynomial coeffs?", __func__);
        return 1;
    }
    if (pltDataInit(data, len, imin, imax, center)) {
        biffAddf(PLT, "%s: problem initializing data", __func__);
        return 1;
    }

    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltPolySample)
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (4L in ref)

    return 0;
}
