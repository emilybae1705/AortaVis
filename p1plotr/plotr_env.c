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
#ifdef SCIVIS_GRADE
#  include "pltGrade.h"
#endif
#include "pltTime.h"

/* this, like the grading and timing stuff, is defined in plotr.c, but it doesn't (yet)
 * warrant its own header file */
extern int pltHandleSignals(void);

#define INFO "Prints out state set via environment variables"

static int
envMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    USED(argc);
    USEd(argv, me, hparm);

    printf("%d pltVerbose\n", pltVerbose);
    printf("%d pltHandleSignals()\n", pltHandleSignals());
    printf("%d pltRoundDir()\n", pltRoundDir());
    printf("%d pltRoundBracket()\n", pltRoundBracket());
    pltPrintf("%g pltTimeout()\n", pltTimeout());
    printf("%d pltTimeDo()\n", pltTimeDo());
    printf("%d pltTimeVerbose()\n", pltTimeVerbose());
    printf("%u pltTimeMedian()\n", pltTimeMedian());
    printf("%u pltTimeExtra()\n", pltTimeExtra());
    pltPrintf("%g pltTimeSpread()\n", pltTimeSpread());
#ifdef SCIVIS_GRADE
    char str[GRADE_STRLEN + 1];
    printf("\"%s\" pltGradeStart()\n", pltGradeStart(str));
    printf("\"%s\" pltGradeContinue()\n", pltGradeContinue(str));
    printf("\"%s\" pltGradeEnd()\n", pltGradeEnd(str));
    pltPrintf("%g pltGradeThresh()\n", pltGradeThresh());
    printf("\"%s\" pltGradeThreshUnit()\n", pltGradeThreshUnit(str));
    printf("%d pltGradePoints()\n", pltGradePoints());
    printf("%d pltGradePointStep()\n", pltGradePointStep());
    pltPrintf("%g pltGradeTimeAllowance()\n", pltGradeTimeAllowance());
    printf("%d pltGradeImageVertJoin()\n", pltGradeImageVertJoin());
    pltPrintf("%g pltGradeImageQuantCap()\n", pltGradeImageQuantCap());
    printf("%d pltGradeImagePixelSlop()\n", pltGradeImagePixelSlop());
    pltPrintf("%g pltGradeImagePixelEps()\n", pltGradeImagePixelEps());
    pltPrintf("%g pltGradeImagePixelCap()\n", pltGradeImagePixelCap());
    pltPrintf("%g pltGradeImageFracEps()\n", pltGradeImageFracEps());
    pltPrintf("%g pltGradeImageFracCap()\n", pltGradeImageFracCap());
#endif // SCIVIS_GRADE

    return 0;
}

unrrduCmd plt_envCmd = {"env", INFO, envMain, AIR_TRUE};
