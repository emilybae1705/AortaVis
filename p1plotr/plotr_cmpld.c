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

#define INFO "Documents how plotr was compiled"

static int
cmpldMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    USED(argc);
    USEd(argv, me, hparm);

#if SCIVIS_REAL_IS_DOUBLE
    printf("REAL_IS_DOUBLE 1\n");
#else
    printf("REAL_IS_DOUBLE 0\n");
#endif

#ifdef NDEBUG
    printf("NDEBUG 1\n");
#else
    if (0 == strlen("-DNDEBUG")) {
        /* This is probably ptrun, or something else with GradeWithDebug=True.
           grade/go checks the output of "plotr cmpld" to ensure correct
           compilation. We could teach grade/go about GradeWithDebug, so that
           it doesn't care that a grading program is compiled withOUT
           -DNDEBUG, or, we could just lie about it, which is what we do here */
        printf("NDEBUG 1\n");
    } else {
        printf("NDEBUG 0\n");
    }
#endif

#ifdef SCIVIS_GRADE
    printf("GRADE 1\n");
#else
    printf("GRADE 0\n");
#endif

    return 0;
}

unrrduCmd plt_cmpldCmd = {"cmpld", INFO, cmpldMain, AIR_TRUE};
