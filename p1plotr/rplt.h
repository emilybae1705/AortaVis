/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/

/*
  You can ignore this file; it part of the grading mechanics.
  grade/plotr needs to be able to call both your code and the
  reference code; this header declares the reference code symbols,
  which have been prefixed by "ref_".
*/

extern int ref_pltVerbose;
extern const char *ref_pltBiffKey;
extern const int ref_pltRealIsDouble;
extern real ref_pltNan(unsigned short payload);
extern unsigned short ref_pltNanPayload(real nval);
extern void ref_pltPrintLogStart(void);
extern const char *ref_pltPrintLog(void);
extern void ref_pltPrintLogFinish(void);
extern int ref_pltPrintf(const char *restrict format, ...)
  __attribute__((format(printf, 1, 2)));
extern int ref_pltSprintf(char *restrict dest, const char *restrict format, ...)
  __attribute__((format(printf, 2, 3)));
extern int ref_pltFprintf(FILE *file, const char *restrict format, ...)
  __attribute__((format(printf, 2, 3)));
extern unsigned int ref_pltFloatDist(float A, float B);
extern char *ref_pltCommandLine(const char *me, const char **argv);
extern char *ref_pltTodoStr(const char **strs);
extern long int ref_pltMicrosecondsUsed(void);
extern void ref_pltWallTime(long SecMicrosec[2]);
extern long int ref_pltMicrosecondsWallTimeDiff(const long smsEnd[2],
                                                const long smsStart[2]);
extern void ref_pltOutput(const char *fname);
extern int ref_pltOutputRenamePrefix(const char *prefix);
extern void ref_pltRoundGetenv(void);
extern int ref_pltRoundDir(void);
extern int ref_pltRoundBracket(void);
extern void ref_pltRoundDirSet(int dir);
extern void ref_pltRoundSet(void);
extern void ref_pltRoundUnset(void);
extern uint ref_pltSprintLenMax(uint oldmax, const real *vv, uint vvNum);
extern const pltKernel *const ref_pltKernelZero;
extern const pltKernel *const ref_pltKernelBox;
extern const pltKernel *const ref_pltKernelSin;
extern const pltKernel *const ref_pltKernelCos;
extern const pltKernel *const ref_pltKernelTooth;
extern const pltKernel *const ref_pltKernelTent;
extern const pltKernel *const ref_pltKernelBspln2;
extern const pltKernel *const ref_pltKernelBspln3;
extern const pltKernel *const ref_pltKernelBspln4;
extern const pltKernel *const ref_pltKernelBspln5;
extern const pltKernel *const ref_pltKernelCtmr;
extern const pltKernel *const ref_pltKernelSpark;
extern const pltKernel *const ref_pltKernelLuna;
extern const pltKernel *const ref_pltKernelCelie;
extern const pltKernel *const pltKernelAll[];
extern const pltKernel *ref_pltKernelParse(const char *kstr);
extern hestCB *ref_pltKernelHest;
extern const airEnum *const ref_pltCenter_ae;
extern const airEnum *const ref_pltMark_ae;
extern pltData *ref_pltDataNew(void);
extern pltData *ref_pltDataNix(pltData *data);
extern int ref_pltDataInit(pltData *data, uint len, real min, real max,
                           pltCenter center);
extern int ref_pltDataSave(const char *fname, const pltData *data, const char *content);
extern int ref_pltDataLoad(pltData *data, const char *fname);
extern real ref_pltLerp(real omin, real omax, real imin, real xx, real imax);
extern real ref_pltItoW(const pltData *data, real indexPos);
extern real ref_pltWtoI(const pltData *data, real worldPos);
extern real ref_pltPolyEval(const real *pc, uint pclen, real xx);
extern int ref_pltPolySample(pltData *data, uint len, real imin, real imax,
                             pltCenter center, const real *pc, uint pclen);
extern int ref_pltConvoEval(real *result, unsigned short *outside, real xx, uint whichD,
                            const pltData *data, const pltKernel *kern);
extern int ref_pltConvoSample(pltData *odata, uint olen, real omin, real omax,
                              pltCenter center, uint whichD, const pltData *din,
                              const pltKernel *kern);
extern int ref_pltPlot(unsigned char *const rgb, uint xsize, uint ysize, real xmin,
                       real xmax, real ymin, real ymax, uint whichD,
                       const pltData *idata, const pltKernel *kern, const real *pc,
                       uint pclen, real thicknessConvo, real thicknessPoly, int apcoth,
                       real thicknessAxes, real dotRadius, real heightZC);
