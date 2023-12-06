#!/usr/bin/python3

'''
Make a trapezohedron from a pyramid of sides N and height p

Written by Roger Kaufman <polyhedrasmith@gmail.com>
Math by Adrian Rossiter (www.antiprism.com)
'''
import os
import sys
import argparse
import math
import tempfile

import anti_common
from anti_common import run_proc
from anti_common import read_polygon

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument(
    'polygon_fraction',
    help='number of sides of the base polygon (N), or a fraction for star polygons (N/D) (default: %(default)s)',
    default='6', nargs='?', type=read_polygon)

parser.add_argument('-p', '--pyramidHeight', type=float, default=1,
                    help='height of pyramid (default: %(default)s)')

parser.add_argument('-m', '--model', nargs='+', choices=['p','m','a'], default="p m a",
                    help='input pyramid(p), mirror pyramid(m), and/or antiprism(a) (default: %(default)s)')

parser.add_argument('-P', '--pyramidColor', default="red",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-A', '--antiprismColor', default="blue",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-E', '--edgeColor', default="lightgray",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-V', '--vertexColor', default="gold",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-v', '--verbose', action='store_true', default=False,
                    help='output calculated values')

args = parser.parse_args()

# n and d are from anti_common globals
n = anti_common.n
d = anti_common.d

files = ""
if "p" in args.model:
  files += "tmp1.off "
if "m" in args.model:
  files += "tmp2.off "
if "a" in args.model:
  files += "tmp3.off "

# calculation supplied by Adrian Rossiter
angle = 180.0*d/n;
antiprism_height = args.pyramidHeight*(1/math.cos(math.pi*angle/180)-1)

# must go to standard error
if (args.verbose):
  print("pyramid height   = ", str(args.pyramidHeight), file=sys.stderr)
  print("antiprism height = ", str(antiprism_height), file=sys.stderr)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

run_proc('polygon pyr %d/%d -l %.17f | off_trans -R 0,0,%.17f -T 0,0,%.17f | off_color -f %s -o tmp1.off' % (n, d, args.pyramidHeight, angle/2, antiprism_height/2, args.pyramidColor))
run_proc('off_trans -R 0,0,%.17f -M 0,0,1 tmp1.off -o tmp2.off' % (angle))
run_proc('polygon ant %d/%d -l %.17f | off_color -f %s -o tmp3.off' % (n, d, antiprism_height, args.antiprismColor))

run_proc('off_util %s | off_trans -R 90,0,0 | off_color -e %s -v %s' % (files, args.edgeColor, args.vertexColor))
