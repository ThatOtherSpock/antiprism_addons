#!/usr/bin/python3

'''
Make a trapezohedron of a height that is both dual to an antiprism and is also
its stellation. When n/d = 2, make a compound of tetrahedra.

Written by Roger Kaufman <polyhedrasmith@gmail.com>
Math by Adrian Rossiter (www.antiprism.com)
'''
import os
import sys
import argparse
import subprocess
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

parser.add_argument('-m', '--model', nargs='+', choices=['t','a'], default="t",
                    help='output trapezohedron(t), reciprocal antiprism(a), or both (default: %(default)s)')

parser.add_argument('-r', '--rotateAntiprism', action='store_true', default=False,
                    help='rotate antiprism into trapezohedron (default: %(default)s)')

parser.add_argument('-T', '--trapezohedronColor', default="red",
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
if "t" in args.model:
  files += "tmp1.off "
if "a" in args.model:
  files += "tmp2.off "

rotation = 0
if (args.rotateAntiprism):
  rotation = 180.0*d/n;

# calculation supplied by Adrian Rossiter
#antiprism_height = 2*cos(pi*d/(2*n))*sqrt((1-cos(pi*d/n))/(1+cos(pi*d/n)))
antiprism_height = 2*math.cos(math.pi*d/(2*n))*math.sqrt((1-math.cos(math.pi*d/n))/(1+math.cos(math.pi*d/n)))

# must go to standard error
if (args.verbose):
  print("antiprism height = ", str(antiprism_height), file=sys.stderr)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

# make models
run_proc('polygon ant %d/%d -s 1 -r 1 -l %.17lf | off_color -f %s -o tmp1.off' % (n, d, antiprism_height, args.trapezohedronColor))
run_proc('pol_recip -r %.17f tmp1.off | off_trans -R 0,0,%.17lf | off_color -f %s -o tmp2.off' % (math.cos(math.pi*d/(2*n)), rotation, args.antiprismColor))

# combine models rotate and color
run_proc('off_util %s | off_trans -R 90,0,0 | off_color -e %s -v %s' % (files, args.edgeColor, args.vertexColor))
