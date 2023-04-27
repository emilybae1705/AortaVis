/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

/*
  This is a stripped-down version of plt.h, with the declarations of all
  the things that are shared (modulo library renaming) between projects, to
  be used with Python's CFFI to generate Python bindings into libplt.  It
  is different than the common section of plt.h because the C parser used
  by CFFI does NOT also include a C pre-processor (so no #include, #define,
  #ifdef, etc), so the pre-processing has been done here by hand.  Many
  explanatory comments have been removed.
*/

/*
  select things from Teem (in $SCIVIS/tpz/shlib/libtpz.*) that may be
  needed to use the project library API. The typedef structs are very
  incomplete (only using the first field, and then CFFI's "fill in the
  rest" ... mechanism), or are missing all the informative comments;
  see headers in $SCIVIS/tpz/include for all the details.
*/

typedef struct {
    const char *name;
    /* what are these things? */
    unsigned int M;
    /* str[0]: string for the unknown/invalid value;
       str[1] .. str[M]: canonical strings for the enum values;
       "val"     NULL: unknown/invalid = 0;
                       valid values are 1 .. M
       "val" non-NULL: unknown/invalid = val[0];
                       valid are val[1].. val[M]
    */
    const char **str; /* see above */
    const int *val;   /* see above */
    const char **desc;
    /* desc[i] is a short description of the enum values represented by
       str[i] (thereby starting with the unknown value), to be used to
       by things like hest */
    const char **strEqv;
    /* If non-NULL, all the variations in strings recognized in mapping
       from string to value (the values in valEqv).  This **MUST** be
       terminated by a zero-length string ("") so as to signify the end
       of the list.  This should *not* contain the string for
       unknown/invalid.  If "strEqv" is NULL, then mapping from string
       to value is done only by traversing "str", and "valEqv" is
       ignored. */
    const int *valEqv;
    /* If strEqv non-NULL, valEqv holds the values corresponding to the
       strings in strEqv, with one integer for each non-zero-length
       string in strEqv: strEqv[i] is a valid string representation for
       value valEqv[i]. This should *not* contain the value for
       unknown/invalid.  This "valEqv" is ignored if "strEqv" is
       NULL. */
    int sense; /* require case matching on strings */
} airEnum;

typedef struct {
    void *data,               /* where the data is */
      **dataP;                /* (possibly NULL) address of user's data variable */
    unsigned int len,         /* length of array */
      *lenP,                  /* (possibly NULL) address of user's length variable */
      incr,                   /* the granularity of the changes allocated space */
      size;                   /* array is allocated to have "size" increments */
    size_t unit;              /* the size in bytes of one element in the array */
    int noReallocWhenSmaller; /* as it says */
    void *(*allocCB)(void);   /* values of new elements set to return of this */
    void *(*freeCB)(void *);  /* called on the values of invalidated elements */
    void (*initCB)(void *);   /* called on addresses of new elements */
    void (*doneCB)(void *);   /* called on addresses of invalidated elements */
} airArray;
extern int airEnumUnknown(const airEnum *enm);
extern const char *airEnumStr(const airEnum *enm, int val);
extern const char *airEnumDesc(const airEnum *enm, int val);
extern int airEnumVal(const airEnum *enm, const char *str);

typedef struct {
    char *flag;
    ...;
} hestOpt;

extern char *biffGetDone(const char *key);

enum {
    nrrdCenterUnknown, /* 0: no centering known for this axis */
    nrrdCenterNode,    /* 1: samples at corners of things */
    nrrdCenterCell,    /* 2: samples at middles of things */
    nrrdCenterLast
};

