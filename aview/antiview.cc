/*
   Copyright (c) 2006-2016, Adrian Rossiter

   Antiprism - http://www.antiprism.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included
      in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

/* \file viewer_gl.cc
   \brief antiview - command line OFF file viewer
*/

#include "displaypoly_gl.h"
#include "vw_glut.h"

#include "../base/antiprism.h"

#include <string>
#include <unistd.h>
#include <vector>

using std::string;
using std::vector;

using namespace anti;

glut_state glut_s;

class vw_opts : public ViewOpts {
public:
  vw_opts() : ViewOpts("antiview") {}

  void process_command_line(int argc, char **argv);
  void usage();
};

void vw_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options] input_file

View a file in OFF format. If input file isn't given read from
standard input

Options
%s
%s
  -w <wdth> width of sphere containing points (default: calculated)
  -I <dist> maximum distance from origin for viewable points
            (default: ignored)
  Scene options
%s

Viewing
There are five main viewing control modes - rotation (default), drag,
zoom, spin, slice. Select a mode with the following keys, or right-click
on the window and select from the menu.

Click and drag on the window with the mouse to move the model, or use the
arrow keys. The zoom and slice are controlled by forward and backward
dragging, or the up and down arrow keys.

Menu Items
   r - rotate, turn the model about its centre
   d - drag, drag the model around the screen
   z - zoom, Zoom in or out
   s - spin control (when selected will first stop any spin, and add the
       current spin rotation to the current static rotation usually set
       with r)
   S - slice, slice into the model.
   P - projection, toggle display between perspective and orthogonal
   v - show/hide vertices, toggle display of vertex spheres
   e - show/hide edges, toggle display of edge rods
   f - show/hide faces, toggle display of faces
   n - show/hide vertex numbers, toggle display of vertex number labels
   N - show/hide face numbers, toggle display of face number labels
   m - show/hide edge numbers, toggle display of edge number labels
   M - toggle alternative number labels (planar face labels)
   t - rotate through display of face transparency: model, 50%%, none
   y - rotate through display of symmetry elements: axes; axes and
       mirrors; axes, mirrors and rot-reflection planes; none
   O - show/hide orientation, toggle colouring of the front side of 
       faces white and the back side black
   R - reset, reset to the initial viewing options 
   Q - Quit

Other Keys
   J/j - increase/decrease vertex and edge size
   K/k - increase/decrease vertex size
   L/l - increase/decrease edge size
   U/u - increase/decrease label text size
   c - toggle label colours between light and dark
   C - Invert label colours

)",
          prog_name(), help_ver_text, help_view_text, help_scene_text);
}

void vw_opts::process_command_line(int argc, char **argv)
{
  Status stat;
  opterr = 0;
  int c;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hv:e:iV:E:F:w:m:x:n:s:t:I:D:C:L:R:B:")) !=
         -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) { // Keep switch for consistency/maintainability
    default:
      if (!(stat = read_disp_option(c, optarg))) {
        if (stat.is_warning())
          warning(stat.msg(), c);
        else
          error(stat.msg(), c);
      }
    }
  }

  if (argc - optind >= 1)
    while (argc - optind >= 1)
      ifiles.push_back(argv[optind++]);
  else
    ifiles.push_back("");
}

Vec3d neg_z(Vec3d v)
{
  v[2] *= -1;
  return v;
}

#if GLUT_TYPE == FOUND_FLTKGLUT

bool glutInitFltk(int *argc, char **argv)
{
  vector<char *> argv_fl;        // FLTK options and parameters
  argv_fl.push_back(argv[0]);    // program name
  vector<char *> argv_other;     // Non-FLTK options and parameters
  argv_other.push_back(argv[0]); // program name

  // Divide original argument list into FLTK and non-FLTK entries
  for (int i = 1; i < *argc; i++) {
    if (strcmp(argv[i], "-geometry") == 0 || strcmp(argv[i], "-display") == 0) {
      argv_fl.push_back(argv[i]);
      ++i; // option needs parameter, so advance i
      if (i < *argc)
        argv_fl.push_back(argv[i]);
    }
    else if (strcmp(argv[i], "-iconic") == 0)
      argv_fl.push_back(argv[i]);
    else
      argv_other.push_back(argv[i]);
  }

  // Set up argument list for glutInit with FLTK options
  int argc_fl = argv_fl.size();
  for (int i = 0; i < argc_fl; i++)
    argv[i] = argv_fl[i];

  glutInit(&argc_fl, argv);
  if (argc_fl > 1) // not all aguments processed by FLTK
    return false;

  // Set up argument list with FLTK options removed
  *argc = argv_other.size();
  for (int i = 0; i < *argc; i++)
    argv[i] = argv_other[i];

  return true;
}
#endif

int main(int argc, char *argv[])
{
  // Check if a -geometry argument was given
  bool geometry_arg = false;
  for (int i = 1; i < argc; i++)
    if (strcmp(argv[i], "-geometry") == 0) {
      geometry_arg = true;
      break;
    }

  vw_opts opts;

#if GLUT_TYPE == FOUND_FLTKGLUT
  if (!glutInitFltk(&argc, argv))
    opts.error("invalid or missing FLTK Glut option parameter for "
               "-geometry or -display");
#else
  glutInit(&argc, argv);
#endif

  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  if (!geometry_arg) {
    glutInitWindowSize(600, 500);
    glutInitWindowPosition(100, 50);
  }
  glutCreateWindow("Antiview");
  glutDisplayFunc(display_cb);
  glutIdleFunc(glut_idle_cb);
  glutReshapeFunc(reshape_cb);
  glutMouseFunc(mouse_cb);
  glutKeyboardFunc(keyboard_cb);
  glutSpecialFunc(special_cb);
  glutMotionFunc(motion_cb);

  opts.set_geom_defs(DisplayPoly_gl());
  opts.set_num_label_defs(DisplayNumLabels_gl());
  opts.set_sym_defs(DisplaySymmetry_gl());
  opts.process_command_line(argc, argv);
  opts.set_view_vals(glut_s.scen);

  glut_s.save_camera();
  glut_s.init();
  glut_s.make_menu();

  glutMainLoop();
  return 0;
}
