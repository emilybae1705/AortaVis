#!/usr/bin/env python3
#
# plt and plotr: SciVis-2023 Project 1
# Copyright (C)  2023 University of Chicago. All rights reserved.
#
# This is only for students and instructors in the 2023 CMSC 23710/33710
# ("SciVis") class, for use in that class. It is not licensed for open-source
# or any other kind of re-distribution. Do not allow this file to be copied
# or downloaded by anyone outside the 2023 SciVis class.
#

"""
Project 1 utility for random rounding floating-point arithmetic,
written by GLK with ❤️ for teaching SciVis.
More complete info via ./rr.py --help
"""
import os
import re
import argparse
from collections import Counter
from pathlib import Path
from enum import Enum


class ArgTypeMixin(Enum):
    """Help argparse parse an enum"""

    # copied/adapted from:
    # https://gist.github.com/ptmcg/23ba6e42d51711da44ba1216c53af4ea"""
    @classmethod
    def argtype(cls, vstr: str) -> Enum:
        """tries to interpret vstr as element of enum"""
        values = [v.name.lower() for v in cls]
        if vstr.lower() in values:
            return cls[vstr.upper()]
        # else
        choices = ', '.join(values)
        raise argparse.ArgumentTypeError(
            f'{vstr!r} is not in {cls.__name__} enum; ' f'choices are: {choices}'
        )

    def __str__(self):
        """string representation of enum value"""
        return self.name.lower()


class MODE(ArgTypeMixin, Enum):
    """The different purposes that this tool serves."""

    CMD = 1  # for processing plotr.c
    CC = 2   # for processing other .c source files
    ASM = 3  # for processing .s assembly files


def swap_at(lines, mark, frag):
    """at first occurence of mark in lines, insert frag (as split into lines)"""
    idx = lines.index(mark)
    lines.pop(idx)
    for line in reversed(frag.splitlines()):
        lines.insert(idx, line)


def cmd_go(lines):
    """How to transform plotr.c to do random rounding: add buffer and code initialize it"""
    swap_at(
        lines,
        '#define RR_DOING 0',
        r"""#define RR_DOING 1
// a (static) global to be modified by marker functions
static int RR_depth = 0;
// Hopefully this is a simple way of making a function that won't get optimized away;
// this means "student code beginning, in a function"
int
pltStucodeBeginFunc() {
    return ++RR_depth;
}
// student code ending, in a function
int
pltStucodeEndFunc() {
    return --RR_depth;
}
#define RR_NUM   (1u<<16)
// RR_flcw[] = global buffer of floating point control words, which determine rounding
unsigned short RR_flcw[RR_NUM];
// RR_idx = current index into RR_flcw, in [0,RR_NUM)
unsigned short RR_idx;
// RR_swap = place for storing register used in setting rounding
int RR_swap;
// RR_seed = seed for initiatializing RR_flcw[]
unsigned int RR_seed;
// initialize RR_flcw based on RR_seed, re-zero RR_idx
static void
_RR_init(int verb) {
    RR_idx = 0;
    /* special behaviors for certain values of seed:
    0: not valid, triggers usage message
    1: all 1
    2: all 0
    3: 1, 0, 1, 0, 1, ...
    4: 0, 1, 0, 1, 0, ...
    */
    if (1 <= RR_seed && RR_seed <= 4) {
        for (uint ii = 0; ii < RR_NUM; ii++) {
            switch (RR_seed) {
            case 1:
                RR_flcw[ii] = 1;
                break;
            case 2:
                RR_flcw[ii] = 0;
                break;
            case 3:
            case 4:
                RR_flcw[ii] = (ii + RR_seed) % 2;
                break;
            }
        }
    } else {
        airRandMTState *rng = airRandMTStateNew(RR_seed);
        for (uint ii = 0; ii < RR_NUM; ii++) {
            // pick out one bit to use for rounding direction
            RR_flcw[ii] = !!(4 & airUIrandMT_r(rng));
        }
        airRandMTStateNix(rng);
    }
    if (verb) {
        printf("%s: %u values from seed %u:\n", __func__, RR_NUM, RR_seed);
        for (uint ii = 0; ii < RR_NUM; ii++) {
            printf(" %u", RR_flcw[ii]);
            if (ii && !((ii+1) % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }
    return;
}
""",
    )
    return lines


def indent(line):
    """returns the leading whitespace in a given line"""
    if match := re.match(r'^(\s*)\S', line):
        ret = match.group(1)
    else:
        ret = ''
    return ret


