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
Builds the _plt CFFI (in "API out-of-line") python extension module (in file named
_plt.<platform>.so, where <platform> is something about your Python version and OS),
which links into the shared libplt library containing the project C code. With
optional "-r" command-line argument, will create _rplt extension to wrap librplt
reference code library.
"""
import os
import sys
import argparse
import pathlib
import cffi

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError means you need python3, not python2
del _x, _y


def make_shlib(shlib_ext: str) -> None:
    # (HEY: this is copy/pasted to plt.py)
    """
    Runs "make shlib" to rebuild libplt.{so,dylib} containing *student* code
    """
    # Seem to need some directory trickery: the import that got us to this point could
    # have been started from anywhere; we need to get to the parent directory of this
    # very file. 1st ".parent" below is only dir containing this file); 2nd ".parent"
    # is the parent directory with the Makefile with needed 'shlib' target
    pth = pathlib.Path(os.path.realpath(__file__)).parent.parent
    cmd = f'cd {pth.absolute()}; make shlib'
    if not VERBOSE:
        cmd += ' -s'  # make "make" run silently
    if VERBOSE:
        print(f'## build_plt.py: rebuilding shared library libplt.{shlib_ext}, via:')
        print('   ', cmd)
    if os.system(cmd):
        raise RuntimeError(f'due to trying to rebuild libplt.{shlib_ext}')


def build(use_ref: bool, risd: bool, use_int: bool) -> None:
    """
    Sets up and makes calls into cffi.FFI() to compile Python _plt extension module
    that links into lib_plt shared library (or _rplt extension module linking into
    lib_rplt, if build_ref)
    """
    rrr = 'r' if use_ref else ''
    # given reliance on files in specific places; change to dir containing this file
    os.chdir(pathlib.Path(os.path.realpath(__file__)).parent)
    # remember here = current directory, as location of librplt shared library for rpath,
    # to enable later import of _rplt extension library from somewhere besides here
    here = os.getcwd()
    # THIS is where we use our knowledge of $SCIVIS; so that we can get
    # the student-specific path to libtpz.SHEXT needed for all nrrd stuff
    shlib_path = SCIVIS + '/tpz/shlib'
    source_args = {
        'libraries': [f'{rrr}plt', 'tpz'],
        'include_dirs': ['..', SCIVIS + '/tpz/include'],
        'library_dirs': [here, shlib_path],
        # The next arg teaches the extension library about the paths that the dynamic linker
        # should look in for other libraries we depend on (the dynamic linker does not know
        # or care about $SCIVIS). We are avoiding any reliace on environment variables like
        # LD_LIBRARY_PATH on linux or DYLD_LIBRARY_PATH on Mac (on recent Macs the System
        # Integrity Protection (SIP) actually disables DYLD_LIBRARY_PATH).
        # On linux, paths listed here are passed to -Wl,--enable-new-dtags,-R<dir>
        # and "readelf -d _plt.cpython*-linux-gnu.so | grep PATH" should show these paths,
        # and "ldd _plt.cpython*-linux-gnu.so" should show where dependencies were found.
        # On Mac, paths listed should be passed to -Wl,-rpath,<dir>, and you can see those
        # with "otool -l _plt.cpython*-darwin.so", in the LC_RPATH sections. However, in
        # at least one case GLK observed, this didn't happen, so we redundantly also set
        # rpath directly for Macs, in the next statement below.
        # (Also for Mac: note that the "-int" option sets the use_int argument above, which
        # runs the "install_name_tool" utility below, but this should not be necessary if
        # the rpath has been set correctly)
        'runtime_library_dirs': [here, shlib_path],
        # https://docs.python.org/3/distutils/apiref.html#distutils.core.Extension
        'undef_macros': ['NDEBUG'],  # keep asserts() and pltVerbose as normal
    }
    if risd:
        source_args['extra_compile_args'] = ['-DSCIVIS_REAL_IS_DOUBLE']
    if sys.platform == 'darwin':  # make extra sure that rpath is set on Mac
        source_args['extra_link_args'] = [f'-Wl,-rpath,{P}' for P in [here, shlib_path]]
    ffibld = cffi.FFI()
    if VERBOSE:
        print('## build_plt.py: calling set_source with ...')
        for key, val in source_args.items():
            print(f'   {key} = {val}')
    ffibld.set_source(
        f'_{rrr}plt',
        '#include "../plt.h"',
        **source_args,
    )
    # declare this so that plt.py can call free() on biff messages
    ffibld.cdef('extern void free(void *);')
    # things common across projects
    ffibld.cdef(f'typedef {"double" if risd else "float"} real;')
    with open('cdef_common.h', 'r', encoding='utf-8') as file:
        ffibld.cdef(file.read())
    # things specific to p1plotr
    with open('cdef_plt.h', 'r', encoding='utf-8') as file:
        ffibld.cdef(file.read())
    if VERBOSE:
        print(f'## build_plt.py: compiling _{rrr}plt ...')
    out_path = ffibld.compile(verbose=(VERBOSE > 0))
    so_made = os.path.basename(out_path)
    if VERBOSE:
        print(f'## build_plt.py: ... done compiling _{rrr}plt')
        print(f'## build_plt.py: created extension library: {so_made}')
    if use_int:  # should only be true on mac
        cmd = (
            'install_name_tool -change @rpath/libtpz.dylib '
            f'{SCIVIS}/tpz/shlib/libtpz.dylib {so_made}'
        )
        if VERBOSE:
            print('## build_plt.py: setting full path to libtpz.dylib with:')
            print('   ', cmd)
        if os.system(cmd):
            raise RuntimeError(f'due to trying to set full path to libtpz.dylib in {so_made}')


def parse_args():
    """
    Set up and run argparse command-line parser
    """
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Utility for compiling CFFI-based '
        'python3 extension around libplt shared library'
    )
    parser.add_argument(
        '-v',
        metavar='verbosity',
        type=int,
        default=1,
        required=False,
        help='verbosity level (use "-v 0" for silent operation)',
    )
    parser.add_argument(
        '-r',
        action='store_true',
        default=False,
        help='use this flag to built _rplt around the libr_lib library of reference code.',
    )
    parser.add_argument(
        '-risd',
        action='store_true',
        default=False,
        help=(
            'By default the "real" type within plt is typedef\'d to float. '
            'If plt has been compiled with "make CFLAGS=-DSCIVIS_REAL_IS_DOUBLE", '
            'then this -risd option must be used.'
        ),
    )
    if sys.platform == 'darwin':  # mac
        parser.add_argument(
            '-int',
            action='store_true',
            default=False,
            help=(
                'after creating cffi extension library, store in it the explicit '
                '$SCIVIS/tpz/shlib path to libtpz.dylib, using install_name_tool.'
            ),
        )
    return parser.parse_args()


if __name__ == '__main__':
    # need SCIVIS to find "tpz" Teem/png/zlib shared library for compilation
    SCIVIS = os.getenv('SCIVIS')
    if SCIVIS is None:
        raise Exception(
            '## build_plt.py: Need environment variable "SCIVIS" set '
            'in order to build extension library'
        )
    ARGS = parse_args()
    VERBOSE = ARGS.v
    if sys.platform == 'darwin':  # mac
        SHEXT = 'dylib'
    else:
        SHEXT = 'so'
    if not ARGS.r:
        make_shlib(SHEXT)
    build(ARGS.r, ARGS.risd, ARGS.int if sys.platform == 'darwin' else False)
