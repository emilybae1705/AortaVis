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

# TODO: reconsider if the paired (line string, line index) representation is
# really the best way to do this

# pylint doesn't like the number of error-checking conditionals in check_args
# but how else to do them all, and all in one place?
# pylint: disable=too-many-branches
"""
Project 1 utility for splicing student code blocks between files and directories.
"""
import os
import argparse


# all the files (for this project) with student code blocks
# (per-project info is inserted here by a simplistic string substitution,
# which is the only reason a list literal is not used)
PFILES = 'util.c poly.c convo.c pplot.c plt.h'.split()
# marks beginning of student code block
CB_BEG = r'.v.v  begin student code'
# marks end of student code block
CB_END = r"'^'^  end student code"


def read_lines(fname):
    """
    reads lines from file, returning list of (line,index) pairs
    where index is the 0-based line number
    """
    lin_idx = []  # (line,index) pairs
    begs = []  # indices of lines containing CB_BEG
    ends = []  # indices of lines containing CB_END
    lines = []   # input lines, possibly filtered
    with open(fname, 'r', encoding='utf-8') as file:
        for line in file.readlines():
            lines.append(line.rstrip())
    for index, line in enumerate(lines):
        lin_idx.append((line, index))
        if CB_BEG in line:
            begs.append(index)
        if CB_END in line:
            ends.append(index)
    if len(begs) != len(ends):
        raise RuntimeError(
            f'File "{fname}" had {len(begs)} beginnings but '
            f'{len(ends)} ends of student code blocks'
        )
    for bki, _ in enumerate(begs):
        if not begs[bki] < ends[bki]:
            raise RuntimeError(
                f'Code block {1+bki} seems to end (line {1+ends[bki]}) '
                f'before it begins (line {1+begs[bki]})'
            )
        if bki and not begs[bki] > ends[bki - 1]:
            raise RuntimeError(
                f'Code block {1+bki} seems to begin (line {1+begs[bki]}) '
                f'before previous block ends (line {1+ends[bki-1]})'
            )
    return (lin_idx, begs, ends)


def splice_how(args, dst_fn, src_fn):
    """
    Returns (dst_fn, content) where content is the list of lines to save to filename dst_fn.
    We don't actually do the splicing here, because we want to first make sure all splices
    can succeed before any new files are written.
    """
    (src_ln, src_begs, src_ends) = read_lines(src_fn)
    (dst_ln, dst_begs, dst_ends) = read_lines(dst_fn)
    # know the #begs == #ends for both src and dst, but do they match between src and dst?
    if len(src_begs) != len(dst_begs):
        raise RuntimeError(
            f'Saw {len(src_begs)} student code blocks in source "{src_fn}" '
            + f'but {len(dst_begs)} blocks in destination "{dst_fn}"'
        )
    bnum = len(src_begs)  # == len(dst_begs)
    if args.b and args.b > bnum:
        raise RuntimeError(
            f'Want to splice block {args.b} but there '
            f'{"are" if bnum > 1 else "is"} only '
            f'{len(src_begs)} block{"s" if bnum > 1 else ""} in {src_fn}'
        )
    if not ARGS.imm:  # so, do look for mis-matches in code block beginnings
        for bki in range(bnum):
            if args.b and bki + 1 != args.b:  # only doing one block, and, this isn't it
                continue
            sln = src_ln[src_begs[bki]]
            dln = dst_ln[dst_begs[bki]]
            if not dln[0] == sln[0]:
                raise RuntimeError(
                    f'Mismatch in lines beginning student code block {1+bki}:\n'
                    + f'source: "{sln[0]}" ({src_fn} line {1+sln[1]})\n'
                    + f'  dest: "{dln[0]}" ({dst_fn} line {1+dln[1]})'
                )
    changed = False  # set to True if we actually changed anything
    dst_add = 0  # output offset: add to dst_begs[bki] to get from orig index to real index
    # pylint wants there to be fewer variables; GLK unsure how to do that
    for bki in range(bnum):
        if args.b and bki + 1 != args.b:  # only doing one block, and, this isn't it
            continue
        dbi = dst_add + dst_begs[bki] + 1
        dei = dst_add + dst_ends[bki]
        sbi = src_begs[bki] + 1
        sei = src_ends[bki]
        if VERB > 1:
            print(
                f'block {bki}: destination "{dst_fn}" lines [{dbi},{dei}) '
                + f'<-- source "{src_fn}" lines [{sbi},{sei})'
            )
        src_blk = src_ln[sbi:sei]
        if [ln[0] for ln in dst_ln[dbi:dei]] != [ln[0] for ln in src_blk]:
            changed = True
        del dst_ln[dbi:dei]  # remove existing block in destination
        dst_ln[dbi:dbi] = src_blk  # insert block from source
        dst_add += (sei - sbi) - (dei - dbi)  # offset for finding next block in dest
    return dst_ln if changed else None


def find_save_fn(fn_ex, suff):
    """
    find an unused filename for saving back-up copy of destination
    """
    fnspl = os.path.splitext(fn_ex)
    bfn = fnspl[0]
    ext = fnspl[1]
    while True:
        bfn = f'{bfn}-{suff}'
        try_fn = f'{bfn}{ext}'
        if not os.path.exists(try_fn):
            break
        # else try_fn already exists; try again
    return try_fn


