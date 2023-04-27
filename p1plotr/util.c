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
  pltLerp: lerping

  INPUT:
  imin, imax: bounds of input value interval
  real xx: value considered relative to input interval
  omin, omax: bounds of output interval

  OUTPUT:
  returns yy that ranges from omin to omax as xx ranges from imin to imax:

   yy - omin      xx - imin
  ----------- =  -----------
  omax - omin    imax - imin

  It is the caller's responsibility to make sure imax != imin. No error
  checking (for imax == imax, or any non-finite values) is required here.
  NOTE: use only "real"s for intermediate values, not "double"s.
*/
real
pltLerp(real omin, real omax, real imin, real xx, real imax) {
    real ret = 0;
    /* The USED() here just stop compiler warnings complaining about "unused arguments"
       before you've finished the work here. They do not appear in any way in the actual
       (assembly) output from the compiler. */
    USED(omin, omax, imin, xx, imax);
    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltLerp)
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (2L in ref)
    return ret;
}

/*
  pltItoW: convert from index to world space, for given data
  pltWtoI: convert from world to index space, for given data

  No error is checking needed for either pltItoW or pltWtoI.
  You can assume that:
  - data is non-NULL,
  - data->center is either pltCenterCell or pltCenterNode,
  - data->len is large enough for given centering, and
  - data->min != data->max

  Note that you will not actually be looking at data values in data->vv.
  Your code will use data->center, data->len, data->min, and data->max.

  You will lose points if your pltItoW and pltWtoI code does not rely on
  pltLerp. You really just need two ways of calling pltLerp, depending on
  the value of data->center.
*/
real
pltItoW(const pltData *data, real indexPos) {
    real ret = 0;
    USEd(data);
    USED(indexPos);
    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltItoW)
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (6L in ref)
    return ret;
}

real
pltWtoI(const pltData *data, real worldPos) {
    real ret = 0;
    USEd(data);
    USED(worldPos);
    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltWtoI)
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (6L in ref)
    return ret;
}
