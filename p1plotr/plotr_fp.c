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
#include <stdint.h> // for uint32_t

/* this file is the only part of plt that doesn't buy into the "real" typedef, so here
 * there is lots of float-specific stuff */

// additional headers with #define's that "./plotr fp -defs" can list
#include <float.h>
#include <fenv.h>
#include <math.h>
#include <limits.h>

/* __STR, _STR, FUNC, MAP, and PRINT all are pre-processor
   tricks to implement ./plotr fp -defs */

/* Macro for making a string out of whatever something has been #define'd to, exactly,
   without chasing down a sequence of #includes. This also side-steps the possible
   changes from printf conversion of numeric values, and allows us to see, for example,
   if the #define'd value is actually a function call. c.f.
   https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html */
#define __STR(name) #name
#define _STR(name)  __STR(name)

/* FSTR(FOO) defines a function str_FOO that returns either the #define'd value of FOO,
   if it has been #define'd to a constant, else returns a string indicating that FOO has
   not been so #define'd. Note that this says nothing about whether FOO is a #define'd
   macro (e.g. "#define FOO(x) (x*x)"); we would need to use an #ifdef to check for that,
   but #ifdef's can't themselves be part of a macro expansion. */
#define FSTR(FOO)                                                                       \
  static const char *str_##FOO(void) {                                                  \
    static const char ncon[] = "(" #FOO " is not a #define'd constant)";                \
    static const char here[] = _STR(FOO);                                               \
    const char *ret;                                                                    \
    if (strcmp(here, #FOO)) {                                                           \
      ret = here;                                                                       \
    } else {                                                                            \
      ret = ncon;                                                                       \
    }                                                                                   \
    return ret;                                                                         \
  }

// Put here all the things to show the #define'd (constant) value of
#define MAP(X)                                                                          \
  X(EOF)                                                                                \
  X(NULL)                                                                               \
  X(INFINITY)                                                                           \
  X(NAN)                                                                                \
  X(CHAR_BIT)                                                                           \
  X(SCHAR_MIN)                                                                          \
  X(SHRT_MIN)                                                                           \
  X(INT_MIN)                                                                            \
  X(SCHAR_MAX)                                                                          \
  X(SHRT_MAX)                                                                           \
  X(INT_MAX)                                                                            \
  X(UCHAR_MAX)                                                                          \
  X(USHRT_MAX)                                                                          \
  X(UINT_MAX)                                                                           \
  X(FLT_RADIX)                                                                          \
  X(DECIMAL_DIG)                                                                        \
  X(FLT_DECIMAL_DIG)                                                                    \
  X(FLT_MIN)                                                                            \
  X(FLT_TRUE_MIN)                                                                       \
  X(FLT_MAX)                                                                            \
  X(FLT_EPSILON)                                                                        \
  X(FLT_DIG)                                                                            \
  X(FLT_MANT_DIG)                                                                       \
  X(FLT_MIN_EXP)                                                                        \
  X(FLT_MIN_10_EXP)                                                                     \
  X(FLT_MAX_EXP)                                                                        \
  X(FLT_MAX_10_EXP)                                                                     \
  X(FLT_ROUNDS)                                                                         \
  X(DBL_MIN)                                                                            \
  X(DBL_TRUE_MIN)                                                                       \
  X(DBL_MAX)                                                                            \
  X(FE_TONEAREST)                                                                       \
  X(FE_UPWARD)                                                                          \
  X(FE_DOWNWARD)                                                                        \
  X(FE_TOWARDZERO)                                                                      \
  X(FLT_EVAL_METHOD)                                                                    \
  X(FLT_HAS_SUBNORM)

// Define all the str_ functions
MAP(FSTR)

#define INFO "Floating-point representation sandbox"
static char *infoLong = INFO
  ". This allows you to inspect the bit-level representation of "
  "floating point numbers. The utility of this is in "
  "being able to enter the floating point numbers in various ways, "
  "and in seeing the low-level representation of those values and "
  "arithmetic operations (especially sums) involving those values. "
  "Floats can be input in various formats (see below) on the command "
  "line. They can also can be specified in a separate file (one value "
  "per line), with the filename given at the end of the command-line, "
  "prefixed with \"@\". Lines starting with \"#\" are ignored. "
  "Separately from all this, "
  "\"./plotr fp -dist\" computes ULP-ish differences between pairs "
  "of values. Also separately, "
  "\"./plotr fp -defs\" reveals what various #define's are. ";

