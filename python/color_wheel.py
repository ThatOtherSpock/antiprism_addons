#!/usr/bin/python3

'''
Make various color wheels

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''

import os
import sys
import argparse
import subprocess
import tempfile

import anti_common
from anti_common import run_proc

def polygon(num_cols, map_name, col_sys, planar_args):
    denomenator = int(args.num_cols);
    numerator = 3 * denomenator;
    run_proc('polygon dih -s 1 %d/%d | off_color -f K -v E -e F -m "%s" | planar -d tile -M %s -E v "%s"' % (numerator, denomenator, map_name, col_sys, planar_args))

def grid(num_cols, map_name, col_sys, planar_args):
    run_proc('col_util -m %s -d grid -w b -Z "%s" | off_trans -C > color_grid1.off' % (map_name, num_cols))
    run_proc('off_trans -R 0,0,-90 color_grid1.off > color_grid2.off')
    run_proc('off_util color_grid1.off color_grid2.off | planar -d tile -M %s -E V "%s"' % (col_sys, planar_args))

def globe(num_cols, map_name, col_sys, planar_args):
    M = int(num_cols)
    n = M * 2
    run_proc('n_icons -n %d -M %d -t 0 -m "%s" -f l > color_globe1.off' % (n, M, map_name))
    run_proc('n_icons -n %d -M %d -t 0 -m "%s" -f m > color_globe2.off' % (n, M, map_name))
    run_proc('off_util color_globe1.off color_globe2.off | planar -d tile -M %s -E V "%s"'  % (col_sys, planar_args))

def build_map(off_file, off_map):
    run_proc('col_util -d 4 -U u -O "%s" > "%s"' % (off_file, off_map))

#https://stackoverflow.com/questions/32607370/python-how-to-get-the-number-of-lines-in-a-text-file
def line_count(map_name):
    with open(map_name) as f:
      count = sum(1 for _ in f)
    return count

def int_range(a):
    num = int(a)
    if num < 0:
        raise argparse.ArgumentTypeError("integer value must be positive or zero")
    return num

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument('map_name', nargs='?', default="",
                    help='map name or definition. if file type .off is seen, build map from off file')

parser.add_argument('num_cols', type=int_range,
                    help='number of map colors. non-negative integer. zero for all colors of an off file')

parser.add_argument('-m', '--model', choices=['w','g','G'], default='w',
                    help='type of model. w - wheel, g - grid, G - globe (default: %(default)s)')

parser.add_argument('-c', '--col_sys', choices=['rgb', 'hsv', 'hsl'], default="rgb",
                    help='rgb, hsv or hsl (default: %(default)s)')

parser.add_argument('rest', nargs=argparse.REMAINDER,
                    help='argument string at end are passed to planar')

args = parser.parse_args()

#https://www.geeksforgeeks.org/python-program-to-convert-a-list-to-string/
planar_args = " ".join(args.rest)

#https://stackoverflow.com/questions/7604966/maximum-and-minimum-values-for-ints
filename = args.map_name
count = sys.maxsize
# if we are dealing with a file, we need full path before changing directory tmp directory
if os.path.isfile(args.map_name):
  filename = os.path.abspath(args.map_name)
  count = line_count(filename)

# make and change to a temporary directory and work inside of it
#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
tmp_dir = tempfile.TemporaryDirectory()
os.chdir(tmp_dir.name)

if args.map_name.endswith("off"):
  off_map = "off_map.txt"
  build_map(filename, off_map)
  filename = off_map
  count = line_count(filename)
  if (args.num_cols == 0):
    args.num_cols = line_count(filename)

if (args.model == 'G' and args.num_cols < 3):
  print(f"{__file__}: error: with globe model, num_cols must be greater than 2", file=sys.stderr)
  sys.exit(1)
elif (args.num_cols == 0):
  print(f"{__file__}: error: num_cols must be greater than zero", file=sys.stderr)
  sys.exit(1)
elif (args.num_cols > count):
  print(f"{__file__}: error: num_cols is greater than map count (%d)" % count, file=sys.stderr)
  sys.exit(1)

if (args.model == 'w'):
  polygon(args.num_cols, filename, args.col_sys, planar_args)
elif (args.model == 'g'):
  grid(args.num_cols, filename, args.col_sys, planar_args)
elif (args.model == 'G'):
  globe(args.num_cols, filename, args.col_sys, planar_args)