def cc_go(ilines, fname):
    """How to transform .c files (besides plotr.c) to do random rounding"""
    # top-level student code blocks (for helper functions) have to be marked
    # in a way that is visible in the generated assembly
    stem = Path(fname).stem
    beg_mark = '// v.v.v.v.v.v.v.v.v.v.v.v.v.v.v  begin student code'
    end_mark = "// ^'^'^'^'^'^'^'^'^'^'^'^'^'^'^  end student code"
    # declare the functions defined in plotr.c and used below
    olines = [
        # these functions are called to demarcate student code within functions
        'extern int pltStucodeBeginFunc(void);',
        'extern int pltStucodeEndFunc(void);',
        # HEY declaring these helps debug some assembly, but may not be needed for asm_go()
        'extern unsigned short RR_flcw[];',
        'extern unsigned short RR_idx;',
        'extern int RR_swap;',
    ]
    for idx, iline in enumerate(ilines):
        if beg_mark in iline:
            if iline.startswith(beg_mark):
                # We create a function with a unique name, detectable in the assembly,
                # to say: "student code block is beginning, at top-level scope of file"
                oline = f'int pltStucodeBeginFile_{stem}_{idx+1}(void) {{ return {idx+1};}}'
            else:
                # We make function call, detectable in the assembly, to say:
                # "student code block is beginning, within an existing function's scope"
                oline = indent(iline) + 'pltStucodeBeginFunc();'
        elif end_mark in iline:
            if iline.startswith(end_mark):
                # student code block end, at top-level scope of file
                oline = f'int pltStucodeEndFile_{stem}_{idx+1}(void) {{ return {idx+1};}}'
            else:
                # student code block end, within function scope
                oline = indent(iline) + 'pltStucodeEndFunc();'
        else:
            # line does not contain either beginning or ending student code marker
            oline = iline
        olines.append(oline)
    return olines


def count_print(dict):
    for (cnt, wut) in sorted([(cnt, wut) for (wut, cnt) in dict.items()], reverse=True):
        print(f'   ({cnt}) "{wut}"')


def expand_x(instructs):
    """expand one or two letter size suffixes of instructions"""
    ret = []
    for inst in instructs:
        if inst.endswith('XX'):
            # this was expanded only as needed to describe instructions seen in assembly
            ret += [f'{inst[:-2]}{xx}' for xx in ['bl', 'wl', 'lq']]
        elif inst.endswith('X'):
            ret += [f'{inst[:-1]}{x}' for x in ['b', 'w', 'l', 'q']]
        else:
            ret.append(inst)
    return ret


def expand_c(instructs):
    """expand the various condition codes within instructions"""
    ret = []
    for inst in instructs:
        if 'C' in inst:
            ret += [
                inst.replace('C', c)
                for c in [
                    'ae',
                    'a',
                    'b',
                    'g',
                    'l',
                    'le',
                    'be',
                    'ge',
                    'e',
                    'ne',
                    'p',
                    'np',
                    's',
                    'ns',
                    'o',
                ]
            ]
        else:
            ret.append(inst)
    return ret


