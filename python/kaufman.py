#!/usr/bin/python3

'''
Make Kaufman Polyhedra. A Kaufman Polyhedra is the convex hull of a base model
merged with it's canonical reciprocal. It is the same as a conway join:
conway j^n input_file | canonical | off_color -f S -m spread

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''
import os
import sys
import argparse
import subprocess
import tempfile

import anti_common
from anti_common import run_proc
from anti_common import read_off_file

def canonical(output):
    run_proc('canonical tmp_base.off -d 0 -l 16 -i 100000 -n 100000 -z 100000 %s' % (output), args.verbose)
    if (args.verbose):
      print(" ", file=sys.stderr)

def kaufman():
    for i in range(1, int(args.n)+1):
      canonical("-O bd -o tmp_canonical_bd.off")
      run_proc('conv_hull tmp_canonical_bd.off -o tmp_base.off')

def conway():
    # if n = 0 then just do seed
    if (args.n > 0):
      oper = 'j^%d' % (args.n)
    else:
      oper = 'S'
    v_arg = "-v" if (args.verbose) else ""
    run_proc('conway %s %s tmp_base.off > tmp_conway.off' % (v_arg, oper), args.verbose, True)
    run_proc('mv tmp_conway.off tmp_base.off')

def int_range(a):
    num = int(a)
    if num < 0:
        raise argparse.ArgumentTypeError("integer value must be positive or zero")
    return num

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument('off_file', nargs='?', default="",
                    help='OFF file or Antiprism built-in model. can also be from standard input')

parser.add_argument('n', type=int_range,
                    help='number of interations. non-negative integer')

parser.add_argument('-c', '--conway', action='store_true', default=False,
                    help='use conway notation join')

parser.add_argument('-E', '--edgeColor', default="lightgray",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-V', '--vertexColor', default="gold",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-v', '--verbose', action='store_true', default=False,
                    help='allow reporting output from canonical')

args = parser.parse_args()

#get full path, if it is a .off file
if ('.' in args.off_file):
  args.off_file = os.path.abspath(args.off_file)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

read_off_file(args.off_file, tmp_dir, "tmp_base.off")

if (args.conway):
  conway()
else:
  kaufman()

run_proc('off_color tmp_base.off -f S -e %s -v %s > tmp_color.off' % ( args.edgeColor, args.vertexColor))
run_proc('mv tmp_color.off tmp_base.off')
canonical("")
