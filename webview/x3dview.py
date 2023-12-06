#!/usr/bin/python3

# BSD Zero Clause License
# 
# Copyright (c) 2023 Roger Kaufman <polyhedrasmith@gmail.com>
# 
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

'''
View VRML/X3D file with an online html browser VRML/X3D file viewer.
if vr1tovr2 is in the path, it will internally convert from VRML 1.0 to VRML 2.0
if tovrmlx3d (of view3dscene) exists, can convert VRML to X3D

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''
import argparse

import os
import sys
import time
import subprocess
import webbrowser
import fileinput
import tempfile
import shutil

def run_proc(proc, verbose=False, debug=False):
    # open interactive bash using -i
    #https://stackoverflow.com/questions/6856119/can-i-use-an-alias-to-execute-a-program-from-a-python-script
    # tack exit onto end of proc so it exits bash shell. silence the exit.
    proc = proc + ";exit 2>/dev/null"
    if (debug):
      print("proc =", proc, flush=True, file=sys.stderr)
    sp = subprocess.run(["/bin/bash", "-i", "-c", proc], stderr=(sys.stderr if (verbose) else subprocess.DEVNULL))
    return sp

#https://stackoverflow.com/questions/55324449/how-to-specify-a-minimum-or-maximum-float-value-with-argparse
def float_range(mini,maxi):
    """Return function handle of an argument type function for 
       ArgumentParser checking a float range: mini <= arg <= maxi
         mini - minimum acceptable argument
         maxi - maximum acceptable argument"""

    # Define the function with default arguments
    def float_range_checker(arg):
        """New Type function for argparse - a float within predefined range."""

        try:
            f = float(arg)
        except ValueError:    
            raise argparse.ArgumentTypeError("must be a floating point number")
        if f < mini or f > maxi:
            raise argparse.ArgumentTypeError("must be in range [" + str(mini) + " .. " + str(maxi)+"]")
        return f

    # Return function handle to checking function
    return float_range_checker
        
def port_checker(a):
    num = int(a)
     
    if num < 0 or num > 65535:
        raise argparse.ArgumentTypeError("port must be between 0 an 65535")
    return num
    
#https://stackoverflow.com/questions/3703276/how-to-tell-if-a-file-is-gzip-compressed
def is_gz_file(filepath):
    with open(filepath, 'rb') as test_f:
        return test_f.read(2) == b'\x1f\x8b'

#https://www.scivision.dev/python-detect-wsl/
def in_wsl() -> bool:
    return 'microsoft-standard' in os.uname().release

def command_exists(command_name):
    exists = False
    proc = 'alias | grep %s' % (command_name)
    p = subprocess.run(["/bin/bash", "-i", "-c", proc], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if (p.returncode == 0):
      exists = True
    else:
      if (shutil.which(command_name)):
        exists = True
      else:
        command_name = command_name + ".exe"
        if (shutil.which(command_name)):
          exists = True
    return (exists, command_name)

# defaults are set here
__version__ = 2.3
verbose = False
default_port = 8080
default_sleep = 4
default_url = 1
if sys.platform.startswith('linux') and not in_wsl():
  default_browser = 2
else:
  default_browser = 1

# url list. note port is as a string "PORT" to be replace by variable 'port'
url_help = [
f'0 - listing (set default_url in {__file__})',
'1 - Full screen view',
'2 - Window view',
'3 - X_ite latest version',
]

url_list = [
'not used',
'https://www.interocitors.com/polyhedra/x3dview.html?url=http://127.0.0.1:PORT/x3dview.wrl',
'https://www.interocitors.com/polyhedra/x3dwin.html?url=http://127.0.0.1:PORT/x3dview.wrl',
'https://www.interocitors.com/polyhedra/x3dlate.html?url=http://127.0.0.1:PORT/x3dview.wrl',
]

# browser list
browser_help = [
f'0 - listing (set default_browser in {__file__})',
'1 - Microsoft Edge',
'2 - Chrome',
'3 - Firefox',
]

#https://stackoverflow.com/questions/54298597/how-to-compose-a-list-with-conditional-elements
#https://stackoverflow.com/questions/1854/how-to-identify-which-os-python-is-running-on
if sys.platform.startswith('linux') and not in_wsl():
  browser_list = [
  'not used',
  '/usr/bin/msedge',
  '/usr/bin/google-chrome',
  '/snap/bin/firefox',
  ]
else:
  if sys.platform.startswith('cygwin') or sys.platform.startswith('win'):
    prefix = "C:"
  elif in_wsl():
    prefix = "/mnt/c"
  else:
    print(f"{__file__}: error: platform not found")
    exit(1)

  browser_list = [
  'not used',
  '%s/Program Files (x86)/Microsoft/Edge/Application/msedge.exe' % (prefix),
  '%s/Program Files (x86)/Google/Chrome/Application/chrome.exe' % (prefix),
  '%s/Program Files/Mozilla Firefox/firefox.exe' % (prefix),
  ]

# if outside quotes were added to a file name they must be removed
# this is in case the file name was sent from dos batch
#https://stackoverflow.com/questions/40950791/remove-quotes-from-string-in-python
for i in range(len(sys.argv)):
  sys.argv[i] = sys.argv[i].replace('"', '').replace("'", '')

#https://stackoverflow.com/questions/3853722/how-to-insert-newlines-on-argparse-help-text#comment10098369_3853776
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

#https://stackoverflow.com/questions/7576525/optional-stdin-in-python-with-argparse
#https://github.com/python/cpython/issues/58364
parser.add_argument('x3d_file', nargs='?', type=argparse.FileType('rb'), default='-',
                    help='VRML/X3D file (may be compressed). can also be from standard input')
                    
parser.add_argument('-v', '--vrml2', action='store_true', default=False,
                    help='convert vrml 1.0 to vrml 2.0 (done before -x)')
                    
parser.add_argument('-x', '--x3d', choices=['x','c'], default="",
                    help='convert vrml 2.0 to xml (x3d) or classic (x3dv) (default: none)')
                    
parser.add_argument('-g', '--gzip', action='store_true', default=False,
                    help='compress output with gzip')
                    
parser.add_argument('-w', '--writeout', action='store_true', default=False,
                    help='write to stdout instead of sending to browser')

parser.add_argument('-url', '--url', type=int, choices=range(0, len(url_list)), metavar=("{number from 0 to " + str(len(url_list)-1) + "}"), default=default_url,
                    help='url of online viewer, 0 to list (default: %(default)s)')

parser.add_argument('-browser', '--browser', type=int, choices=range(0, len(browser_list)), metavar=("{number from 0 to " + str(len(browser_list)-1) + "}"), default=default_browser,
                    help='browser, 0 to list (default: %(default)s)')

parser.add_argument('-port', '--port', type=port_checker, metavar=("{number from 0 to 65535}"), default=default_port,
                    help='port number for server (default: %(default)s)')

parser.add_argument('-sleep', '--sleep', type=float_range(1.0, 3600.0), metavar=("from 1 to 3600 seconds"), default=default_sleep,
                    help='time in seconds before server shutdown (default: %(default)s)')

parser.add_argument('--version', action='version', version='%(prog)s {version}'.format(version=__version__))

args = parser.parse_args()

#https://stackoverflow.com/questions/18569045/how-do-i-get-a-list-to-print-one-word-per-line
if (args.url == 0 ):
  print('\n'.join(url_help))
  sys.exit()
  
if (args.browser == 0 ):
  print('\n'.join(browser_help))
  sys.exit()
elif not os.path.isfile(browser_list[args.browser]):
  print(f"{__file__}: error: browser %s not found" % browser_list[args.browser])
  sys.exit()

url = url_list[args.url]
url = url.replace("PORT", str(args.port))

browser_path = browser_list[args.browser]

#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
# create temporary directory for server
tmp_dir = tempfile.TemporaryDirectory()

# read from file argument or stdin. write a copy of off file in temporary directory
full_name = tmp_dir.name + "/x3dview.wrl"
fout = open(full_name, "wb")
fout.writelines(args.x3d_file.readlines())
fout.close()

#https://stackoverflow.com/questions/2507808/how-to-check-whether-a-file-is-empty-or-not
if os.stat(full_name).st_size == 0:
  print(f"{__file__}: error: empty input")
  sys.exit(1)

# change to temporary directory and start server
os.chdir(tmp_dir.name)

# process temp file, it is a copy so ok to change it

# form command string
if is_gz_file("x3dview.wrl"):
  proc = 'gzip -dc'
else:
  proc = 'cat'

#https://unix.stackexchange.com/questions/6516/filtering-invalid-utf8
proc += ' x3dview.wrl | iconv -c -t UTF-8 > x3dtemp.wrl; mv x3dtemp.wrl x3dview.wrl'
run_proc(proc)

if (args.vrml2):
  # if vr1tovr2.exe (to check and convert vrml 1.0  to vrml 2.0) is found...
  # online at: https://www.interocitors.com/polyhedra/vr1tovr2/index.html
  # note: windows only until a linux solution is found
  #https://docs.python.org/3/library/shutil.html#shutil.which

  command_name = "vr1tovr2"
  result = command_exists(command_name)
  command_found = result[0]
  command_name = result[1]

  if (command_found):
   run_proc('%s -p x3dview.wrl > x3dtemp.wrl; mv x3dtemp.wrl x3dview.wrl' % (command_name))
  else:
    print(f"{__file__}: warning: %s not found" % (command_name))

if (args.x3d):
  # view3dscene is found at https://castle-engine.io/view3dscene.php
  # command must exist in path or alias
  # will also attempt to convert VRML 1.0 to X3D
  #proc = 'view3dscene --write --write-force-x3d --write-encoding'

  command_name = "tovrmlx3d"
  result = command_exists(command_name)
  command_found = result[0]
  command_name = result[1]
  
  if (command_found):
    proc = '%s --force-x3d --encoding' % command_name

    if "x" in args.x3d:
      proc += ' xml'
    else:
      proc += ' classic'

    # send warning text to /dev/null
    proc += ' "x3dview.wrl" 2>/dev/null > x3dtemp.wrl; mv x3dtemp.wrl x3dview.wrl'
    run_proc(proc)
    
    if os.stat("x3dview.wrl").st_size == 0:
      print(f"{__file__}: error: %s conversion did not produce x3d" % (command_name))
      sys.exit(1)
  else:
    print(f"{__file__}: warning: %s not found" % (command_name))

if (args.gzip):
  # output can be compressed
  command_name = "gzip"
  result = command_exists(command_name)
  command_found = result[0]
  command_name = result[1]
 
  if (command_found):
    run_proc('cat x3dview.wrl | %s -f > x3dtemp.wrl; mv x3dtemp.wrl x3dview.wrl' % (command_name))
  else:
    print(f"{__file__}: warning: %s not found") % (command_name)
  
if (args.writeout):
  # write to stdout and exit
  run_proc("cat x3dview.wrl")
  exit(0)

# launch server with subprocess in background
command_name = "webview_server.py"
result = command_exists(command_name)
command_found = result[0]
command_name = result[1]
if (command_found):
  proc = 'webview_server.py -port %s -sleep %s 2>/dev/null &' % (str(args.port), str(args.sleep))
  sp = subprocess.Popen(["/bin/bash", "-i", "-c", proc])
  sp.communicate()
else:
  print(f"{__file__}: error: %s not found" % (command_name))
  sys.exit(1)

# webbrower.get(...) returns instantly
# subprocess.call(webbrower.get(...)) does not return
webbrowser.register('browser', None, webbrowser.BackgroundBrowser(browser_path))
webbrowser.get('browser').open(url, new=2)

# delay for webbrowser.get
time.sleep(args.sleep)

# remove temporary file so it won't still be there on a fail
tmp_dir.cleanup()