def doit(args, ondir):
    """
    Calls splice_how on individual files to learn how to splice what needs to be spliced,
    and then saves out results of all splices.
    """
    todo = []
    if ondir:
        for fname in PFILES:
            todo.append(
                (
                    f'{args.dst}/{fname}',
                    splice_how(args, f'{args.dst}/{fname}', f'{args.src}/{fname}'),
                )
            )
    else:
        todo.append((args.dst, splice_how(args, args.dst, args.src)))
    # having learned how to do the splicing, now do it
    for fn_ln in todo:
        if not fn_ln[1]:
            # some file wasn't actually changed
            if VERB:
                print(f'destination "{fn_ln[0]}" unchanged')
            continue
        # file fn_ln[0] needs to change
        if 'NO' != args.s:
            save_fn = find_save_fn(fn_ln[0], args.s)
            if VERB:
                print(f'copying original destination "{fn_ln[0]}" to "{save_fn}"')
            os.rename(fn_ln[0], save_fn)
        with open(fn_ln[0], 'w', encoding='utf-8') as file:
            for lin_idx in fn_ln[1]:
                print(lin_idx[0], file=file)
        if VERB:
            print(f'saved new destination "{fn_ln[0]}"')


def check_args(args):
    """
    Does all the error checking on the given command-line args.
    As a sneaky convenience, returns boolean for "we're running on whole directories"
    """
    if not args.s:
        raise RuntimeError('Need non-empty string for "-s" command-line option')
    # dst_dir: destination args.dst is a directory
    if not (dst_dir := os.path.isdir(args.dst)):
        if not os.path.isfile(args.dst):
            raise RuntimeError(f'Destination "{args.dst}" neither director nor file')
    # src_dir: source args.src is a directory
    if not (src_dir := os.path.isdir(args.src)):
        if not os.path.isfile(args.src):
            raise RuntimeError(f'Source "{args.src}" neither director nor file')
    if args.dst == args.src:
        raise RuntimeError(
            f'Destination "{args.dst}" and source "{args.src}" must ' 'be different'
        )
    if 1 == dst_dir + src_dir:  # must either 0, 1, or 2
        raise RuntimeError(
            f'Destination "{args.dst}" and source "{args.src}" need '
            'to either both be files or both be directories'
        )
    if src_dir:  # check that all PFILES are there, with correct permissions
        if args.b:  # want to splice a single block
            raise RuntimeError(
                f'Splicing a single block with "-b {args.b}" only possible '
                'with source and destination as files, not directories.'
            )
        for fname in PFILES:
            if not fname:
                raise RuntimeError('Got empty filename in PFILES')
            if not os.path.isfile(f'{args.src}/{fname}'):
                raise RuntimeError(f'Source directory "{args.src}" missing file "{fname}"')
            if not os.access(f'{args.src}/{fname}', os.R_OK):
                raise RuntimeError(f'Source file "{args.src}/{fname}" not readable')
            if not os.path.isfile(f'{args.dst}/{fname}'):
                raise RuntimeError(f'Destination directory "{args.dst}" missing file "{fname}"')
            if not os.access(f'{args.dst}/{fname}', os.W_OK):
                raise RuntimeError(f'Destination file "{args.dst}/{fname}" not writeable')
    else:  # check that single src and dst file have correct permissions
        if not os.access(args.src, os.R_OK):
            raise RuntimeError(f'Source file "{args.src}" not readable')
        if not os.access(args.dst, os.W_OK):
            raise RuntimeError(f'Destination file "{args.dst}" not writeable')
    return src_dir  # == dst_dir


def parse_args():
    """
    Set up and run argparse command-line parser
    """
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Project 1 utility for splicing student '
        'code blocks between files and directories',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        '-v',
        metavar='verbosity',
        type=int,
        default=1,
        required=False,
        help='Verbosity level (use "-v 0" for silent operation)',
    )
    parser.add_argument(
        '-s',
        metavar='suffix',
        type=str,
        default='old',
        required=False,
        help='When splicing into file dst.x that already has content in its '
        'student code blocks, a copy of the original dst.x will be saved as '
        'dst-old.x (where .x is the file extension, like .c or .h), or in '
        'dst-old-old.x if dst-old.x already exists, etc. Use this option to '
        'change the suffix to something other than "old". Turn this all off '
        '(to overwrite and lose student code blocks in dst.x) with "-s NO"',
    )
    parser.add_argument(
        '-imm',
        action='store_true',
        default=False,
        help='Use this to Ignore MisMatches between the lines beginning '
        'the student code blocks in the source vs destination files; '
        'halting at mismatches is a way to make sure blocks are not copied '
        'between the wrong files.',
    )
    parser.add_argument(
        '-b',
        metavar='block',
        type=int,
        default=0,
        required=False,
        help='By default (as with "-b 0"), all blocks in a file are spliced. '
        'Use this option to splice only a single block in a single file (not '
        'directories of files), as identified by the *one*-based numbering of '
        'the block in the source file (e.g. "-b 1" splices only first block).',
    )
    parser.add_argument('src', type=str, help='source file or directory.')
    parser.add_argument(
        'dst',
        type=str,
        help='destination file or directory.  The source '
        'and destination must either both be single files, or both be '
        'directories. If src and dst are directories, these Project 1 files '
        'within them will be spliced:\n ' + ' '.join(PFILES),
    )
    return parser.parse_args()


if __name__ == '__main__':
    ARGS = parse_args()
    VERB = ARGS.v
    doit(ARGS, check_args(ARGS))
