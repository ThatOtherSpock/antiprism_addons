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
View an OFF file with an online html browser OFF file viewer.

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

#https://stackoverflow.com/questions/9978880/python-argument-parser-list-of-list-or-tuple-of-tuples
def coords(s):
    try:
        x, y, z = map(float, s.split(','))
        return '{:f},{:f},{:f}'.format(x, y, z)
    except:
        raise argparse.ArgumentTypeError("Coordinates must be x,y,z")

def port_checker(a):
    num = int(a)

    if num < 0 or num > 65535:
        raise argparse.ArgumentTypeError("port must be between 0 an 65535")
    return num

#https://www.scivision.dev/python-detect-wsl/
def in_wsl() -> bool:
    return 'microsoft-standard' in os.uname().release

# defaults are set here
__version__ = 2.3
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
'1 - https://www.interocitors.com (python author\'s site)',
'2 - https://asliceofcuriosity.fr (threejs author\'s site)',
'3 - Simple-Off-Viewer Live Window at www.interocitors.com',
]

url_list = [
'not used',
'https://www.interocitors.com/polyhedra/offview.html?url=http://127.0.0.1:PORT/offview.off',
'https://asliceofcuriosity.fr/blog/extra/polyhedra-viewer-antiprism.html?url=http://127.0.0.1:PORT/offview.off',
'https://www.interocitors.com/polyhedra/offwin.html?url=http://127.0.0.1:PORT/offview.off',
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
parser.add_argument('off_file', nargs='?', type=argparse.FileType('r'), default=sys.stdin,
                    help='OFF file. can also be from standard input')

parser.add_argument('-v', '--vertexRadius', type=float_range(0.0, 1.0), default=0.03,
                    help='vertex sphere radius (range 0.0-1.0) (default: %(default)s)')

parser.add_argument('-e', '--edgeRadius', type=float_range(0.0, 1.0), default=0.02,
                    help='edge cylinder radius (range 0.0-1.0) (default: %(default)s)')

parser.add_argument('-x', '--hideElements', nargs='+', choices=['v','e','f'], default="",
                    help='hide elements. to hide vertices, edges and faces')

parser.add_argument('-l', '--blackEdges', dest='useBaseColor', action='store_false', default=True,
                    help='paint edges black (default: use defined colors)')

parser.add_argument('-t', '--transparency', type=float_range(0.0, 1.0), default=1.0,
                    help='face transparency. from 0 (invisible) to 1.0 (opaque) (default: %(default)s)')

parser.add_argument('-B', '--backgroundColor', default="cccccc",
                    help='background color in hexadecimal (default: %(default)s)')

parser.add_argument('-rot', '--rotationSpeed', type=float, default=0,
                    help='rotational speed (default: %(default)s)')

# use -rotax=-x,y,z when x is negative
parser.add_argument('-rotax', '--rotationAxis', type=coords, default='0,1,0',
                    help='rotational axis as x,y,z (default: %(default)s)')

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

verticesActive=True
if "v" in args.hideElements:
  verticesActive=False
edgesActive=True
if "e" in args.hideElements:
  edgesActive=False
facesActive=True
if "f" in args.hideElements:
  facesActive=False

arg_str = "&vertexRadius=" + str(args.vertexRadius)
arg_str += "&edgeRadius=" + str(args.edgeRadius)
arg_str += "&verticesActive=" + str(verticesActive).lower()
arg_str += "&edgesActive=" + str(edgesActive).lower()
arg_str += "&facesActive=" + str(facesActive).lower()
arg_str += "&useBaseColor=" + str(args.useBaseColor).lower()
arg_str += "&transparency=" + str(1.0 - args.transparency)
arg_str += "&backgroundColor=" + str(args.backgroundColor)
arg_str += "&rotationSpeed=" + str(args.rotationSpeed)
arg_str += "&rotationDirection=" + str(args.rotationAxis)

url = url_list[args.url]
url = url.replace("PORT", str(args.port))
url += arg_str

browser_path = browser_list[args.browser]

#https://stackoverflow.com/questions/3223604/how-do-i-create-a-temporary-directory-in-python
# create temporary directory for server
tmp_dir = tempfile.TemporaryDirectory()

# read from file argument or stdin. write a copy of off file in temporary directory
full_name = tmp_dir.name + "/offview.off"
fout = open(full_name, "w")
fout.writelines(args.off_file.readlines())
fout.close()

#https://stackoverflow.com/questions/2507808/how-to-check-whether-a-file-is-empty-or-not
if os.stat(full_name).st_size == 0:
  print(f"{__file__}: error: empty input")
  sys.exit()

# change to temporary directory and start server
os.chdir(tmp_dir.name)

# launch server with subprocess in background
proc = 'webview_server.py -port %s -sleep %s 2>/dev/null &' % (str(args.port), str(args.sleep))
sp = subprocess.Popen(["/bin/bash", "-i", "-c", proc])
sp.communicate()

# webbrower.get(...) returns instantly
# subprocess.call(webbrower.get(...)) does not return
webbrowser.register('browser', None, webbrowser.BackgroundBrowser(browser_path))
webbrowser.get('browser').open(url, new=2)

# delay for webbrowser.get
time.sleep(args.sleep)

# remove temporary file so it won't still be there on a fail
tmp_dir.cleanup()
