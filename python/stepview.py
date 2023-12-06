#!/usr/bin/python3

'''
Step through and view off or obj files in a directory. Press ESC on the
antiview window to advance to the next model. Press CTRL-C on command shell
a few times to exit

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''
import os
import sys
import argparse
import time
import subprocess
import ast

import anti_common
from anti_common import run_proc

#https://code.activestate.com/recipes/134892/
class _Getch:
    """Gets a single character from standard input.  Does not echo to the screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): return self.impl()

class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()

getch = _Getch()

path = "./"

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument('-t', '--filetype', choices=['off', 'obj'], default="off",
                    help='off or obj files (default: %(default)s)')

parser.add_argument('-V', '--viewer', choices=['antiview', 'offview'], default="antiview",
                    help='antiview or offview OFF viewer (default: %(default)s)')

parser.add_argument('-s', '--subcommand', type=str, default="",
                    help='optional quoted string is an antiprism command that can be inserted before sending to the viewer. ex. "off_color -f L"')

#https://stackoverflow.com/questions/16174992/cant-get-argparse-to-read-quoted-string-with-dashes-in-it
parser.add_argument('-a', '--antiargs', type=str, default="",
                    help='optional quoted string are arguments to be appended to the viewer. hint: because of python hyphen bug, use an equal sign, ex. -a="-v 0.02 -s a"')

parser.add_argument('-v', '--verbose', action='store_true', default=False,
                    help='view procedure calls')

args = parser.parse_args()

### not needed when using subprocess with bash -i (see below)
#https://stackoverflow.com/questions/19511440/add-b-prefix-to-python-variable
#alias = (bytes.decode(subprocess.check_output('get_alias.py ' + args.viewer, shell=True))).strip()
#if alias != "":
#  args.viewer = alias

#https://stackoverflow.com/questions/24426451/how-to-terminate-loop-gracefully-when-ctrlc-was-pressed-in-python
stored_exception = None

# sorted directory list of only files
#https://stackoverflow.com/questions/11968976/list-files-only-in-the-current-directory
filelist = [f for f in sorted(os.listdir(path)) if os.path.isfile(f)]

#add dot to file type
args.filetype = '.' + args.filetype

#https://stackoverflow.com/questions/3416401/removing-elements-from-a-list-containing-specific-characters
#https://stackoverflow.com/questions/1894269/how-to-convert-string-representation-of-list-to-a-list
filelist = ast.literal_eval(f"{[x for x in filelist if args.filetype in x]}")

if (len(filelist) == 0):
  print("stepview: error: no file found of file type " + args.filetype, file=sys.stderr)

if (len(args.subcommand) != 0):
  args.subcommand += " |"

for file in filelist:
  print("viewing: '" + file + "'", file=sys.stderr)

  try:
    if stored_exception:
      break

    filename = '"' + os.path.join(path, file) + '"'

    if (args.filetype == '.off'):
      cmd = 'off_util'
    elif (args.filetype == '.obj'):
      cmd = 'obj2off'

    run_proc('%s %s | %s %s %s' % (cmd, filename, args.subcommand, args.viewer, args.antiargs), False, args.verbose)

    # for offview, keyboard input to continue
    if ("offview" in args.viewer):
      if file == filelist[-1]:
        sys.exit(0)

      print("press space bar or enter. esc to exit.", file=sys.stderr)
      while True:
        key = ord(getch())
        # if the key is enter or space
        if key == 32 or key == 13:
          break
        # if key is escape
        elif key == 27:
          sys.exit(0)
    elif ("antiview" in args.viewer):
      # give enough time to see ctrl+c
      time.sleep(0.25)

# type to break out with ctrl+c
  except KeyboardInterrupt:
    print("[CTRL+C detected]")
    stored_exception = sys.exc_info()
