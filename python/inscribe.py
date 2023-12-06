#!/usr/bin/python3

'''
This program sets the radius of model2's face, edge, or vertex given by
element number, to that of model1's face, edge, or vertex given by its
element number.

Model 2 is resized and combined with model 1. The original model1 and model2
are not changed.

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

def get_radius(elem_type, elem_num, model):
    return float(split_on_comma(off_query("d", elem_type, elem_num, model)))

def int_range(a):
    num = int(a)
    if num < 0:
        raise argparse.ArgumentTypeError("integer value must be positive or zero")
    return num

#https://stackoverflow.com/questions/3853722/how-to-insert-newlines-on-argparse-help-text#comment10098369_3853776
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument('model1', help='off file')

parser.add_argument('model2', help='off file')

parser.add_argument('-e', '--elemtype1', choices=['V','E','F'], default='',
                    help='element type of model1')

parser.add_argument('-n', '--elemnum1', type=int_range, default=0,
                    help='element number of model1 (default: %(default)d)')

parser.add_argument('-E', '--elemtype2', choices=['V','E','F'], default='',
                    help='element type of model2')

parser.add_argument('-N', '--elemnum2', type=int_range, default=0,
                    help='element number of model2 (default: %(default)d)')

parser.add_argument('-v', '--verbose', action='store_true', default=False,
                    help='output calculated values')

args = parser.parse_args()

if (args.elemtype1 == ""):
  print(f"{__file__}: error: element type for model1 required (-e)", file=sys.stderr)
  sys.exit(1)
if (args.elemtype2 == ""):
  print(f"{__file__}: error: element type for model2 required (-E)", file=sys.stderr)
  sys.exit(1)

#get full path, if it is a .off file
if (args.model1.endswith(".off")):
  args.model1 = os.path.abspath(args.model1)
if (args.model2.endswith(".off")):
  args.model2 = os.path.abspath(args.model2)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

read_off_file(args.model1, tmp_dir, "inscribe1.off")
read_off_file(args.model2, tmp_dir, "inscribe2.off")

radius1 = get_radius(args.elemtype1, args.elemnum1, "inscribe1.off")
radius2 = get_radius(args.elemtype2, args.elemnum2, "inscribe2.off")

if (args.verbose):
  print("radius1 = ", str(radius1), file=sys.stderr)
  print("radius2 = ", str(radius2), file=sys.stderr)

ratio = radius1/radius2;

if (args.verbose):
  print("model 2 is scaled by ", str(ratio), file=sys.stderr)

run_proc('off_trans -S %.15lf %s | off_util - %s' % (ratio, "inscribe2.off", "inscribe1.off"))
