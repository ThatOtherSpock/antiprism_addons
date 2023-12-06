#!/usr/bin/python3

'''
Embed a dipyramid within a cube in various positions

Written by Roger Kaufman <polyhedrasmith@gmail.com>
Math by Adrian Rossiter (www.antiprism.com)
'''
import os
import sys
import subprocess
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

parser.add_argument('-m', '--model', choices=['d','e','D','E'], default="d",
                    help='embed dipyramid in cube faces(d), edges(e), alternative view: faces(D), edges(E) (default: %(default)s)')

parser.add_argument('-D', '--dipyramidColor', default="red",
                    help='an X11 color name (default: %(default)s)')

parser.add_argument('-C', '--cubeColor', default="yellow",
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

# calculation supplied by Adrian Rossiter
angle = math.pi*d/n
if "d" in args.model or "D" in args.model:
  A = 1/(2*math.tan(angle))
elif "e" in args.model or "E" in args.model:
  A = 1/(2*math.sin(angle))

# must go to standard error
if (args.verbose):
  print("angle = ", str(angle), file=sys.stderr)
  print("A     = ", str(A), file=sys.stderr)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

if ("d" in args.model):
  #alternate way to set cube upright. off_trans cube -R 45,0,0 -R 0,0,35.264389682754654315377
  run_proc('polygon dip %d/%d -e %.17f -l %.17f | off_trans -R 90,%.17f,0 | off_color -f %s -o tmp1.off' % (n, d, math.sqrt(2)/2, A, angle*180/math.pi, args.dipyramidColor))
elif ("e" in args.model):
  run_proc('polygon dip %d/%d -e %.17f -l %.17f | off_trans -R 90,0,0 | off_color -f %s -o tmp1.off' % (n, d, math.sqrt(2)/2, A, args.dipyramidColor))
elif ("D" in args.model):
  run_proc('polygon dip %d/%d -e 1 -l %.17f | off_trans -R 90,%.17f,0 | off_color -f %s -o tmp1.off'  % (n, d, A, angle*180/math.pi, args.dipyramidColor))
elif ("E" in args.model):
  run_proc('polygon dip %d/%d -e 1 -l %.17f | off_trans -R 90,0,0 | off_color -f %s -o tmp1.off' % (n, d, A, args.dipyramidColor))

if ("d" in args.model) or ("e" in args.model):
  run_proc('off_trans cube -R 1,1,1,0,1,0 -R 0,-15,0 -S %0.17f | off_color -f %s -r A:0.5 -o tmp2.off' % (A*math.sqrt(1+1/3.0), args.cubeColor))
elif ("D" in args.model) or ("E" in args.model):
  run_proc('off_trans cube -R 0,45,0 -R 90,0,0 -S %0.17f | off_color -f %s -r A:0.5 -o tmp2.off' % (A*math.sqrt(2), args.cubeColor))

run_proc('off_util tmp1.off tmp2.off | off_color -e %s -v %s' % (args.edgeColor, args.vertexColor))
