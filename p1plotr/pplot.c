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
  You should use this function to set a given pixel in the output image, e.g. setrgb(pix,
  rgbBackground) or setrgb(pix, rgbPlot).  It takes one of the pltMark enum values to
  lookup into the RGBLUT array; changes to the RGBLUT contents should change the output
  colors your code uses.
*/
static inline void
setrgb(unsigned char *pix, pltMark which) {
    static const unsigned char RGBLUT[][3] = {
      {255, 255, 255}, // pltMarkBackground,
      {220, 220, 255}, // pltMarkAxes,
      {255, 150, 110}, // pltMarkPolynomial,
      {220, 100, 255}, // pltMarkOutsideOne,
      {20, 20, 255},   // pltMarkOutsideMore,
      {0, 0, 0},       // pltMarkConvolution,
      {150, 150, 150}, // pltMarkDot,
      {100, 255, 100}, // pltMarkZeroCrossing,
    };
    const unsigned char *rgb = RGBLUT[which - pltMarkBackground];
    pix[0] = rgb[0];
    pix[1] = rgb[1];
    pix[2] = rgb[2];
}

/* You may (if you want) define new functions or #define macros here; but our intent is
   only that they would be used in this file.  Thus, prefix any function declarations
   here with "static" */
// v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pplot.c helpers)
// ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (12L in ref)