def asm_go(ilines):
    """How to transform .s (assembly) files to do random rounding; this is where
    the work of injecting rounding mode changes actually happens."""
    # NOTE that everything at https://www.felixcloutier.com/x86 is in Intel syntax
    # instruction suffixes:
    #   b byte
    #   w word (2 bytes, 16 bit)
    #   l long or doubleword (4 bytes 32, bit)
    #   q quadword (8 bytes, 64 bit)
    # fp = floating-point
    # sp = single-precision, dp = single-precision
    # notfpa: instructions that do not do *arithmetic* on floating point values
    # notfpa_suff: notfpa but before XX, X, C suffix expansion
    notfpa_suff = [
        'movX',  # move https://www.felixcloutier.com/x86/mov
        'movd',  # move https://www.felixcloutier.com/x86/movd:movq
        'movdqa',  # move aligned double quadword https://c9x.me/x86/html/file_module_x86_id_183.html
        'movzXX',  # move zero-extended https://web.stanford.edu/class/archive/cs/cs107/cs107.1166/guide_x86-64.html
        'movsXX',  # move sign-extended
        'movdqu',  # move unaligned double quadword https://c9x.me/x86/html/file_module_x86_id_184.html
        'movabsq',  # https://stackoverflow.com/questions/40315803/difference-between-movq-and-movabsq-in-x86-64
        'pmovzxdq',  # one of the Packed Move with Zero Extend family https://www.felixcloutier.com/x86/pmovzx
        'pextrd',  # extract value from src XMM reg at given offset https://www.felixcloutier.com/x86/pextrb:pextrd:pextrq
        'pinsrd',  # insert double int https://www.felixcloutier.com/x86/pinsrb:pinsrd:pinsrq
        'pblendw',  # words from src op conditionally written to dest op, depending on imm op https://www.felixcloutier.com/x86/pblendw
        'btl',  # selects bit specified by 1st op at pos designated by 2nd op, store in CF https://www.felixcloutier.com/x86/bt.html
        # cmov = conditional moves https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/6mkuavhs7/index.html
        #            see also https://c9x.me/x86/html/file_module_x86_id_34.html
        'cmovCX',
        'pushq',  # push quad on stack
        'popq',  # pop from stack into quad
        'callq',  # proc call, pushing quad return addr https://www.felixcloutier.com/x86/call
        'retq',  # proc return, popping quad return addr
        # conditional jumps http://unixwiz.net/techtips/x86-jumps.html
        'jC',
        'jmp',  # jump
        'jmpq',  # jump to quad address
        # set on condition https://www.felixcloutier.com/x86/setcc.html
        #          http://www.mathemainzel.info/files/x86asmref.html
        #          https://cs.brown.edu/courses/cs033/docs/guides/x64_cheatsheet.pdf
        'setC',  # sets byte to 1
        'cmpX',  # sets condition codes from integer compare
        'pcmpeqd',  # compare packed data for equal https://www.felixcloutier.com/x86/pcmpeqb:pcmpeqw:pcmpeqd
        'testX',  # sets condition codes from integer and
        'rorb',  # rotate bits right https://www.felixcloutier.com/x86/rcl:rcr:rol:ror
        'notX',  # bit negation
        'orX',  # bitwise or
        'xorX',  # bitwise xor
        'pxor',  # https://www.felixcloutier.com/x86/pxor
        'andX',  # bitwise and
        'incX',  # increment by one
        'decX',  # decrement by one
        'addX',  # integer add
        'paddd',  # add packed doubleword integers https://c9x.me/x86/html/file_module_x86_id_226.html
        'negX',  # two's complement negation https://www.felixcloutier.com/x86/neg
        'subX',  # integer subtraction
        'sbbX',  # integer subtraction with borrow https://c9x.me/x86/html/file_module_x86_id_286.html
        'psubd',  # packed integer subtraction https://www.felixcloutier.com/x86/psubb:psubw:psubd
        'leaX',  # load eff. addr of source (into typed dest) https://www.felixcloutier.com/x86/lea.html
        'shlX',  # logical (unsigned) left shift (multiply) https://hackmd.io/@paolieri/x86_64  <-- nice page
        'sarX',  # arithmetic (signed) right shift (divide) https://en.wikibooks.org/wiki/X86_Assembly/Shift_and_Rotate
        'psrad',  # packed arithmetic (signed) right shift (divide) https://www.felixcloutier.com/x86/psraw:psrad:psraq
        'shrX',  # logical (unsigned) right shift (divide)
        'psrld',  # shift packed data right logical https://www.felixcloutier.com/x86/psrlw:psrld:psrlq
        'pshufd',  # shuffle packed doublewords https://www.felixcloutier.com/x86/pshufd.html
        'imulX',  # signed int multiply
        'mulX',  # unsigned int multiply # https://www.felixcloutier.com/x86/mul
        'pmulld',  # multiply packed integers https://www.felixcloutier.com/x86/pmulld:pmullq
        'divl',  # integer divide on longs
        'pmaxud',  # maximum of packed uints https://www.felixcloutier.com/x86/pmaxud:pmaxuq
        ##################### fp-related, but not fp arithmetic (so not dependent on rounding mode?)
        'wait',  # check for and handle pending, unmasked, fp exceptions https://www.felixcloutier.com/x86/wait:fwait
        'movss',  # move scalar sp fp value https://www.felixcloutier.com/x86/movss
        'movsd',  # move scalar dp fp value https://www.felixcloutier.com/x86/movsd
        #             what's it for with real==float ???
        'movaps',  # move aligned packed sp fp https://www.felixcloutier.com/x86/movaps.html'
        'movups',  # move unaligned packed sp fp https://www.felixcloutier.com/x86/movups
        'movlps',  # move low packed sp fp https://www.felixcloutier.com/x86/movlps
        'movlpd',  # move low packed dp fp https://www.felixcloutier.com/x86/movlpd
        'movapd',  # move aligned packed dp fp https://www.felixcloutier.com/x86/movapd
        'movupd',  # move unaligned packed dp fp (non-strict ???) https://www.felixcloutier.com/x86/movupd
        #             what are these three for with real==float ???
        'movshdup',  # move odd index sp fp https://www.felixcloutier.com/x86/movshdup
        'movsldup',  # (non-strict ???) move even index sp fp https://www.felixcloutier.com/x86/movsldup
        'shufps',  # packed interleave shuffle of quadruplets of sp fp https://www.felixcloutier.com/x86/shufps
        'insertps',  # insert scalar sp fp https://www.felixcloutier.com/x86/insertps
        'blendps',  # blend packed sp fp https://www.felixcloutier.com/x86/blendps
        'blendvps',  # variable blend packed sp fp https://www.felixcloutier.com/x86/blendvps
        #             "Conditionally copy each dword of sp fp from 2nd source op and 1st source op depending on mask reg op"
        'fstps',  # store fp value https://c9x.me/x86/html/file_module_x86_id_117.html
        'fldt',  # load 80-bit real to stack top http://web.mit.edu/rhel-doc/3/rhel-as-en-3/i386-float.html
        #         but why is this being used with real==float ???
        'cvttss2si',  # convert sp fp to signed double int using truncation https://www.felixcloutier.com/x86/cvttss2si
        #       " When a conversion is inexact, a truncated (round toward zero) result is returned"
        'cvtss2sd',  # convert sp fp to dp https://staffwww.fullcoll.edu/aclifton/cs241/lecture-floating-point-simd.html
        #              (seems to be part of calling printf)
        'cvtsd2ss',  # convert scalar dp fp to sp fp https://www.felixcloutier.com/x86/cvtsd2ss
        #              what's it for with real==float ???
        'cvtsi2ss',  # convert double int to scalar sp fp https://www.felixcloutier.com/x86/cvtsi2ss.html
        'cvtsi2ssl',  # not sure ???
        'cvtpd2ps',  # (non-strict??) convert packed dp fp to packed sp fp https://www.felixcloutier.com/x86/cvtpd2ps
        'cvtsi2sd',  # convert signed double int to dp fp https://www.felixcloutier.com/x86/cvtsi2sd
        #             what are these two for with real==float ???
        'cvtsi2sdq',  # convert quad int to dp https://www.cs.cmu.edu/~fp/courses/15411-f14/misc/asm64-handout.pdf
        #             what's it for with real==float ???
        'cvtsi2ssq',  # convert quad int to sp https://csapp.cs.cmu.edu/public/waside/waside-sse.pdf
        'comiss',  # sp fp ordered compare w/ possibility of NaN https://www.felixcloutier.com/x86/comiss
        'ucomiss',  # sp fp unordered compare, with possibility of NaN https://www.felixcloutier.com/x86/ucomiss
        #           role of comiss vs ucomiss ???
        'comisd',  # dp fp compare https://www.felixcloutier.com/x86/comisd
        # "COMISD differs UCOMISD in that it signals SIMD fp invalid op exception (#I) with NaN operand"
        'cmpneqss',  # (non-strict ???) compare scalar sp fp https://www.felixcloutier.com/x86/cmpss
        'cmpneqps',  # (non-strict ???) compare packed sp fp https://www.felixcloutier.com/x86/cmpps
        'cmpltps',  # (non-strict ???) another compare packed sp fp
        #            https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/epmph/index.html
        'ucomisd',  # dp fp unordered compare, with possibility of NaN https://www.felixcloutier.com/x86/ucomisd
        #             HEY: looks like "ucomisd %xmm1, %xmm1" is part of __inline_isfinited but WHY is dp part of real=float ???
        'orps',  # (non-strict ???) bitwise logical OR of packed sp fp https://www.felixcloutier.com/x86/orps
        'xorps',  # bitwise XOR of packed sp fp https://www.felixcloutier.com/x86/xorps
        #           curious about what this is used for, especially if src != dest ???
        'andps',  # bitwise AND of packed sp fp https://www.felixcloutier.com/x86/andps
        #           seems to be from __inline_isfinitef ???
        'andpd',  # bitwise AND of packed dp fp https://www.felixcloutier.com/x86/andpd
        #           what's it for with real==float ???
        'roundss',  # round (to int) packed sp fp, with GIVEN rounding mode https://www.felixcloutier.com/x86/roundss
        #        (seem to be part of floor) ??? depends on fp environment current rounding mode?
        'minss',  # (non-strict ???) min of scalar sp fp https://www.felixcloutier.com/x86/minss
        'maxss',  # (non-strict ???) max of scalar sp fp https://www.felixcloutier.com/x86/maxss
    ]
    # expand the size suffixes
    notfpa = expand_c(expand_x(notfpa_suff))
    # yesfpa: yes doing fp arithmetic for which rounding mode matters
    yesfpa = [
        'addss',  # sp fp add https://www.felixcloutier.com/x86/addss
        'addps',  # (non-strict) packed sp fp add https://www.felixcloutier.com/x86/addps
        'subss',  # sp fp subtract https://www.felixcloutier.com/x86/subss
        'subps',  # (non-strict) packed sp fp subtract https://www.felixcloutier.com/x86/subps
        'mulss',  # sp fp multiply https://www.felixcloutier.com/x86/mulss
        'mulps',  # (non-strict) packed sp fp multiply https://www.felixcloutier.com/x86/mulps
        'divss',  # sp fp divide https://www.felixcloutier.com/x86/mulss
        'divps',  # (non-strict) packed sp fp divide https://www.felixcloutier.com/x86/divps'
        'sqrtss',  # square root of sp fp https://www.felixcloutier.com/x86/sqrtss
    ]
    directs = []
    instructs = []
    labels = []
    olines = []
    for iline in ilines:
        if not iline:
            # some lines are blank
            continue
        if iline.startswith('\t'):
            fields = iline.split('\t')[1:]
            if fields[0].startswith('#'):
                # ignoring comments
                continue
            if fields[0].startswith('.'):
                # it's a directive
                directs.append(fields[0].split()[0])
            else:
                # it's an instruction
                inst = fields[0]
                if not (inst in notfpa or inst in yesfpa):
                    instructs.append(inst)
        elif re.match(r'^ *#', iline):
            # some comments don't start with tab
            continue
        elif (
            iline.startswith('.set ')
            or iline.startswith('.subsections_via_symbols')
            or iline.startswith('.zerofill ')
        ):
            # directives that don't seem to start with \t tab
            directs.append(iline.split()[0])
        elif re.match(r'^\S*:', iline):
            labels.append('.set')
        else:
            # this is weird
            print(f'\n????????? |{iline}|\n')
        olines.append(iline)
    # https://stackoverflow.com/questions/20510768/count-frequency-of-words-in-a-list-and-sort-by-frequency
    # print('directives:')
    # count_print(Counter(directs))
    if Counter(instructs):
        print('instructions:')
        count_print(Counter(instructs))
        exit(0)
    return olines


