Webview is a system to display OFF and VRML/X3D files via a web based viewer.

Written by Roger Kaufman <polyhedrasmith@gmail.com>

offview.py is a python program to view OFF files with an online viewer
written in Threejs. code at https://github.com/Wagyx/simple-off-viewer

x3dview.py is a python program to view VRML/X3D files with an online viewer
written in Javascript and WebGL found at https://create3000.github.io/x_ite/

Installation

Place python files (ending in .py) in a directory in your path.

These scripts have been tested under Cygwin64 bash and Windows WSL bash. These
environments require Python3.

===============================================================================
webview_server.py

  CORS server. This program is called from the others and runs for -sleep x
  seconds while a web page is loaded.

-------------------------------------------------------------------------------
offview.py - url can be changed to your own installation of multi-off-viewer.
You are welcome to use the url's provided. browser_path has been provided for
some browsers, or add a path to a browser if necessary. The server process
is killed after a default of 3 seconds. If this is not long enough the default
can be increased in the code, but offview.py will not return to a prompt until
it is finished. The -sleep parameter is provided to adjust it on a per run
basis.

Some default parameters in offview.py can be changed as desired. The default
browser is set for linux as Chrome, otherwise it is Microsoft Edge. WSL has
access to Windows features so it defaults to Microsoft Edge.

# defaults are set here
default_port = 8080
default_sleep = 3
default_url = 1
if sys.platform.startswith('linux') and not in_wsl():
  default_browser = 2
else:
  default_browser = 1

Current url list

$ offview.py -url 0
0 - listing (set default_url in /mnt/c/lhome/roger/bin/offview.py)
1 - https://www.interocitors.com full screen view (python author's site)
2 - https://www.interocitors.com window view (python author's site)
3 - https://asliceofcuriosity.fr (threejs author's site)

Current browser list

$ offview.py -browser 0
0 - listing (set default_browser in offview.py)
1 - Microsoft Edge
2 - Chrome
3 - Firefox

  examples of types of usage:
  
  offview.py file.off
  cat file.off | offview.py
  
-------------------------------------------------------------------------------
x3dview.py - url can be changed to your own installation of X_ite.
You are welcome to use the url's provided. browser_path has been provided for
some browsers, or add a path to a browser if necessary. The server process
is killed after a default of 3 seconds. If this is not long enough the default
can be increased in the code, but x3dview.py will not return to a prompt until
it is finished. The -sleep parameter is provided to adjust it on a per run
basis.

if vr1tovr2 is in the path, it will internally convert VRML 1.0 to VRML 2.0
found online at: https://www.interocitors.com/polyhedra/vr1tovr2/index.html

if tovrmlx3d (of view3dscene) exists, can convert VRML to X3D
view3dscene is found at https://castle-engine.io/view3dscene.php
command must exist in path or alias

Some default parameters in x3dview.py can be changed as desired. The default
browser is set for linux as Chrome, otherwise it is Microsoft Edge. WSL has
access to Windows features so it defaults to Microsoft Edge.

# defaults are set here
default_port = 8080
default_sleep = 3
default_url = 1
if sys.platform.startswith('linux') and not in_wsl():
  default_browser = 2
else:
  default_browser = 1

Current url list

$ x3dview.py -url 0
0 - listing (set default_url in x3dview.py)
1 - Full screen view
2 - Window view
3 - X_ite latest version

Current browser list

$ x3dview.py -browser 0
0 - listing (set default_browser in x3dview.py)
1 - Microsoft Edge
2 - Chrome
3 - Firefox

  examples of types of usage:
  
  x3dview.py file.wrl
  cat file.wrl | x3dview.py
 