enum {
    nrrdKindUnknown,
    nrrdKindDomain, /*  1: any image domain */
    nrrdKindSpace,  /*  2: a spatial domain */
    nrrdKindTime,   /*  3: a temporal domain */
    /* -------------------------- end domain kinds */
    /* -------------------------- begin range kinds */
    nrrdKindList,            /*  4: any list of values, non-resample-able */
    nrrdKindPoint,           /*  5: coords of a point */
    nrrdKindVector,          /*  6: coeffs of (contravariant) vector */
    nrrdKindCovariantVector, /*  7: coeffs of covariant vector (eg gradient) */
    nrrdKindNormal,          /*  8: coeffs of unit-length covariant vector */
    /* -------------------------- end arbitrary size kinds */
    /* -------------------------- begin size-specific kinds */
    nrrdKindStub,              /*  9: axis with one sample (a placeholder) */
    nrrdKindScalar,            /* 10: effectively, same as a stub */
    nrrdKindComplex,           /* 11: real and imaginary components */
    nrrdKind2Vector,           /* 12: 2 component vector */
    nrrdKind3Color,            /* 13: ANY 3-component color value */
    nrrdKindRGBColor,          /* 14: RGB, no colorimetry */
    nrrdKindHSVColor,          /* 15: HSV, no colorimetry */
    nrrdKindXYZColor,          /* 16: perceptual primary colors */
    nrrdKind4Color,            /* 17: ANY 4-component color value */
    nrrdKindRGBAColor,         /* 18: RGBA, no colorimetry */
    nrrdKind3Vector,           /* 19: 3-component vector */
    nrrdKind3Gradient,         /* 20: 3-component covariant vector */
    nrrdKind3Normal,           /* 21: 3-component covector, assumed normalized */
    nrrdKind4Vector,           /* 22: 4-component vector */
    nrrdKindQuaternion,        /* 23: (w,x,y,z), not necessarily normalized */
    nrrdKind2DSymMatrix,       /* 24: Mxx Mxy Myy */
    nrrdKind2DMaskedSymMatrix, /* 25: mask Mxx Mxy Myy */
    nrrdKind2DMatrix,          /* 26: Mxx Mxy Myx Myy */
    nrrdKind2DMaskedMatrix,    /* 27: mask Mxx Mxy Myx Myy */
    nrrdKind3DSymMatrix,       /* 28: Mxx Mxy Mxz Myy Myz Mzz */
    nrrdKind3DMaskedSymMatrix, /* 29: mask Mxx Mxy Mxz Myy Myz Mzz */
    nrrdKind3DMatrix,          /* 30: Mxx Mxy Mxz Myx Myy Myz Mzx Mzy Mzz */
    nrrdKind3DMaskedMatrix,    /* 31: mask Mxx Mxy Mxz Myx Myy Myz Mzx Mzy Mzz */
    nrrdKindLast
};

enum {
    nrrdTypeUnknown = 0, /*  0: signifies "type is unset/unknown" */
    nrrdTypeDefault = 0, /*  0: signifies "determine output type for me" */
    nrrdTypeChar,        /*  1:   signed 1-byte integer */
    nrrdTypeUChar,       /*  2: unsigned 1-byte integer */
    nrrdTypeShort,       /*  3:   signed 2-byte integer */
    nrrdTypeUShort,      /*  4: unsigned 2-byte integer */
    nrrdTypeInt,         /*  5:   signed 4-byte integer */
    nrrdTypeUInt,        /*  6: unsigned 4-byte integer */
    nrrdTypeLLong,       /*  7:   signed 8-byte integer */
    nrrdTypeULLong,      /*  8: unsigned 8-byte integer */
    nrrdTypeFloat,       /*  9:          4-byte floating point */
    nrrdTypeDouble,      /* 10:          8-byte floating point */
    nrrdTypeBlock,       /* 11: size user defined at run time; MUST BE LAST */
    nrrdTypeLast
};

enum {
    nrrdSpaceUnknown,
    nrrdSpaceRightUp,                   /*  1: 2-D, oriented like upper right
                                            Cartesian quadrant, number I */
    nrrdSpaceRightDown,                 /*  2: 2-D, oriented like raster
                                            coordinates */
    nrrdSpaceRightAnteriorSuperior,     /*  3: NIFTI-1 (right-handed) */
    nrrdSpaceLeftAnteriorSuperior,      /*  4: standard Analyze (left-handed) */
    nrrdSpaceLeftPosteriorSuperior,     /*  5: DICOM 3.0 (right-handed) */
    nrrdSpaceRightAnteriorSuperiorTime, /*  6: */
    nrrdSpaceLeftAnteriorSuperiorTime,  /*  7: */
    nrrdSpaceLeftPosteriorSuperiorTime, /*  8: */
    nrrdSpaceScannerXYZ,                /*  9: ACR/NEMA 2.0 (pre-DICOM 3.0) */
    nrrdSpaceScannerXYZTime,            /* 10: */
    nrrdSpace3DRightHanded,             /* 11: */
    nrrdSpace3DLeftHanded,              /* 12: */
    nrrdSpace3DRightHandedTime,         /* 13: */
    nrrdSpace3DLeftHandedTime,          /* 14: */
    nrrdSpaceLast
};