# counts = Counter(list1)
# print(counts)
## Counter({'apple': 3, 'egg': 2, 'banana': 1})
def rr_go(args):
    """calls the filtering function, depending on mode"""
    # (on GLK's laptop) clang -S is generating invalid utf-8 characters 0x80
    # which causes "fin.readlines()" to choke with, e.g.
    # UnicodeDecodeError: 'utf-8' codec can't decode byte 0x80 in position 6435: invalid start byte
    # https://stackoverflow.com/questions/19699367/for-line-in-results-in-unicodedecodeerror-utf-8-codec-cant-decode-byte
    # suggests using a different encoding: ISO-8859-1, and that seems to work
    # but then removed the problem by using "tr" within Makefile
    enc = 'utf-8'
    with open(args.in_fname, 'r', encoding=enc) as fin:
        lines = [line.rstrip() for line in fin.readlines()]
    if MODE.CMD == args.mode:
        lines = cmd_go(lines)
    elif MODE.CC == args.mode:
        lines = cc_go(lines, args.in_fname)
    elif MODE.ASM == args.mode:
        lines = asm_go(lines)
    with open(args.out_fname, 'w', encoding=enc) as fout:
        for line in lines:
            print(line, file=fout)


def check_args(args):
    """does as much error checking as possible on given args"""
    if not os.path.isfile(args.in_fname):
        raise RuntimeError(f'Input "{args.in_fname}" is not a file')


def parse_args():
    """
    Set up and run argparse command-line parser
    """
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Project 1 utility for implementing '
        'random rounding floating-point arithmetic via various C '
        'and assembly transformations',
        formatter_class=argparse.RawTextHelpFormatter,
    )
    # formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-v',
        metavar='verbosity',
        type=int,
        default=1,
        required=False,
        help='Verbosity level (use "-v 0" for silent operation)',
    )
    parser.add_argument(
        'mode',
        type=MODE.argtype,
        choices=MODE,
        help='The mode in which this is running, depending on what kind of '
        'input file is given; the choices are:\n'
        '"cc": for modifying .c files other than plotr.c\n'
        '"cmd": for modifying plotr.c in particular\n'
        '"asm": for translating one .s assembly file into another.\n',
    )
    parser.add_argument('in_fname', metavar='input', type=str, help='input filename')
    parser.add_argument('out_fname', metavar='output', type=str, help='output filename')
    return parser.parse_args()


if __name__ == '__main__':
    ARGS = parse_args()
    VERB = ARGS.v
    check_args(ARGS)
    rr_go(ARGS)