-----
Note:
If using Anaconda the following occurrences may need to be edited to
proc = subprocess.Popen([r"python.exe", ...rest of the stuff


===============================================================================
Version 3.0 helps (in Windows)

---------------------
use webview_server.py -h for avaiable parameters.

usage: webview_server.py [-h] [-port {number from 0 to 65535}] [-sleep SLEEP]
                         [--version]

CORS Web Server provided for use with webview.py

optional arguments:
  -h, --help            show this help message and exit
  -port {number from 0 to 65535}, --port {number from 0 to 65535}
                        port number for server (default: 8080)
  -sleep SLEEP, --sleep SLEEP
                        time in seconds before server shutdown (0 to run
                        forever) (default: 0)
  --version             show program's version number and exit

--------------
use offview.py -h for avaiable parameters.
  
usage: offview.py [-h] [-v VERTEX_RADIUS] [-e EDGE_RADIUS] [-x {v,e,f} [{v,e,f} ...]] [-l] [-V VERTEX_COLOR] [-E EDGE_COLOR] [-F FACE_COLOR] [-B BACKGROUND_COLOR] [-rot ROTATION_SPEED]
                  [-rotax ROTATION_AXIS] [-t TRANSPARENCY] [-url {number from 0 to 3}] [-browser {number from 0 to 3}] [-port {number from 0 to 65535}] [-sleep from 1 to 3600 seconds] [--version]
                  [off_file]

View an OFF file with an online html browser OFF file viewer.

Written by Roger Kaufman <polyhedrasmith@gmail.com>

positional arguments:
  off_file              OFF file. can also be from standard input

options:
  -h, --help            show this help message and exit
  -v VERTEX_RADIUS, --vertex_radius VERTEX_RADIUS
                        vertex sphere radius (range 0.0-1.0) (default: 0.03)
  -e EDGE_RADIUS, --edge_radius EDGE_RADIUS
                        edge cylinder radius (range 0.0-1.0) (default: 0.02)
  -x {v,e,f} [{v,e,f} ...], --hide_elements {v,e,f} [{v,e,f} ...]
                        hide elements. to hide vertices, edges and faces
  -l, --black_edges     paint edges black (overrides -V,-E) (default: use defined colors)
  -V VERTEX_COLOR, --vertex_color VERTEX_COLOR
                        vertex color override in hexadecimal (url 1,2) (default: none)
  -E EDGE_COLOR, --edge_color EDGE_COLOR
                        edge color override in hexadecimal (url 1,2) (default: none)
  -F FACE_COLOR, --face_color FACE_COLOR
                        face color override in hexadecimal (url 1,2) (default: none)
  -B BACKGROUND_COLOR, --background_color BACKGROUND_COLOR
                        background color in hexadecimal (default: cccccc)
  -rot ROTATION_SPEED, --rotation_speed ROTATION_SPEED
                        rotational speed (default: 0)
  -rotax ROTATION_AXIS, --rotation_axis ROTATION_AXIS
                        rotational axis as x,y,z (default: 0,1,0)
  -t TRANSPARENCY, --transparency TRANSPARENCY
                        face transparency. from 0 (invisible) to 1.0 (opaque) (url 3 only) (default: 1.0)
  -url {number from 0 to 3}, --url {number from 0 to 3}
                        url of online viewer, 0 to list (default: 1)
  -browser {number from 0 to 3}, --browser {number from 0 to 3}
                        browser, 0 to list (default: 2)
  -port {number from 0 to 65535}, --port {number from 0 to 65535}
                        port number for server (default: 8080)
  -sleep from 1 to 3600 seconds, --sleep from 1 to 3600 seconds
                        time in seconds before server shutdown (default: 4)
  --version             show program's version number and exit

--------------
use x3dview.py -h for avaiable parameters.

usage: x3dview.py [-h] [-v] [-x {x,c}] [-g] [-w] [-url {number from 0 to 3}]
                  [-browser {number from 0 to 3}]
                  [-port {number from 0 to 65535}]
                  [-sleep from 1 to 3600 seconds] [--version]
                  [x3d_file]

View VRML/X3D file with an online html browser VRML/X3D file viewer.
if vr1tovr2 is in the path, it will internally convert from VRML 1.0 to VRML 2.0
if tovrmlx3d (of view3dscene) exists, can convert VRML to X3D

positional arguments:
  x3d_file              VRML/X3D file (may be compressed). can also be from
                        standard input

optional arguments:
  -h, --help            show this help message and exit
  -v, --vrml2           convert vrml 1.0 to vrml 2.0 (done before -x)
  -x {x,c}, --x3d {x,c}
                        convert vrml 2.0 to xml (x3d) or classic (x3dv)
                        (default: none)
  -g, --gzip            compress output with gzip
  -w, --writeout        write to stdout instead of sending to browser
  -url {number from 0 to 3}, --url {number from 0 to 3}
                        url of online viewer, 0 to list (default: 1)
  -browser {number from 0 to 3}, --browser {number from 0 to 3}
                        browser, 0 to list (default: 1)
  -port {number from 0 to 65535}, --port {number from 0 to 65535}
                        port number for server (default: 8080)
  -sleep from 1 to 3600 seconds, --sleep from 1 to 3600 seconds
                        time in seconds before server shutdown (default: 3)
  --version             show program's version number and exit


===============================================================================
The following helper files are provided for file manager programs

Windows batch files are currently setup for Cygwin64 or WSL that has python3
installed.

  offview.bat is a convenience Windows batch file to be placed in the directory
  in the windows path. This will allow Windows Explorer to use 'open with' for
  an OFF file with offview.bat calling offview.py with no parameters. Custom
  parameters may be added as desired.

  x3dview.bat is a convenience Windows batch file to be placed in the directory
  in the windows path. This will allow Windows Explorer to use 'open with' for
  a VRML/X3D file with x3dview.bat calling x3dview.py with no parameters.
  Custom parameters may be added as desired.

  
offview.desktop and x3dview.desktop - (for linux) shortcut to launch application

  convenience files for placement in your own .local/share/applications
  
  This assumes the python files are in your path.
  
  You may need to run these commands (check first without running them)
  cd ~/.local/share/applications
  desktop-file-install offview.desktop --dir ./local/share/applications --rebuild-mime-info-cache
  desktop-file-install x3dview.desktop --dir ./local/share/applications --rebuild-mime-info-cache
  
  This will allow a linux file manager to use 'open with' for an OFF file with
  offview.py with no parameters. Custom parameters can be added as desired.
  -- and --
  This will allow a linux file manager to use 'open with' for an VRML/X3D file with
  x3dview.py with no parameters. Custom parameters can be added as desired.
  
  Antiprism icons are also provided


offview.png (and offview.ico) - for the linux desktop file (or other use)
-- and --
x3dview.png (and x3dview.ico)

  Place the icon files in the following directory.
  
  ~/.local/share/icons
  
  icons can also show in the file manager if these icons exist
  
  ~/.local/share/icons/model-x-geomview-off.png
  ~/.local/share/icons/model-x3d+xml.png
  ~/.local/share/icons/model-vrml.png
  
  In Windows, the easiest way to set an icon for a file type is with the
  free file types manager
  
  https://www.nirsoft.net/utils/file_types_manager.html
