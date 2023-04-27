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

const char *pltTodo[]
  = {"file",    "C function",      "plotr command", "grade/go test", "\\",
     "util.c",  "pltLerp()",       "plotr lerp",    "lerp",          "\\",
     "util.c",  "pltItoW()",       "plotr itow",    "util",          "\\",
     "util.c",  "pltWtoI()",       "plotr wtoi",    "util",          "\\",
     "poly.c",  "pltPolyEval()",   "plotr peval",   "poly",          "\\",
     "poly.c",  "pltPolySample()", "plotr psamp",   "poly",          "\\",
     "convo.c", "pltConvoEval()",  "plotr ceval",   "convo",         "\\",
     "pplot.c", "pltPlot()",       "plotr plot",    "plot",          NULL};

#define INFO "Just a list of what to do for Project 1"

static int
todoMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    USED(argc);
    USEd(argv, me, hparm);
    char *todo = pltTodoStr(pltTodo);
    printf("%s", todo);
    airFree(todo);
    return 0;
}

unrrduCmd plt_todoCmd = {"todo", INFO, todoMain, AIR_TRUE};
