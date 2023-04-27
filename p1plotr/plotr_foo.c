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

#define INFO "Stub command; does nothing; for hacking/scratch-space"
static char *infoLong = INFO
  ". There is no purpose for this, except as a place where "
  "you can write code that you want to try or experiment with. "
  "NOTE THAT NOTHING HERE WILL BE GRADED!  The purpose of this is save "
  "you the hassle of creating a separate main() function "
  "in a new main.c, and then figuring out how to compile/link it. "
  "\"plotr foo\" is already built (and is always built) as part of "
  "building the \"plotr\" command.  So, the only restriction is that "
  "you can't add code here that doesn't compile cleanly.";

static int
fooMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    int nn;
    hestOptAdd(&hopt, "n", "int", airTypeInt, 1, 1, &nn, "23710",
               "minimal hest usage: parse an int");

    /* ... feel free to add more hest options, as guided by the examples of
       hest usage in other plotr commands. Just keep in mind that, because
       hestOptAdd() depends on a user-defined type system (airTypeInt,
       airTypeString, ..., and, with airTypeOther it uses additional var-args
       arguments), the compiler can do ZERO useful typechecking on its
       arguments, so you have to be careful about how you call it */

    /* this next line ("hparm->noArgsIsNoProblem = AIR_TRUE;") means that,
       unlike with other plotr commands, it's not actually a problem if none of
       the command-line options are supplied; without this, "plotr foo" would
       generate its usage information, which would be annoying, since the
       purpose is to be a hacking/scratch-space. */
    hparm->noArgsIsNoProblem = AIR_TRUE;
    hestParseOrDie(hopt, argc, argv, hparm, me, infoLong, AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
    printf("(for further information please see \"%s --help\")\n", me);

    // . . . . . . . . . . . . . . . . . useful code here
    printf("%s: pltVerbose = %d\n", __func__, pltVerbose);

    // ... do whatever you want; just make sure it compiles cleanly ...

    /* for example, the following demonstrates the IEEE 754 NaN's "payload"
       with pltNan() and pltNanPayload(). */
    real rr = pltNan(nn);
    unsigned short payload = pltNanPayload(rr);
    pltPrintf("%d -> %g -> %hu\n", nn, rr, payload);

    // ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' useful code here

    /* (GLK uses this to ensure that the airEnums are created correctly)
    char err[AIR_STRLEN_LARGE];
    if (airEnumCheck(err, pltCenter_ae)) {
        printf("pltCenter_ae problem:\n%s\n", err);
    }
    if (airEnumCheck(err, pltMark_ae)) {
        printf("pltMark_ae problem:\n%s\n", err);
    }
    */

    // keep this to avoid memory leaks
    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_fooCmd = {"foo", INFO, fooMain, AIR_FALSE};