/* to learn about the surprisingly non-trivial details of getting
   nice-looking decimal expansions of floating-point numbers, see:
   https://www.ryanjuckett.com/printing-floating-point-numbers/
*/

// surefire way to get infinity
static float
fpInf(void) {
    float ff = INFINITY; // from math.h
    return ff;
}

// surefire way to get not-a-number ("quiet")
static float
fpNan(void) {
    float ff = fpInf();
    return ff / ff;
}

// print N bits of ii into str
static void
bitPrint(char *str, uint ii, uint N) {
    for (uint bi = 0; bi < N; bi++) {
        str[N - 1 - bi] = '0' + (ii & 1);
        ii /= 2;
    }
    str[N] = '\0';
}

// parse bits in str, save in *NP
static int
bitParse(const char *str, uint *NP) {
    uint ret = *NP = 0;
    uint cn = strlen(str);
    for (uint ci = 0; ci < cn; ci++) {
        if ('0' == str[ci]) {
            ret = 2 * ret + 0;
        } else if ('1' == str[ci]) {
            ret = 2 * ret + 1;
        } else {
            return 0;
        }
    }
    *NP = ret;
    return 1;
}

/* finds operator ('/', '*', '+', '-', '=') in string, taking
   care to ignore + and - that appear as exponents */
static char *
findop(char *str, char op) {
    char *ret;
    ret = strchr(str, op);
    if (ret && ('+' == op || '-' == op)) {
        if (ret > str) { // if not the first character (so not unary + and -)
            if (ret[1]   // if not the last character
                && ('e' == ret[-1] || 'E' == ret[-1])) {
                // look again past exponent
                ret = findop(ret + 1, op);
            }
        } else {
            ret = NULL;
        }
    }
    return ret;
}

#define FLOAT_STRLEN 1024
typedef struct {
    char name[FLOAT_STRLEN + 1]; // in case floats are named on command-line
    union {
        uint32_t i;
        float f;
    } u;
} Float;

/* HACK: nan payload to represent what is really the ulp of some previously parsed
   number, rather than a specific fixed value; hopefully it is not a payload that is
   needed for other uses */
#define ULP_PLD 23710

/* HACK to go with "ulp" handling; last float parsed, and whether a float has been
   previously parsed */
static float last = 0;
static int lastSet = 0;

/* parse float from str, save in *ptr. Does lots of error checking,
   and puts human-readable error messages in err */