/*
  pltPlot: plots a convolution result and/or a polynomial into an RGB image,
  according to various parameters.

  INPUT:
  xsize, ysize: the output image array rgb has been pre-allocated for 3*xsize*ysize
  values (each of type unsigned char).  The axis ordering of array "rgb" is (fast to
  slow): RGB, X, Y. The convention of RGB array display is that the faster spatial
  index
  ("X") which increases from left to right, and the slower spatial index ("Y")
  increases from *top* to *bottom*.

  xmin, xmax: the X (horizontal) axis of the image has xsize CELL-CENTERED samples in
  the world-space interval [xmin,xmax], with xmin on the left side of the image, and
  xmax on the right side.

  ymin, ymax: the Y (vertical) axis of the image has ysize CELL-CENTERED samples in
  the world-space interval [ymin,ymax]. ymin is the vertical world-space position of
  the *bottom* of the image; ymax is the vertical position of the *top* of the image.
  Increasing the Y world-space coordinate will *decrease* the index-space coordinate,
  which is opposite of how the X axis is handled.

  whichD: the whichD-th derivative of the convolution result should be plotted. With
  whichD==0, the convolution itself is plotted; with whichD==1, the first derivative
  is plotted, etc.  Derivatives are with respect to *world* space (thanks to
  pltConvoEval).

  idata, kern: exactly as in pltConvoEval.  If idata is non-NULL, the convolution of
  idata with kern is plotted. The convolution result should ONLY be plotted where it
  is defined (as per the *outside value set by pltConvoEval).  If idata is NULL, then
  there is no data convolution to compute or to plot; perhaps only the polynomial is
  to be plotted.

  pc, pclen: a representation of a polynomial, as with pltPolyEval. If pc is non-NULL
  and pclen > 0, then the polynomial represented by pc should be plotted.  This
  analytic polynomial is defined for all world-space positions, regardless of the
  sampling interval of the input data. If pc is NULL, then no polynomial should be
  plotted.

  thicknessConvo, thicknessPoly: the *vertical* height, in pixels, of the graph of the
  convolution result, and of the polynomial). That is, the judgement of whether a pixel
  is inside the plot curve is based on the difference between the vertical (index-space,
  integer) pixel location and the vertical (index-space, not necessarily integer)
  location of the plot, at that horizontal location.

  apcoth: will either be 1 or 0. If 1, then APproximate COnstant-THickness plots of
  the convolution and the polynomial. The per-pixel inside test is still based on
  vertical offset from the plot, but by knowing the slope (in index-space) of the
  plot at that horizontal position, and using the Pythagorean Theorem, the amount of
  the vertical offset that still counts as inside the plot curve becomes a function
  of slope: as the plot curve becomes more vertical (with either positive or negative
  slope), a greater range of vertical offsets are "inside" the plot curve. For plots
  that are straight lines, the approximation is exact, and the plot of the convolution
  or polynomial will be exactly thicknessConvo or thicknessPoly pixels thick.

  thicknessAxes: if this is non-zero: draw axes at to indicate (world-space) y=0 and
  (world-space) x=0, with this thickness (in pixels).

  diameterDot: if this is non-zero: indicate the input discrete data values with
  circular dots, with this diameter (in pixels).  There are smart and dumb ways of
  implementing this; dumb is great for this project.  In particular, it is okay if
  the execution time of pltPlot increases linearly with increasing number of data
  points.

  heightZC: if this is non-zero, and if idata is non-NULL, draw vertical tickmarks of
  this height (in pixels) and with thickness thicknessAxes, to show the
  Zero-Crossings in the data. This is independent of the data's node- vs
  cell-centering, and independent of the convolution kernel. Consider the set of line
  segments connected end-to-end formed by LINEAR interpolation (lerping) between
  successive input data values in idata.  The zero-crossing tickmarks show where one
  line segment touches the horizontal axis (y=0) at an isolated point. This is
  typically inside the interval between two adjacent idata sample locations, but it
  can also happen exactly at one endpoint of that interval. Do not draw these
  tickmarks inside an interval that starts and ends at y=0 (since then the line
  segment is touching y=0 at more than an isolated point).

  OUTPUT:

  rgb: the output plot image is stored here, with graphical marks (i.e. plots, axes,
  dots, tick-marks) drawn according to the parameters and specification above. A
  pixel is inside a graphical mark if the *center* of the pixel (recall that this
  image space is always sampled with cell-centered samples) falls within the
  image-space extent of that mark. For all the graphical marks, the *location* of the
  mark is defined in terms of world-space, but its extent (thickness or diameter),
  and hence a pixel's membership in the mark, is defined in terms of image
  index-space, and computed in terms of comparisons of (non-integral) index-space
  coordinates.

  The identity of the background and all marks is represented by the pltMark C enum:
  -- pltMarkBackground   : background behind all marks
  -- pltMarkAxes,        : axes along Y=0 and X=0
  -- pltMarkPolynomial,  : plot of polynomial given by pc (if non-NULL), pclen
  The remaining marks are drawn if idata is non-NULL:
  -- pltMarkOutsideMore, : show on the X (Y=0) axis those X locations that are inside
                           the data sampling interval, but outside the convolution
                           support by more than one sample
  -- pltMarkOutsideOne,  : show on the X axis those X location that are inside the
                           data sampling interval, but outside convolution support by
                           exactly one sample
  -- pltMarkConvolution, : plot of result of convolving sampled data with kernel (at
                           those locations where convolution could be computed
                           because the kernel support fell entirely within the data)
  -- pltMarkDot,         : dots showing the idata input data samples
  -- pltMarkZeroCrossing : vertical tick-marks at zero-crossings of lerp'ed data

  The ordering is important: the correct image is defined as the result of the marks
  being drawn in the above order: first background, then axes, then polynomial, etc.
  The zero-crossing tick-marks appear on top of any other marks, and the dots to show
  input data values appear on top of the convolution plot, which appears on top of
  the polynomial plot, etc.  The ordering of marks above is also a possible order of
  implementation.

  More about pltMarkOutsideOne and pltMarkOutsideMore: This is to visualize where we
  have data, but aren't able to compute convolution, because the kernel support
  extends past valid data indices.  If the kernel was missing more than one data
  value, color the axis by rgbOutsideMore; if only one data value was missing, color
  the axis by rgbOutsideOne. Even when so colored, the y=0 axis still has thickness
  thicknessAxes. To implement this, note how pltConvoSample() uses pltNan() to
  produce a NaN when some samples are missing from the convolution sum.  The number
  of missing samples is stored in the integer "payload" of NaN, which can be
  retrieved with pltNanPayload().

  RETURNS: 0 if all is well, else 1 in case of error (using biff), because of bad
  parameters. However all the error checking is already done for you: no further
  error handling is needed in your code.
*/
int
pltPlot(unsigned char *const rgb, uint xsize, uint ysize, real xmin, real xmax,
        real ymin, real ymax, uint whichD, const pltData *idata, const pltKernel *kern,
        const real *pc, uint pclen, real thicknessConvo, real thicknessPoly, int apcoth,
        real thicknessAxes, real diameterDot, real heightZC) {
    USED(heightZC);
    if (!(rgb)) {
        biffAddf(PLT, "%s: got NULL rgb pointer", __func__);
        return 1;
    }
    if (1 == !!idata + !!kern) {
        biffAddf(PLT,
                 "%s: need idata (%p) and kern (%p) to either be both "
                 "non-NULL (to plot convo of idata with kern) or both NULL",
                 __func__, (const void *)idata, (const void *)kern);
        return 1;
    }
    /* either we want to plot convolution result of idata and kern, or
    no convolution plotting wanted */
    if (1 == !!pc + !!pclen) {
        biffAddf(PLT,
                 "%s: need either NULL pc (%p) and zero pclen (%u) or "
                 "non-NULL pc and pclen>0",
                 __func__, (const void *)pc, pclen);
        return 1;
    }
    if (!(0 == apcoth || 1 == apcoth)) {
        biffAddf(PLT, "%s: apcoth should be 0 or 1 (not %d)", __func__, apcoth);
        return 1;
    }
    /* either we have pclen>=1 coefficients in polynomial pc,
    or polynomial plotting wanted */
    /* It's ok to have NULL idata and NULL pc: just draw background and axes */
    if (!(isfinite(thicknessConvo) && thicknessConvo >= 0  //
          && isfinite(thicknessPoly) && thicknessPoly >= 0 //
          && isfinite(thicknessAxes) && thicknessAxes >= 0 //
          && isfinite(diameterDot) && diameterDot >= 0)) {
        biffAddf(PLT, "%s: non-finite or negative size params (%g,%g,%g,%g)", __func__,
                 thicknessConvo, thicknessPoly, thicknessAxes, diameterDot);
        return 1;
    }
    /* The "mop" is a gadget for remembering what dynamically-allocated resources we
       have to free up when we return from this function */
    airArray *mop = airMopNew();
    /* NOTE: creating "ydata" to store the meta-data about the Y axis. You should use
       pltItoW and pltWtoI with ydata to convert between Y index- and world-space,
       even though the ydata->vv[] data elements are not actually needed or set. Note
       also the order in which ymin and ymax are passed to pltDataInit: this is how
       we reconcile the raster image convention of Y index increasing as we move
       downwards, with the Cartesian coordinate convention of Y position decreasing
       as we move downwards. */
    pltData *ydata = pltDataNew();
    airMopAdd(mop, ydata, (airMopper)pltDataNix, airMopAlways);
    if (pltDataInit(ydata, ysize, ymax, ymin, pltCenterCell)) {
        biffAddf(PLT, "%s: trouble setting up ydata", __func__);
        airMopError(mop);
        return 1;
    }
    /* Creating "xdata" to store meta-data about X axis, and, if idata, the
       convolution results to be plotted */
    pltData *xdata = pltDataNew();
    airMopAdd(mop, xdata, (airMopper)pltDataNix, airMopAlways);
    pltData *ddata;
    if (idata) {
        // we have data for which we plot the convolution
        ddata = pltDataNew(); // derivative of xdata
        airMopAdd(mop, ddata, (airMopper)pltDataNix, airMopAlways);
        /* precompute a vector of convolution results, one value per horizontal
           output pixel, both for what we plot (the "whichD"-th derivative ) and
           its derivative (the ("whichD"+1)-th derivative) */
        if (pltConvoSample(xdata, xsize, xmin, xmax, pltCenterCell, whichD, idata, kern)
            || pltConvoSample(ddata, xsize, xmin, xmax, pltCenterCell, whichD + 1, idata,
                              kern)) {
            biffAddf(PLT, "%s: trouble convolving with input data", __func__);
            airMopError(mop);
            return 1;
        }
    } else {
        // even if not plotting data, we can usefully set meta-data in xdata
        if (pltDataInit(xdata, xsize, xmin, xmax, pltCenterCell)) {
            biffAddf(PLT, "%s: trouble setting up xdata", __func__);
            airMopError(mop);
            return 1;
        }
        ddata = NULL;
    }
    /* now, either way (with or without sampled data to plot), xdata is set.
       NOTE: You should use xdata as part of any and all conversions between
       index and world space along the X axis. */
    pltData *pdata = NULL, // for pre-computed evaluations of polynomial pc
      *qdata = NULL;       // for pre-computed evaluations of derivative of pc
    if (pc) {
        // we have a polynomial to plot
        pdata = pltDataNew();
        airMopAdd(mop, pdata, (airMopper)pltDataNix, airMopAlways);
        if (pltPolySample(pdata, xsize, xmin, xmax, pltCenterCell, pc, pclen)) {
            biffAddf(PLT, "%s: trouble sampling polynomial", __func__);
            airMopError(mop);
            return 1;
        }
        if (apcoth) {
            /* Need polynomial derivative, so create qdata to store that. Create qc
               as coefficients of the derivative of polynomial. qc[] is a C99
               variable-length array (VLA), which is a nice convenience (as compared
               to using malloc() and then making sure to free() later), HOWEVER qc[]
               will be allocated on the stack, which means that valgrind's great
               memtrace tool is less able to detect issues with how you use qc[]. The
               code here using qc[] is simple enough that we can probably avoid any
               bugs, but in general, NOTE that the difficulty of memtrace to analyze
               usage of stack-allocated memory is a good reason to avoid VLAs. You
               have been warned. */
            uint qclen = pclen > 1 ? pclen - 1 : 1;
            real qc[qclen];
            qc[0] = 0; // in case pclen==1 (derivative of constant is 0)
            for (uint pi = 1; pi < pclen; pi++) {
                qc[pi - 1] = pi * pc[pi];
            }
            qdata = pltDataNew();
            airMopAdd(mop, qdata, (airMopper)pltDataNix, airMopAlways);
            // sample the polynomial derivative
            if (pltPolySample(qdata, xsize, xmin, xmax, pltCenterCell, qc, qclen)) {
                biffAddf(PLT, "%s: trouble sampling polynomial derivative", __func__);
                airMopError(mop);
                return 1;
            }
        } // if (apcoth)
    }     // if (pc)

    /* initialize output values to some predictable but very incorrect stripe pattern */
    for (uint ii = 0; ii < xsize * ysize; ii++) {
        setrgb(rgb + 3 * ii,
               ((ii / 4) % (xsize / 4)) ? pltMarkZeroCrossing : pltMarkOutsideMore);
    }

    /*
    Suggested strategy: traverse the output image index space, and for each pixel, start
    by getting its address (to pass to setrgb(), the function you should use to set RGB
    pixel values).  Initialize the pixel to the background color. Then, use a mix of
    geometry and arithmetic to see if the pixel location is inside one of the marks to
    draw, and if so set that pixel color accordingly.  So some pseudo-code might be
    summarized as:

      for Pixel in all image pixels:
        set Pixel color to background color
        for Mark in coordinate axes, graph of polynomial,
                    outside more, outside one,
                    graph of convolution result,
                    dots at data points, zero-crossing tick marks:
          if Pixel is inside Mark:
            set Pixel color to Mark color

    The "for Pixel" loop is naturally expressible with C for loops, but the "for Mark"
    loop is really just a sequence of per-mark tests, for each of the pltMark values.

    Note that it is *perfectly okay* to write to set the pixel color more than once
    (since that simplifies implementation).  It is even ok, for drawing the data dots and
    the zero-crossing tickmarks, to re-traverse the entire data at every pixel! The
    reference implemenation does this. This is terribly inefficient, but it is ok given
    the pedagogical scope of this intro assignment (more convolution math than about
    efficiency).

    Make use of the samples, pre-computed above, of convolution (xdata, and its
    derivative ddata) or polynomial (pdata, and its derivative qdata), and use xdata and
    ydata for managing all index-to-world space conversions on X and Y axes respectively.
    NOTE: no new work for doing convolution or polynomial evaluation should be happening
    below.

    This is largely a test of your ability to juggle the many coordinate spaces in
    play: X and Y image world-spaces, X and Y image index-spaces, and the world- and
    index-spaces of the input data.
    */
    // v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code (pltPlot)
    // ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code (80L in ref)

    airMopOkay(mop);
    return 0;
}
