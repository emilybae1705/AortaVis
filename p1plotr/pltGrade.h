/*
  plt and plotr: SciVis-2023 Project 1
  Copyright (C)  2023 University of Chicago. All rights reserved.

  This is only for students and instructors in the 2023 CMSC 23710/33710
  ("SciVis") class, for use in that class. It is not licensed for open-source
  or any other kind of re-distribution. Do not allow this file to be copied
  or downloaded by anyone outside the 2023 SciVis class.
*/
// functions used for grading, all defined in plotr.c

#define GRADE_STRLEN 512

/* accessors for variables that are set via environment variables and static to plotr.c
 */
extern char *pltGradeStart(char dst[GRADE_STRLEN + 1]);
extern char *pltGradeContinue(char dst[GRADE_STRLEN + 1]);
extern char *pltGradeEnd(char dst[GRADE_STRLEN + 1]);
extern real pltGradeThresh(void);
extern char *pltGradeThreshUnit(char dst[GRADE_STRLEN + 1]);
extern int pltGradePoints(void);
extern int pltGradePointStep(void);
extern real pltGradeTimeAllowance(void);
extern real pltGradeTimeout(void);
extern int pltGradeImageVertJoin(void);
extern real pltGradeImageQuantCap(void);
extern int pltGradeImagePixelSlop(void);
extern real pltGradeImagePixelEps(void);
extern real pltGradeImagePixelCap(void);
extern real pltGradeImageFracEps(void);
extern real pltGradeImageFracCap(void);

extern void pltGradeMultiStart(int mean);
extern void pltGradeMultiFinish();
extern void pltGradeInt(int yy, int ryy, const char *desc);
extern void pltGradeEnum(int yy, int ryy, const airEnum *enm, const char *desc);
extern void pltGradeReal(real yy, real ryy, const char *desc);
extern void pltGradeVector(const real *v, const real *rv, uint len, const char *desc);
extern void pltGradeString(const char *sstr, const char *rstr, uint headLines,
                           int sortLines, const char *desc);

// one in a list of items to be graded at once
typedef struct {
    char desc[GRADE_STRLEN + 1]; // what is this
    real stu;                    // student value
    real ref;                    // reference value
    real thrscl;                 /* how threshold for this item should scaled
                                    relative to pltGradeThresh */
} pltGradeItem;
extern void pltGradeItemSet(pltGradeItem *itm, const char *desc, real stu, real ref,
                            real thrscl);
extern pltGradeItem *pltGradeItemZip(real *stu, real *ref, uint len);
extern void pltGradeTuple(const pltGradeItem *itm, uint itmLen, long stutime,
                          long reftime);

extern void pltGradeImage(const char *oname, const char *content, const void *_stu,
                          const void *_ref, int is8bit, uint chan, uint isz0, uint isz1,
                          long stutime, long reftime);
