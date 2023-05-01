# written by GLK Mon Apr 24 2023 sitting with Kseniia
# to be run in something like scivis-2023/work/p1plotr/py
# after running:
#   python3 build_plt.py -r
# in order to build the "_rplt" extension module (e.g. _rplt.cpython-311-darwin.so)
# around librplt.so

import rplt

data = rplt.pltDataNew()

# assumes you've run:
#    ../rplotr psamp -p 0 0.5 2 -mm -3 3 -n 10 -o parab.txt
rplt.pltDataLoad(data, b'parab.txt')

print("some things about the data:")
print(f'data.len = {data.len}')
print(f'data.min = {data.min}')
print(f'data.max = {data.max}')
print(f'data.vvp[0] = {data.vv[0]}')

# For part below, you do need to use rplt.ffi, because it knows what "real" is:
print(f"sizeof(real) = {rplt.ffi.sizeof('real')}")
# whereas if you did:
# >>>import cffi
# >>> ffi.sizeof('int')
# >>> ffi.sizeof('real')
# you'd get an error:
#            Traceback (most recent call last):
#               File "/usr/local/lib/python3.11/site-packages/cffi/api.py", line 183, in _typeof
#                result = self._parsed_types[cdecl]
#                         ~~~~~~~~~~~~~~~~~~^^^^^^^
#            KeyError: 'real'

# now, TODO: emulate:
#    ../rplotr ceval -i parab.txt -k ctmr -x 0.3 -d 0
# which returns:
# 0.330000311 = pltConvoEval(parab.txt, Ctmr, 0.3); outside=0

# these first two function args are wrong.
# use rplt.ffi.new, twice, to make places for pltConvoEval to write to
result = rplt.ffi.new('real *')
outside = rplt.ffi.new('unsigned short *')
rplt.pltConvoEval(result, outside, 0.3, 0, data, rplt.pltKernelCtmr)
print(result[0])
print(outside[0])