static int
fpParseFun(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
    Float *FL = (Float *)ptr;
    strcpy((*FL).name, ""); // will be set to something in case of '='
    char *divs = findop(str, '/');
    char *muls = findop(str, '*');
    char *adds = findop(str, '+');
    char *subs = findop(str, '-');
    char *eqls = findop(str, '=');
    if (!strcmp(str, "inf") || !strcmp(str, "+inf")) {
        (*FL).u.f = fpInf();
    } else if (!strcmp(str, "-inf")) {
        (*FL).u.f = -fpInf();
    } else if (!strcmp(str, "nan")) {
        (*FL).u.f = fpNan();
    } else if (str == strstr(str, "nan:")) {
        const char *payS = str + 4;
        uint pay; // won't use pay; but parse it for error checking
        if (1 != sscanf(payS, "%u", &pay)) {
            sprintf(err, "%s: couldn't parse payload \"%s\" in \"%s\" as uint", __func__,
                    payS, str);
            return 1;
        }
        (*FL).u.f = pltNan(pay);
    } else if (!strcmp(str, "ulp")) {
        if (!lastSet) {
            sprintf(err, "%s: no previously parsed number: can't take its ulp",
                    __func__);
            return 1;
        }
        Float Incr;
        Incr.u.f = last;
        Incr.u.i++;
        (*FL).u.f = Incr.u.f - last;
    } else if (str == strstr(str, "0x")) {
        uint ui;
        if (1 != sscanf(str + 2, "%x", &ui)) {
            sprintf(err, "%s: couldn't parse \"%s\" (in \"%s\") as hex uint", __func__,
                    str + 2, str);
            return 1;
        }
        (*FL).u.i = ui;
    } else if (divs || muls || adds || subs || eqls) {
        char *_str = airStrdup(str);
        divs = findop(_str, '/');
        muls = findop(_str, '*');
        adds = findop(_str, '+');
        subs = findop(_str, '-');
        eqls = findop(_str, '=');
        uint numops = !!divs + !!muls + !!adds + !!subs + !!eqls;
        if (!(eqls && (eqls + 1 == subs || eqls + 1 == adds)) && 1 < numops) {
            sprintf(err,
                    "%s: can only have one of operations "
                    "/,*,+,-,= (not %u in \"%s\")",
                    __func__, numops, str);
            free(_str);
            return 1;
        }
        char *opc = (eqls ? eqls : (divs ? divs : (muls ? muls : (adds ? adds : subs))));
        if (findop(opc + 1, opc[0])) {
            sprintf(err, "%s: saw more than one %c (in \"%s\")", __func__, opc[0], str);
            free(_str);
            return 1;
        }
        *opc = '\0';
        opc++;
        char _err[AIR_STRLEN_HUGE];
        if (eqls) {
            // save the name, then parse the value
            airStrcpy((*FL).name, FLOAT_STRLEN + 1, _str);
            Float GL;
            if (fpParseFun(&GL, opc, _err)) {
                sprintf(err, "%s: couldn't parse \"%s\" as 2nd operand: %s", __func__,
                        opc, _err + strlen(__func__) + 2);
                free(_str);
                return 1;
            }
            (*FL).u.f = GL.u.f;
        } else {
            Float FAA;
            if (fpParseFun(&FAA, _str, _err)) {
                sprintf(err, "%s: couldn't parse \"%s\" as 1st operand: %s", __func__,
                        _str, _err + strlen(__func__) + 2);
                free(_str);
                return 1;
            }
            Float FBB;
            if (fpParseFun(&FBB, opc, _err)) {
                sprintf(err, "%s: couldn't parse \"%s\" as 2nd operand: %s", __func__,
                        opc, _err + strlen(__func__) + 2);
                free(_str);
                return 1;
            }
            // compute and save the result
            Float ULP;
            ULP.u.f = pltNan(ULP_PLD);
            float BB;
            if (FAA.u.i == ULP.u.i) {
                sprintf(err, "%s: only expect \"ulp\" as 2nd operand, not 1st",
                        __func__);
                free(_str);
                return 1;
            } else if (FBB.u.i == ULP.u.i) {
                /* have ulp as 2nd operand */
                if (divs || muls) {
                    sprintf(err, "%s: only expect + or - with ulp, not %c", __func__,
                            divs ? '/' : '*');
                    free(_str);
                    return 1;
                }
                Float Incr;
                Incr.u.i = FAA.u.i + 1;
                BB = Incr.u.f - FAA.u.f;
            } else {
                /* 2nd operand != ulp */
                BB = FBB.u.f;
            }
            float AA = FAA.u.f;
            (*FL).u.f = divs ? AA / BB : (muls ? AA * BB : (adds ? AA + BB : AA - BB));
        }
    } else if (strchr(str, ':')) {
        const char *col1 = strchr(str, ':');
        const char *col2 = strchr(col1 + 1, ':');
        if (!col2) {
            sprintf(err,
                    "%s: got only one \":\" in \"%s\" "
                    "need two to specify sign:expo:frac",
                    __func__, str);
            return 1;
        }
        // 11 ==  length of hex format: 1 + strlen(":") + 2 + strlen(":") + 6
        // 34 == length of bits format: 1 + strlen(":") + 8 + strlen(":") + 23
        int gotHex = (11 == strlen(str));
        int gotBit = (34 == strlen(str));
        if (!(gotHex || gotBit)) {
            sprintf(err,
                    "%s: sign:expo:frac should be length 11 (for hex) or "
                    "34 (for bits), not %u",
                    __func__, (uint)strlen(str));
            return 1;
        }
        uint sign, expo, frac;
        if (str == strstr(str, "1:")) {
            sign = 1;
        } else if (str == strstr(str, "0:")) {
            sign = 0;
        } else {
            sprintf(err,
                    "%s: sign:expo:frac must start w/ either \"0:\" or "
                    "\"1:\" (not \"%c%c\")",
                    __func__, str[0], str[1]);
            return 1;
        }
        // so first : is at str[1]; check on second :
        if (!((gotHex && 2 == col2 - col1 - 1) || (gotBit && 8 == col2 - col1 - 1))) {
            sprintf(err,
                    "%s: for sign:expo:frac as %s, need %u chars for "
                    "expo, not %u",
                    __func__, gotHex ? "hex" : "bits", gotHex ? 2 : 8,
                    (uint)(col2 - col1 - 1));
            return 1;
        }
        char mystr[35]; // known to be long enough, given above
        strcpy(mystr, str);
        mystr[col2 - str] = '\0';
        char *expoS = mystr + 2;
        char *fracS = mystr + (col2 + 1 - str);
        if (gotHex) {
            if (1 != sscanf(expoS, "%x", &expo)) {
                sprintf(err, "%s: trouble parsing exponent \"%s\" as hex", __func__,
                        expoS);
                return 1;
            }
            if (1 != sscanf(fracS, "%x", &frac)) {
                sprintf(err, "%s: trouble parsing fraction \"%s\" as hex", __func__,
                        fracS);
                return 1;
            }
            if (frac > 0x7fffff) {
                sprintf(err, "%s: fraction %u (\"%s\") too big for 23 bits", __func__,
                        frac, fracS);
                return 1;
            }
        } else {
            if (1 != bitParse(expoS, &expo)) {
                sprintf(err, "%s: trouble parsing exponent \"%s\" as bits", __func__,
                        expoS);
                return 1;
            }
            if (1 != bitParse(fracS, &frac)) {
                sprintf(err, "%s: trouble parsing fraction \"%s\" as bits", __func__,
                        fracS);
                return 1;
            }
        }
        // at last we have sign, expo, frac; build the float
        (*FL).u.i = (sign << 31) + (expo << 23) + frac;
    } else {
        // not inf, nan, hex, fraction, or sign:expo:frac, so try plain sscanf
        if (1 != sscanf(str, "%f", &((*FL).u.f))) {
            sprintf(err, "%s: couldn't parse \"%s\" as float", __func__, str);
            return 1;
        }
    }
    /* if we got here, we must have parsed something */
    last = (*FL).u.f;
    lastSet = 1;
    return 0;
}
static hestCB _fpParse = {sizeof(Float), "float",
                          /* we are missing some const-correctness that hest expects
                             fixing this would a re-write of fpParseFun so that it
                             worked with a copy of the given string */
                          (int (*)(void *, const char *, char *))fpParseFun, NULL};
