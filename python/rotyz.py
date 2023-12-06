#!/usr/bin/python3

'''
Rotate an element of an OFF file to Y (0,1,0) and a second element to Z (0,0,1)

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''
import os
import sys
import subprocess
import argparse
import tempfile

import anti_common
from anti_common import run_proc
from anti_common import read_off_file
from anti_common import split_on_comma
from anti_common import off_query

def get_coord(elem_type, elem_num, model):
    if (elem_type == 'V'):
      operation = 'c'
    else:
      operation = 'C'
    return split_on_comma(off_query(operation, elem_type, elem_num, model)).replace(" ",",")

def int_range(a):
    num = int(a)
    if num < 0:
        raise argparse.ArgumentTypeError("integer value must be positive or zero")
    return num

#https://stackoverflow.com/questions/3853722/how-to-insert-newlines-on-argparse-help-text#comment10098369_3853776
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument('off_file', help='off file')

parser.add_argument('-e', '--elemtype1', choices=['V','E','F'], default='',
                    help='element type to rotate to Y')

parser.add_argument('-n', '--elemnum1', type=int_range, default=0,
                    help='element number to rotate to Y (default: %(default)d)')

parser.add_argument('-E', '--elemtype2', choices=['V','E','F'], default='',
                    help='element type to rotate to Z')

parser.add_argument('-N', '--elemnum2', type=int_range, default=0,
                    help='element number to rotate to Z (default: %(default)d)')

parser.add_argument('-v', '--verbose', action='store_true', default=False,
                    help='output calculated values')

args = parser.parse_args()

if (args.elemtype1 == ""):
  print(f"{__file__}: error: element to rotate to Y required (-e)", file=sys.stderr)
  sys.exit(1)
if (args.elemtype2 == ""):
  print(f"{__file__}: error: element to rotate to Z required (-E)", file=sys.stderr)
  sys.exit(1)

#get full path, if it is a .off file
if (args.off_file.endswith(".off")):
  args.off_file = os.path.abspath(args.off_file)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

tmpfile = "rotyz.off"
read_off_file(args.off_file, tmp_dir, tmpfile)

coord1 = get_coord(args.elemtype1, args.elemnum1, tmpfile)
coord2 = get_coord(args.elemtype2, args.elemnum2, tmpfile)

if (args.verbose):
  print("coord1 = ", str(coord1), file=sys.stderr)
  print("coord2 = ", str(coord2), file=sys.stderr)

y = "0,1,0"
z = "0,0,1"

run_proc('off_trans -R %s,%s,%s,%s %s' % (coord1, coord2, y, z, tmpfile))