const unsigned int NRRD_DIM_MAX = 16;
const unsigned int NRRD_SPACE_DIM_MAX = 8;
typedef struct {
    size_t size;      /* number of elements along each axis */
    double spacing;   /* if non-NaN, distance between samples */
    double thickness; /* if non-NaN, nominal thickness of one sample */
    double min, max;  /* if non-NaN, range of positions spanned axis */
    double spaceDirection[NRRD_SPACE_DIM_MAX];
    /* inter-sample vector in world-space */
    int center;  /* cell vs. node centering */
    int kind;    /* what kind of information is along this axis */
    char *label, /* short info string for each axis */
      *units;    /* string identifying the unit */
} NrrdAxisInfo;
typedef struct {
    void *data;                           /* the data in memory */
    int type;                             /* a value from the nrrdType enum */
    unsigned int dim;                     /* the dimension (rank) of the array */
    NrrdAxisInfo axis[NRRD_DIM_MAX];      /* info about all the axes */
    char *content;                        /* brief account of what this data is */
    char *sampleUnits;                    /* units of measurement of the values */
    int space;                            /* from nrrdSpace* enum */
    unsigned int spaceDim;                /* dim of space in which the grid lies */
    char *spaceUnits[NRRD_SPACE_DIM_MAX]; /* units for coordinates of space */
    double
      spaceOrigin[NRRD_SPACE_DIM_MAX]; /* the location of the center the first sample */
    double measurementFrame[NRRD_SPACE_DIM_MAX][NRRD_SPACE_DIM_MAX];
    /* coordinate transform  from measurement space
       to world space */
    size_t blockSize;      /* for nrrdTypeBlock, block byte size */
    double oldMin, oldMax; /* extremal values prior to quantization */
    void *ptr;             /* nope */
    char **cmt;            /* comments */
    airArray *cmtArr;
    char **kvp; /* key/value pairs */
    airArray *kvpArr;
} Nrrd;
typedef struct {
    char *path;
    ...;
} NrrdIoState;
extern const airEnum *const nrrdType;
extern const airEnum *const nrrdSpace;
extern const airEnum *const nrrdCenter;
extern const airEnum *const nrrdKind;
extern Nrrd *nrrdNew(void);
extern Nrrd *nrrdNuke(Nrrd *nrrd);
extern int nrrdMaybeAlloc_nva(Nrrd *nrrd, int type, unsigned int dim,
                              const size_t *size);
extern int nrrdMaybeAlloc_va(Nrrd *nrrd, int type, unsigned int dim,
                             ... /* size_t sx, sy, .., ax(dim-1) size */);
extern int nrrdLoad(Nrrd *nrrd, const char *filename, NrrdIoState *nio);
extern int nrrdSave(const char *filename, const Nrrd *nrrd, NrrdIoState *nio);

// Teem excerpts done, now common section of plt.h

typedef unsigned int uint;
typedef uint8_t uchar;

extern int pltVerbose;
extern const char *pltBiffKey;
// would be good to make this available somehow...
// #define PLT pltBiffKey // identifies this library in biff error messages
extern const int pltRealIsDouble;
extern real pltNan(unsigned short payload);
extern unsigned short pltNanPayload(real nval);
extern int pltPrintf(const char *restrict format, ...);
extern int pltSprintf(char *restrict dest, const char *restrict format, ...);
extern int pltFprintf(FILE *file, const char *restrict format, ...);
extern unsigned int pltFloatDist(float A, float B);
extern char *pltCommandLine(const char *me, const char **argv);
extern char *pltTodoStr(const char **strs);
extern long pltMicrosecondsUsed();
extern void pltWallTime(long SecMicrosec[2]);
extern long pltMicrosecondsWallTimeDiff(const long smsEnd[2], const long smsStart[2]);
extern void pltOutput(const char *fname);
extern int pltOutputRenamePrefix(const char *prefix);
extern void pltRoundGetenv(void);
extern int pltRoundDir(void);
extern int pltRoundBracket(void);
extern void pltRoundDirSet(int dir);
extern void pltRoundSet(void);
extern void pltRoundUnset(void);
extern uint pltSprintLenMax(uint oldmax, const real *vv, uint vvNum);

typedef struct pltKernel_t {
    const char *name;      // short identifying string
    const char *desc;      // short descriptive string
    unsigned int support;  // # samples needed for convolution
    real (*eval)(real xx); // evaluate kernel once
    void (*apply)(real *ww, real xa);
    const struct pltKernel_t *deriv; /* derivative of this kernel; will point
                                          back to itself when kernel is zero */
} pltKernel;

/* kernel.c: gives compile-time definition of some reconstruction
   kernels and their derivatives (nothing for you to implement) */
extern const pltKernel *const pltKernelZero;
extern const pltKernel *const pltKernelBox;
extern const pltKernel *const pltKernelTent;
extern const pltKernel *const pltKernelBspln2;
extern const pltKernel *const pltKernelBspln3;
extern const pltKernel *const pltKernelBspln4;
extern const pltKernel *const pltKernelBspln5;
extern const pltKernel *const pltKernelCtmr;
extern const pltKernel *const pltKernelSpark;
extern const pltKernel *const pltKernelLuna;
extern const pltKernel *const pltKernelCelie;
extern const pltKernel *const pltKernelAll[];

const int PLT_KERNEL_SUPPORT_MAX = 6;

extern const pltKernel *pltKernelParse(const char *kstr);