static hestCB *fpParse = &_fpParse;

static void
fpExactVal(char *str, Float FL) {
    uint sign = (FL.u.i >> 31) & 1;
    uint expo = (FL.u.i >> 23) & 0xff;
    uint frac = FL.u.i & 0x7fffff;
    if (0 < expo && expo < 0xff) {
        // normalized
        sprintf(str, "(-1)^%u * 2^(%3u-127) * (1+%7u/2^23)", sign, expo, frac);
    } else if (0 == expo) {
        // denormalized
        sprintf(str, "(-1)^%u *  2^(1-127)  * (0+%7u/2^23)", sign, frac);
    } else {
        // expo == 0xff
        if (!frac) {
            sprintf(str, "(-1)^%u * inf", sign);
        } else {
            uint mask = ((uint)1 << 22) - 1;
            sprintf(str, "nan:%u", frac & mask);
        }
    }
    return;
}
// prints info about how ff is represented in floating point
static void
fpPrint(Float FL, int extra) {
    uint sign = (FL.u.i >> 31) & 1;
    uint expo = (FL.u.i >> 23) & 0xff;
    uint frac = FL.u.i & 0x7fffff;
    char expoS[9];
    bitPrint(expoS, expo, 8);
    char fracS[24];
    bitPrint(fracS, frac, 23);
    char exact[FLOAT_STRLEN + 1];
    fpExactVal(exact, FL);
    if (strlen(FL.name)) {
        /* if the float is named, we only print its exact numerical value
           (the same as is printed with "extra") since the idea is that
           this is going to be copied into Mathematica or the like */
        printf("%s = %s;\n", FL.name, exact);
    } else {
        if (extra) {
            printf("0x%08x = ", FL.u.i);
        }
        printf("%u:%02x:%06x = %u:%s:%s ", sign, expo, frac, sign, expoS, fracS);
        if (extra) {
            printf("= %s ", exact);
        }
        int equals;
        if (0xff == expo) {
            // special value
            equals = 1;
        } else {
            /* see if the decimal expansion sufficient to identify the 32-bit
               float, via printf("%.9g"), is actually equal to that float (at
               least in so far as a double can represent the decimal expansion)
               btw, why 9 in "%.9g": https://goo.gl/Sy5JKG */
            char buff[128];
            sprintf(buff, "%.9g", (double)FL.u.f);
            double dd;
            sscanf(buff, "%lg", &dd);
            equals = (dd == (double)FL.u.f);
        }
        printf("%c= %.9g\n", equals ? '=' : '~', (double)FL.u.f);
    }
}
#undef FLOAT_STRLEN

