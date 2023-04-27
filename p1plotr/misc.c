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

// C99's compound literals simplify creating airEnums at compile-time

static const airEnum _pltCenter_ae
  = {"sample centering",
     2,
     (const char *[]){"(unknown_center)", //
                      "node",             //
                      "cell"},
     (int[]){pltCenterUnknown, //
             pltCenterNode,    //
             pltCenterCell},
     (const char *[]){"unknown centering",
                      "samples at boundaries between segments of world-space",
                      "samples at centers of segments of world-space"},
     NULL,
     NULL,
     AIR_FALSE};
const airEnum *const pltCenter_ae = &_pltCenter_ae;

static const airEnum _pltMark_ae = //
  {"plot mark",
   8,
   (const char *[]){
     "(unknown_mark)", // 0
     "background",     // 1
     "axes",           // 2
     "polynomial",     // 3
     "outsideOne",     // 4
     "outsideMore",    // 5
     "convo",          // 6
     "dot",            // 7
     "zeroCrossing"    // 8
   },
   (int[]){
     pltMarkUnknown,
     pltMarkBackground,
     pltMarkAxes,
     pltMarkPolynomial,
     pltMarkOutsideOne,
     pltMarkOutsideMore,
     pltMarkConvolution,
     pltMarkDot,
     pltMarkZeroCrossing,
   },
   (const char *[]){"unknown mark",                //
                    "background behind all marks", //
                    "axes along Y=0 and X=0",      //
                    "plot of an exact polynomial", //
                    "where (on Y=0 axis) one data sample missing for convo",
                    "where (on Y=0 axis) > one data samples missing for convo",
                    "plot of the convolution result",
                    "dots showing underlying data samples",
                    "zero crossings (by lerp) between data samples"},
   NULL,
   NULL,
   AIR_FALSE};
const airEnum *const pltMark_ae = &_pltMark_ae;
