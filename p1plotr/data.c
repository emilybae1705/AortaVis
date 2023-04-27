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

pltData *
pltDataNew(void) {
    pltData *ret = MALLOC(1, pltData);
    assert(ret);
    ret->vv = NULL;
    ret->len = 0;
    ret->min = pltNan(0);
    ret->max = pltNan(0);
    ret->center = pltCenterUnknown;
    return ret;
}

pltData *
pltDataNix(pltData *data) {
    assert(data);
    if (data->vv) {
        free(data->vv);
    }
    free(data);
    return NULL;
}

static int
dataAlloc(pltData *data, uint len) {
    if (!data) {
        biffAddf(PLT, "%s: got NULL pointer", __func__);
        return 1;
    }
    if (data->vv) {
        free(data->vv);
        data->vv = NULL;
    }
    if (len) {
        data->vv = MALLOC(len, real);
        if (!data->vv) {
            biffAddf(PLT, "%s: couldn't alloc %u reals", __func__, len);
            return 1;
        }
        /* else initilize data to some nonsense. In later projects we we won't
           want to spend time initializing data this way, but for this first
           project it helps highlight when things to wrong */
        real nan = pltNan(0);
        for (uint ii = 0; ii < len; ii++) {
            data->vv[ii] = nan;
        }
    }
    data->len = len;
    return 0;
}

static int
metaDataCheck(unsigned len, real min, real max, pltCenter center) {
    if (pltCenterCell == center) {
        if (!(len >= 1)) {
            biffAddf(PLT, "%s: need >= 1 (not %u) cell-centered samples", __func__, len);
            return 1;
        }
    } else if (pltCenterNode == center) {
        if (!(len >= 2)) {
            biffAddf(PLT, "%s: need >= 2 (not %u) node-centered samples", __func__, len);
            return 1;
        }
    } else {
        biffAddf(PLT, "%s: invalid sample centering %d", __func__, center);
        return 1;
    }
    if (!(min != max && isfinite(min) && isfinite(max))) {
        biffAddf(PLT, "%s: sampling interval [%g,%g] 0-length or not finite", __func__,
                 min, max);
        return 1;
    }
    return 0;
}

int
pltDataInit(pltData *data, uint len, real min, real max, pltCenter center) {
    if (!data) {
        biffAddf(PLT, "%s: got NULL pointer", __func__);
        return 1;
    }
    if (dataAlloc(data, len) || metaDataCheck(len, min, max, center)) {
        biffAddf(PLT, "%s: trouble with allocation or meta data", __func__);
        return 1;
    }
    data->len = len;
    data->min = min;
    data->max = max;
    data->center = center;
    return 0;
}

int
pltDataSave(const char *fname, const pltData *data, const char *content) {
    if (!(fname && data)) {
        biffAddf(PLT, "%s: got NULL pointer (%p,%p)\n", __func__, (const void *)fname,
                 (const void *)data);
        return 1;
    }
    if (metaDataCheck(data->len, data->min, data->max, data->center)) {
        biffAddf(PLT, "%s: trouble with meta data", __func__);
        return 1;
    }
    airArray *mop = airMopNew();
    Nrrd *ndta = nrrdNew();
    airMopAdd(mop, ndta, (airMopper)nrrdNix, airMopAlways);
    if (nrrdWrap_va(ndta, data->vv, nrrdTypeReal, 1, (size_t)(data->len))) {
        biffMovef(PLT, NRRD, "%s: trouble wrapping data in nrrd", __func__);
        airMopError(mop);
        return 1;
    }
    ndta->axis[0].min = (double)data->min;
    ndta->axis[0].max = (double)data->max;
    ndta->axis[0].center = (pltCenterCell == data->center ? nrrdCenterCell
                                                          : nrrdCenterNode);
    if (content && strlen(content)) {
        ndta->content = airStrdup(content); // strdup() isn't properly in C99
        assert(ndta->content);
    }

    NrrdIoState *nio = nrrdIoStateNew();
    airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
    /* these next 2 lines ensures that when saving to a plain text file (as
       indicated by giving an output filename ending in ".txt") all the
       required meta-data is stored in # comments */
    nio->bareText = AIR_FALSE;
    nio->moreThanFloatInText = AIR_TRUE;
    if (nrrdSave(fname, ndta, nio)) {
        biffMovef(PLT, NRRD, "%s: trouble saving output", __func__);
        airMopError(mop);
        return 1;
    }
    airMopOkay(mop);
    return 0;
}

static int
checkNrrd(const Nrrd *nin) {
    if (nrrdCheck(nin)) {
        biffMovef(PLT, NRRD, "%s: problem with nrrd itself", __func__);
        return 1;
    }
    if (nrrdTypeReal != nin->type) {
        biffAddf(PLT, "%s: got type %s, not %s", __func__,
                 airEnumStr(nrrdType, nin->type), airEnumStr(nrrdType, nrrdTypeReal));
        return 1;
    }
    if (1 != nin->dim) {
        biffAddf(PLT, "%s: got dimension %u, not 1", __func__, nin->dim);
        return 1;
    }
    if (!(isfinite(nin->axis[0].min) && isfinite(nin->axis[0].max))) {
        biffAddf(PLT, "%s: axis[0]'s min (%g) or max (%g) not set", __func__,
                 nin->axis[0].min, nin->axis[0].max);
        return 1;
    }
    if (nin->axis[0].min == nin->axis[0].max) {
        biffAddf(PLT, "%s: axis[0]'s min and max are both %g", __func__,
                 nin->axis[0].max);
        return 1;
    }
    if (airEnumValCheck(nrrdCenter, nin->axis[0].center)) {
        biffAddf(PLT, "%s: axis[0]'s centering not set", __func__);
        return 1;
    }
    return 0;
}

int
pltDataLoad(pltData *data, const char *fname) {
    if (!(data && fname)) {
        biffAddf(PLT, "%s: got NULL pointer (%p,%p)\n", __func__, (void *)data,
                 (const void *)fname);
        return 1;
    }
    airArray *mop = airMopNew();
    Nrrd *nin = nrrdNew();
    airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdLoad(nin, fname, NULL)) {
        biffMovef(PLT, NRRD, "%s: trouble reading file", __func__);
        airMopError(mop);
        return 1;
    }
    if (checkNrrd(nin)
        || pltDataInit(data, (uint)(nin->axis[0].size), (real)nin->axis[0].min,
                       (real)nin->axis[0].max,
                       (nrrdCenterCell == nin->axis[0].center ? pltCenterCell
                                                              : pltCenterNode))) {
        biffAddf(PLT, "%s: trouble with data", __func__);
        airMopError(mop);
        return 1;
    }
    memcpy(data->vv, nin->data, data->len * sizeof(real));
    airMopOkay(mop);
    return 0;
}