static int
fpMain(int argc, const char **argv, const char *me, hestParm *hparm) {
    if (pltRealIsDouble) {
        printf("%s: WARNING: This was compiled with real==double, but\n", me);
        printf("%s: WARNING: all the floating-point sandbox code here\n", me);
        printf("%s: WARNING: assumes real==float.\n", me);
        printf("%s: WARNING: Do \"make clean; make\" to fix this.\n", me);
    }
    airArray *mop = airMopNew();
    hestOpt *hopt = NULL;
    int doDefs;
    hestOptAdd(&hopt, "defs", NULL, airTypeInt, 0, 0, &doDefs, NULL,
               "instead of doing things with floating point values "
               "as controlled with the following arguments, print "
               "out the values of various #define's (if they have "
               "been #define'd)");
    int doDist;
    hestOptAdd(&hopt, "dist", NULL, airTypeInt, 0, 0, &doDist, NULL,
               "instead of doing things with floating point values "
               "as controlled with the following arguments, print "
               "out distances between values (as measured by "
               "pltFloatDist) after they've been moved away "
               "from zero by the -doff offset amount.");
    real distOff;
    hestOptAdd(&hopt, "doff", "off", airTypeReal, 1, 1, &distOff, "0",
               "when doing -dist distances, and if the two values are both "
               "positive or both negative, then move the values by this "
               "offset away from zero, which limits the resolution of the "
               "difference computation.");
    int doSum;
    hestOptAdd(&hopt, "s", NULL, airTypeInt, 0, 0, &doSum, NULL,
               "with this flag, running sum of values is printed "
               "along with the individual values");
    int doRSum;
    hestOptAdd(&hopt, "rs", NULL, airTypeInt, 0, 0, &doRSum, NULL,
               "this flag is like \"-s\", but then sum is also done "
               "in reverse, and results are compared. If there is a "
               "difference (i.e. a demonstrated non-associativity of "
               "FP addition), the difference is described.");
    int kbn;
    hestOptAdd(&hopt, "kbn", "do", airTypeInt, 1, 1, &kbn, "0",
               "if non-zero, also show results of Kahan and Babuska "
               "summation, as described by Neumaier. Higher values "
               "here show more intermediate results.");
    int extra;
    hestOptAdd(&hopt, "x", NULL, airTypeInt, 0, 0, &extra, NULL,
               "print extra info about FP representation: i.e. "
               "normalized vs denormalized vs nan vs inf. The "
               "normalized and denormalized forms (in which the last "
               "factor starts with \"(1+\" or \"(0+\", respectively) "
               "contain an exact expression for the represented value, "
               "which can be copied into a symbolic math program for "
               "comparison.");
    Float *ff;
    uint ffNum;
    hestOptAdd(&hopt, NULL, "f0", airTypeOther, 0, -1, &ff, NULL,
               "One or more 32-bit floating-point numbers, which may "
               "be entered in one of a few formats:\n "
               "\b\bo \"inf\", \"+inf\", \"-inf\", \"nan\": special values\n "
               "\b\bo \"ulp\": the \"unit in last place\" of the previously "
               "parsed number\n "
               "\b\bo nan:<payload>: NaN with fraction <payload>\n "
               "\b\bo 0x<hex>: 0x prefix hex representation of 32-bit uint\n "
               "\b\bo s:ee:ffffff: sign s=0/1, ee and ff are *hex* values "
               "of 8-bit exponent and 23-bit fraction field\n "
               "\b\bo s:eeeeeeee:fffffffffffffffffffffff: sign s=0/1, "
               "ee and ff are individual *bits* of 8-bit exponent and 23-bit "
               "fraction field\n "
               "\b\bo AopB: computed value operation op on two floats "
               "A and B; op can be +, -, *, or /. Use no spaces. A*B "
               "must be quoted so the shell doesn't expand *. Each of "
               "A and B should be one of the formats here but not AopB, "
               "nor with unary + or -, and maybe other corner cases.\n "
               "\b\bo <float>: something sscanf(\"%f\") can parse directly",
               &ffNum, NULL, fpParse);
    hparm->respFileEnable = AIR_TRUE;
    hestParseOrDie(hopt, argc, argv, hparm, me, infoLong, AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
    /* no simple way to bracket the main action here with setting
       and unsetting the rounding mode */
    pltRoundSet();

    if (doDefs) {
        printf("%s: listing the value of various #define's:\n", me);
        // Call all the printer functions and be done
#define PRINT(X) printf(#X " = %s\n", str_##X());
        MAP(PRINT)
        airMopOkay(mop);
        return 0;
    }

    // Else we do either distance measurement, or printing values
    if (doDist) {
        if (ffNum % 2) {
            fprintf(stderr,
                    "%s: for diff, need even number of values "
                    "(not %u)\n",
                    me, ffNum);
            airMopError(mop);
            return 1;
        }
        uint dnum = ffNum / 2;
        for (uint di = 0; di < dnum; di++) {
            float aa = (float)ff[di].u.f;
            float bb = (float)ff[di + dnum].u.f;
            if (distOff > 0 && aa * bb > 0) {
                float off = (aa > 0 ? 1 : -1) * distOff;
                aa += off;
                bb += off;
            }
            pltPrintf("%u = pltFloatDist(%g, %g)\n", pltFloatDist(aa, bb), (double)aa,
                      (double)bb);
        }
        airMopOkay(mop);
        return 0;
    }

    // Else we do our main purpose: printing values and operations on them
    char fmt[128], spc[128];
    // make printed numbers line up nicely, if reasonably possible
    if (ffNum <= 10) {
        strcpy(fmt, "ff[%u] = ");
        strcpy(spc, "");
    } else if (ffNum <= 100) {
        strcpy(fmt, "ff[%02u] = ");
        strcpy(spc, " ");
    } else if (ffNum <= 1000) {
        strcpy(fmt, "ff[%03u] = ");
        strcpy(spc, "  ");
    } else {
        strcpy(fmt, "ff[%u] = ");
        strcpy(spc, "");
    }
    float fsum = 0;
    // we always compute the KBN sums, but only print if its wanted
    /* KBN code based on https://github.com/JuliaLang/julia/issues/199; also
       part of https://en.wikipedia.org/wiki/Kahan_summation_algorithm */
    float kbnfs = ffNum ? ff[0].u.f : 0; // KBN forward sum
    float kbnc = 0;                      // KBN compensation
    for (uint fi = 0; fi < ffNum; fi++) {
        if (!strlen(ff[fi].name)) printf(fmt, fi);
        fpPrint(ff[fi], extra);
        fsum += ff[fi].u.f;
        if (fi) { // on all but first iteration
            float t = kbnfs + ff[fi].u.f;
            if (fabsf(kbnfs) >= fabsf(ff[fi].u.f))
                kbnc += (kbnfs - t) + ff[fi].u.f;
            else
                kbnc += (ff[fi].u.f - t) + kbnfs;
            kbnfs = t;
            if (fi == ffNum - 1) { // last time through
                kbnfs += kbnc;
                kbnc = 0;
            }
        }
        if ((doSum || doRSum) && fi) {
            Float TF;
            strcpy(TF.name, "");
            TF.u.f = fsum;
            printf("%s  sum = ", spc);
            fpPrint(TF, extra);
            if (kbn) {
                if (kbn > 1 || (kbn && fi == ffNum - 1)) {
                    TF.u.f = kbnfs;
                    printf("%sKBNsum= ", spc);
                    fpPrint(TF, extra);
                    if (kbn > 2) {
                        TF.u.f = kbnc;
                        printf("%sKBNcomp=", spc);
                        fpPrint(TF, extra);
                    }
                }
            }
            printf("\n");
        }
    }
    if (doRSum && ffNum > 1) {
        printf("\nIN REVERSE:\n");
        float rsum = 0;
        long double sumD = 0;
        float kbnrs = ff[ffNum - 1].u.f; // KBN reverse sum
        kbnc = 0;
        for (uint fi = ffNum; fi-- > 0;) { // note loop structure
            if (!strlen(ff[fi].name)) printf(fmt, fi);
            fpPrint(ff[fi], extra);
            rsum += ff[fi].u.f;
            sumD += (long double)ff[fi].u.f;
            if (fi < ffNum - 1) {
                float t = kbnrs + ff[fi].u.f;
                if (fabsf(kbnrs) >= fabsf(ff[fi].u.f))
                    kbnc += (kbnrs - t) + ff[fi].u.f;
                else
                    kbnc += (ff[fi].u.f - t) + kbnrs;
                kbnrs = t;
                if (!fi) { // list time through
                    kbnrs += kbnc;
                    kbnc = 0;
                }
                Float TF;
                strcpy(TF.name, "");
                TF.u.f = rsum;
                printf("%s rsum = ", spc);
                fpPrint(TF, extra);
                if (kbn > 1 || (kbn && !fi)) {
                    TF.u.f = kbnrs;
                    printf("KBNrsum=%s", spc);
                    fpPrint(TF, extra);
                    if (kbn > 2) {
                        TF.u.f = kbnc;
                        printf("KBNcomp=%s", spc);
                        fpPrint(TF, extra);
                    }
                }
                printf("\n");
            }
        }
        printf("\n");
        if (fsum == rsum) {
            printf("(no difference in sums)\n");
            if (kbn) {
                if (kbnrs == kbnfs) {
                    printf("(no difference in KBN sums; estimated correct "
                           "answer: %.17Lg)\n",
                           sumD);
                } else {
                    printf("KBN sums were different\n");
                }
            }
        } else {
            Float S;
            S.u.f = fsum;
            Float R;
            R.u.f = rsum;
            uint expoS = (S.u.i >> 23) & 0xff;
            uint fracS = S.u.i & 0x7fffff;
            uint expoR = (R.u.i >> 23) & 0xff;
            uint fracR = R.u.i & 0x7fffff;
            if (expoS == expoR) {
                uint fdiff = fracS > fracR ? fracS - fracR : fracR - fracS;
                printf("difference from sum order = %u in fraction bits\n", fdiff);
            } else {
                printf("difference from sum order: "
                       "s:expo:frac %u:%x:%x vs %u:%x:%x\n",
                       S.u.i >> 31, expoS, fracS, R.u.i >> 31, expoR, fracR);
            }
            printf("  or, %.17Lg %% of estimated correct answer: %.17Lg\n",
                   100 * fabsl(((long double)fsum - (long double)rsum) / sumD),
                   sumD);
            if (kbn) {
                printf("   vs %.17Lg %% difference with KBN summation\n",
                       100 * fabsl(((long double)kbnfs - (long double)kbnrs) / sumD));
            }
        }
    }

    pltRoundUnset();
    airMopOkay(mop);
    return 0;
}

unrrduCmd plt_fpCmd = {"fp", INFO, fpMain, AIR_FALSE};
