/*
   Copyright (c) 2007-2023, Roger Kaufman

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

/*
   Name: n_icons.cc
   Description: Creates Sphericon like Polyhedra. Also known as Streptohedra
   Project: Antiprism - http://www.antiprism.com
*/

#include "n_icons.h"
#include "../base/antiprism.h"
#include "color_common.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::sort;
using std::string;
using std::swap;
using std::vector;

using namespace anti;

// Need these here since used in opts
bool half_model(const vector<int> &longitudes)
{
  return (2 * longitudes.back() == longitudes.front());
}

bool full_model(const vector<int> &longitudes)
{
  return (longitudes.front() == longitudes.back());
}

// returns angle in range of 0 to 359.999...
double angle_in_range(double angle, const double eps)
{
  while (angle < 0.0)
    angle += 360.0;
  while (angle >= 360.0)
    angle -= 360.0;

  if (double_eq(angle, 0.0, eps) || double_eq(angle, 360.0, eps))
    angle = 0.0;

  return angle;
}

// angle is either point cut or side cut
bool angle_on_aligned_polygon(const double angle, const double n,
                              const double eps)
{
  double ang = angle_in_range(angle, eps);
  bool ret = double_eq(ang, 180.0, eps);
  if (!ret)
    ret = double_eq(fmod(ang, 180.0 / n), 0.0, eps);
  return ret;
}

class ncon_opts : public ProgramOpts {
public:
  string ofile;

  int ncon_order = 4;         // default n_icon n
  int d = 1;                  // default n_icon d for n/d
  int twist = 1;              // default twist of n_icon is 1
  bool point_cut = true;      // actually means the dual
  bool hybrid = false;        // hybrid of the base and dual
  bool add_poles = false;     // add a vertex to the point cut model
  string closure;             // if an open model, method of closure
  vector<int> longitudes;     // how many longitudes and how many to display
  bool lon_invisible = false; // make longitudes invisible instead of deleted
  int build_method = 0;       // methods are 1,2 and 3
  bool hide_indent = true;    // for n/d shells, show indented edges
  double inner_radius = NAN;  // for -z 2, overide calculated radius
  double outer_radius = NAN;  // for -z 2, overide calculated radius
  double angle = 0;           // angle override for -z 3
  bool split_bypass = false;  // if -z 3 split bowties for twisting
  string hide_elems; // elements to hide v,e,f or E edges of invisible f
  bool add_symmetry_polygon = false; // append color polygon for -S,C coloring
  bool info = false;                 // output info of n_icon

  // listing variables
  string ncon_surf;            // what type of surface to list (see help)
  vector<int> ncon_range;      // range of n to list
  vector<int> filter_surfaces; // filter surfaces, 3 to infinity default
  bool long_form = false;      // long form of information
  bool filter_case2 = false;   // filter out case 2 types
  bool list_compounds = false; // alternatively list compounds
  int list_d = 1;              // needs to be a different variable than d

  double eps = anti::epsilon;

  // coloring
  char face_coloring_method = 's'; // default color by the symmetry algorithm
  char edge_coloring_method = 'Q'; // edge coloring method is none
  bool edge_set_no_color = false;  // to set no edge coloring
  Color unused_edge_color = Color::invisible; // edges not part of model
  bool symmetric_coloring = false;            // symmetry color for some modes
  bool circuit_coloring = false;              // circuit coloring for -I
  int flood_fill_stop = 0;                    // flood fill early stop
  Color face_default_color = Color(192, 192, 192, 255); // darkgray
  Color edge_default_color = Color(192, 192, 192, 255); // darkgray

  Coloring clrngs[3];

  int opacity[3] = {-1, -1, -1}; // transparency from 0 to 255, for v,e,f

  // default of 1 applies transparency to all faces and/or edges
  string face_pattern = "1"; // for face transparency patterns
  string edge_pattern = "1"; // for edge transparency patterns

  // common variables
  bool angle_is_side_cut = false; // if -z 3 angle is side cut
  bool double_sweep = false;      // double sweep is set in build_globe
  bool radius_inversion = false;  // if radius inversion occurs
  bool radius_set = false;        // radius is set by user
  int posi_twist = 0;             // often used mod of abs(twist)
  double twist_angle = 0;         // for a bug in process_hybrid
  int longitudes_save = 0;        // for longitudes alpha
  bool digons = false;            // keep track of digons
  bool d_of_one = false;          // keep track of D=1 or N-D=1
  bool n_doubled = false;         // if n is doubled for method 2

  // also used for apppended polygon so it doesn't interfere with edge counts
  vector<Geometry> polar_polygons;

  // former global variable
  bool split = false; // an original variable for splitting caps

  ncon_opts() : ProgramOpts("n_icons") {}

  void process_command_line(int argc, char **argv);
  void usage();
};

void ncon_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options]

Creates Sphericon like Polyhedra. Also known as Streptohedra

Options
%s
  -I        information on current n-icon  
  -l <lim>  minimum distance for unique vertex locations as negative exponent
               (default: %d giving %.0e)
  -o <file> write output to file (default: write to standard output)

Program Options
  -n <n/d>  n-icon of order n. n must be 3 or greater (default: 4)
               use d to make star n-icon. d may not equal n
  -t <twst> number of twists. Can be negative, positive or 0 (default: 1)
  -s        side-cut of even order n-icon (default is point-cut)
  -H        hybrid of even order n-icon
  -a        angle (-z 3 only)
  -r <num>  override inner radius (-z 2 only)
  -R <num>  override outer radius (-z 2 only)
  -b        don't split bowtie faces for twisting (-z 3, -t 0 only)
  -z <mthd> construction method
               1 - n/d must be co-prime. bow-ties can occur (default for d=1)
               2 - n/d compounds allowed. shell model (default for d>1)
               3 - n/d compounds allowed. No bow-ties (default if angle not 0)

Scene Options
  -M <m,m2> longitudes of model of m sides with optional m2 of m sides showing
               m may be odd, 3 or greater if twist is 0 (default: 36,36)
  -g        make m2 longitudes invisible instead of deletion (no -c)
  -A        place a north and south pole in top and bottom if they exist
               (twist 0 or n/2, causes indentation of caps with side cut)
  -c <clse> close open model if m2<m. Valid values h or v (-z 1,2)
               h = horizontal closure, v = vertical closure   
  -x <elms> v, e and f to remove OFF faces with one vertex (vertices),
               two-vertices (edges) and three or more vertices (faces)
               E - if face is invisible, associated edge is made invisible
  -Y        for n/d shells, when showing edges, show indented edges
  -W        add symmetry polygon (-f s,c or -e s)

Coloring Options (run 'off_util -H color' for help on color formats)
keyword: none - sets no color or methods. use: map_'color name'%% for one color
  -F <mthd> mthd is face coloring method. The coloring is done before twist
               s - color by symmetry polygon (default)
               f - color circuits with flood fill (-z 2,3 any n/d)
               c - color by compound
               k - color by compound with flood fill (-z 2,3 any n/d)
               note: the following color options may look better at twist 0
               l - color latitudinally
               m - color longitudinally
               b - checkerboard with first two colors in the color map
               n - use each color in map in succession
               x - first two colors based on sign of x
               y - first two colors based on sign of y
               z - first two colors based on sign of z (z is the twist plane)
               o - use first eight colors per xyz octants
  -E <mthd> mthd is edge coloring method. The coloring is done before twist
               keyword: Q - defer coloring all edges to option Q  (default)
                  or use the same letter options specified in -f, except c,k
               F - color edges with average adjoining face color
  -T <t,e>  transparency. from 0 (invisible) to 255 (opaque). element is any
            or all of e - edges, f - faces, a - all (default: f)
  -m <maps> color maps to be tried in turn. (default: colorful, circuits -C)
               optionally followed by elements from e or f (default: ef)
               use map name of 'index' to output index numbers
               colorful:  red,darkorange1,yellow,darkgreen,cyan,blue,magenta,
                          white,gray50,black
               circuits:  white,gray50 (continuous, discontinuous no wrapping)
  -S        color circuits symmetrically for coloring method s,f (even n)
  -C        color continuous and discontinuous circuits with first two colors
              in the color list (-I required, s,f coloring, no -S, no -z 2)
  -O <strg> face transparency pattern string. valid values
               0 -T n,f value suppressed, 1 -T n,f value applied (default: '1')
  -P <strg> edge transparency pattern string. valid values
               0 -T n,e value suppressed, 1 -T n,e value applied (default: '1')
  -Q <col>  color given to uncolored edges and vertices (default: invisible)
  -G <c,e>  default color c for uncolored elements e (default: darkgray,ef)
               elements e can include e or f
  -X <int>  flood fill stop. used with circuit or compound coloring (-f f,k)
               use 0 (default) to flood fill entire model. if -X is not 0 then
               return 1 from program if entire model has been colored

Surface (or Compound) Count Reporting (options above ignored)
  -L <type> types of n-icons to list. Valid values for type
               p = point cut even order n_icons
               s = side cut even order n-icons
               o = odd order n_icons
               h = hybrids (all)
               i = hybrids (where N/2 is even)
               j = hybrids (where N/2 is odd)
               k = hybrids (where N/4 is even)
               l = hybrids (where N/4 is odd)
  -N <n,n2> range of n-icons to list. n > 2
  -D <int>  set d of n/d for the report (default: 1)
  -K <k,k2> range of surfaces (or compounds) to list. k > 0 (default: 2,1000)
  -J        long form report
  -Z        filter out case 2 types (surfaces only)
  -B        list compounds instead of circuits
)",
          prog_name(), help_ver_text, int(-log(anti::epsilon) / log(10) + 0.5),
          anti::epsilon);
}

void ncon_opts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;
  int num;

  Split parts;

  vector<string> map_files;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv,
                     ":hn:t:sHa:r:R:bz:M:gA:c:x:YWISCO:P:Q:G:X:L:N:D:K:JZBF:E:"
                     "T:m:l:o:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'n': {
      char *p;
      p = strchr(optarg, '/');
      if (p != nullptr) {
        *p++ = '\0';
        print_status_or_exit(read_int(p, &d), "n/d (d part)");
      }

      print_status_or_exit(read_int(optarg, &ncon_order), "n/d (n part)");
      if (ncon_order < 3)
        error("n must be an integer 3 or greater", "n/d (n part)");
      if (d < 1)
        error("d must be 1 or greater", "n/d (d part)");

      if (d == ncon_order)
        error("d may not equal n", "n/d (d part)");

      if (d > ncon_order) {
        d = d % ncon_order;
        warning(
            msg_str("d is greater than n is changed to %d/%d", ncon_order, d),
            "n/d (d part)");
      }
      break;
    }

    case 't':
      print_status_or_exit(read_int(optarg, &twist), c);
      break;

    case 's':
      point_cut = false;
      break;

    case 'H':
      hybrid = true;
      break;

    case 'a':
      print_status_or_exit(read_double(optarg, &angle), c);
      break;

    case 'r':
      print_status_or_exit(read_double(optarg, &inner_radius), c);
      // if (inner_radius <= 0.0)
      //   error("inner radius must be greater than 0", c);
      break;

    case 'R':
      print_status_or_exit(read_double(optarg, &outer_radius), c);
      // if (outer_radius <= 0.0)
      //   error("outer radius must be greater than 0", c);
      break;

    case 'b':
      split_bypass = true;
      break;

    case 'z':
      print_status_or_exit(read_int(optarg, &build_method), c);
      if (build_method < 1 || build_method > 3)
        error("method must be between 1 and 3", c);
      break;

    case 'M':
      print_status_or_exit(read_int_list(optarg, longitudes, true, 2), c);
      if (longitudes.front() < 3)
        error("m must be 3 or greater", c);
      if (longitudes.size() == 1)
        longitudes.push_back(longitudes.front());
      else if (longitudes.size() == 2) {
        if (longitudes.front() < longitudes.back())
          error("sides shown (m2) must be less than or equal to longitudes (m)",
                c);
        if (longitudes.back() <= 0)
          error("sides shown (m2) must be at least 1", c);
      }
      break;

    case 'g':
      lon_invisible = true;
      break;

    case 'A':
      add_poles = true;
      break;

    case 'c':
      if (strspn(optarg, "hv") != strlen(optarg) || strlen(optarg) > 1)
        error(msg_str("closure is '%s', must be h or v (not both)", optarg), c);
      closure = optarg;
      break;

    case 'x':
      if (strspn(optarg, "vefE") != strlen(optarg))
        error(msg_str("elements to hide are '%s' must be from "
                      "v, e, f and E",
                      optarg),
              c);
      hide_elems = optarg;
      break;

    case 'Y':
      hide_indent = false;
      break;

    case 'W':
      add_symmetry_polygon = true;
      break;

    case 'S':
      symmetric_coloring = true;
      break;

    case 'C':
      circuit_coloring = true;
      break;

    case 'O':
      if (strspn(optarg, "01") != strlen(optarg))
        error(msg_str("transparency string is '%s', must consist of "
                      "0 and 1's",
                      optarg),
              c);
      face_pattern = optarg;
      break;

    case 'P':
      if (strspn(optarg, "01") != strlen(optarg))
        error(
            msg_str("transparency string %s must consist of 0 and 1's", optarg),
            c);
      edge_pattern = optarg;
      break;

    case 'Q':
      print_status_or_exit(unused_edge_color.read(optarg), c);
      break;

    case 'G': {
      Color tmp_color;

      bool set_edge = true;
      bool set_face = true;

      Split parts(optarg, ",");
      unsigned int parts_sz = parts.size();

      if (parts_sz > 2)
        error("excess entries for uncolored elements", c);

      for (unsigned int i = 0; i < parts_sz; i++) {
        // first argument is color
        if (i == 0) {
          print_status_or_exit(tmp_color.read(parts[i]), c);
        }
        else if (i == 1) {
          set_edge = false;
          set_face = false;

          if (strspn(parts[i], "ef") != strlen(parts[i]))
            error(msg_str("elements %s must be in e, f", optarg), c);

          if (strchr(parts[i], 'e'))
            set_edge = true;

          if (strchr(parts[i], 'f'))
            set_face = true;
        }
      }

      if (set_edge)
        edge_default_color = tmp_color;
      if (set_face)
        face_default_color = tmp_color;

      break;
    }

    case 'X':
      print_status_or_exit(read_int(optarg, &flood_fill_stop), c);
      if (flood_fill_stop < 0)
        error("flood fill stop must be 0 or greater", c);
      break;

    case 'L':
      if (strspn(optarg, "psohijkl") != strlen(optarg) || strlen(optarg) > 1)
        error(msg_str("n-icon type is '%s', must be only one of p, s, "
                      "o, h, i, j, k, or l\n",
                      optarg),
              c);
      ncon_surf = optarg;
      break;

    case 'N':
      print_status_or_exit(read_int_list(optarg, ncon_range, true, 2), c);
      if (ncon_range.front() < 3)
        error("n must be 3 or greater", c);
      if (ncon_range.size() == 1)
        ncon_range.push_back(ncon_range.front());
      else if ((ncon_range.size() == 2) &&
               (ncon_range.front() > ncon_range.back()))
        error("n2 shown must be greater than or equal to n", c);
      break;

    case 'K':
      print_status_or_exit(read_int_list(optarg, filter_surfaces, true, 2), c);
      if (filter_surfaces.front() < 1)
        error("k must be 2 or greater", c);
      if (filter_surfaces.size() == 1)
        filter_surfaces.push_back(filter_surfaces.front());
      else if ((filter_surfaces.size() == 2) &&
               (filter_surfaces.front() > filter_surfaces.back()))
        error("k2 shown must be greater than or equal to k", c);
      break;

    case 'D':
      print_status_or_exit(read_int(optarg, &list_d), c);
      if (list_d < 1)
        error("d must be 1 or greater", 'D');
      break;

    case 'J':
      long_form = true;
      break;

    case 'Z':
      filter_case2 = true;
      break;

    case 'B':
      list_compounds = true;
      break;

    // method q is undocumented as is the old color by circuit algorithm
    case 'F':
      if (!strcasecmp(optarg, "none"))
        face_coloring_method = '0';
      else if (strspn(optarg, "sflmbnxyzockq") != strlen(optarg) ||
               strlen(optarg) > 1)
        error(msg_str("invalid face coloring method '%c'", *optarg), c);
      else
        face_coloring_method = *optarg;
      break;

    case 'E':
      if (!strcasecmp(optarg, "none")) {
        edge_set_no_color = true;
        edge_coloring_method = 'n';
      }
      else if (!strcmp(optarg, "Q"))
        edge_coloring_method = 'Q';
      else if (strspn(optarg, "sflmbnxyzoFq") != strlen(optarg) ||
               strlen(optarg) > 1)
        error(msg_str("invalid edge coloring method '%s'", optarg), c);
      else
        edge_coloring_method = *optarg;
      break;

    case 'T': {
      int parts_sz = parts.init(optarg, ",");
      if (parts_sz > 2)
        error("the argument has more than 2 parts", c);

      print_status_or_exit(read_int(parts[0], &num), c);
      if (num < 0 || num > 255)
        error("face transparency must be between 0 and 255", c);

      // if only one part, apply to faces as default
      if (parts_sz == 1) {
        opacity[FACES] = num;
      }
      else if (parts_sz > 1) {
        if (strspn(parts[1], "vefa") != strlen(parts[1]))
          error(msg_str("transparency elements are '%s' must be any or all "
                        "from  v, e, f, a",
                        parts[1]),
                c);

        string str = parts[1];
        if (str.find_first_of("va") != string::npos)
          opacity[VERTS] = num;
        if (str.find_first_of("ea") != string::npos)
          opacity[EDGES] = num;
        if (str.find_first_of("fa") != string::npos)
          opacity[FACES] = num;
      }
      break;
    }

    case 'm':
      map_files.push_back(optarg);
      break;

    case 'I':
      info = true;
      break;

    case 'l':
      int sig_compare;
      print_status_or_exit(read_int(optarg, &sig_compare), c);
      if (sig_compare > DEF_SIG_DGTS)
        warning("limit is very small, may not be attainable", c);
      eps = pow(10, -sig_compare);
      break;

    case 'o':
      ofile = optarg;
      break;

    default:
      error("unknown command line error");
    }
  }

  if (argc - optind > 0)
    error("too many arguments");

  // surfaces subsystem
  if (ncon_surf.length()) {
    if (!ncon_range.size()) {
      error("for surfaces reporting -N must be specified", "N");
    }
    else if (!is_even(ncon_range.front()) &&
             ncon_range.front() == ncon_range.back()) {
      if (strchr(ncon_surf.c_str(), 'h') || strchr(ncon_surf.c_str(), 'i') ||
          strchr(ncon_surf.c_str(), 'j'))
        error("for listing hybrid surfaces n must be even", "N");
      else if (strchr(ncon_surf.c_str(), 'p'))
        error("for listing even order n-icon surfaces n must be even", "N");
      else if (strchr(ncon_surf.c_str(), 's'))
        error("for listing side cut n-icon surfaces n must be even", "N");
    }
    else {
      if (is_even(ncon_range.front()) &&
          ncon_range.front() == ncon_range.back())
        if (strchr(ncon_surf.c_str(), 'o'))
          error("for listing odd order n-icons surfaces n must be odd", "N");
    }

    if (circuit_coloring)
      error("circuit coloring cannot be used with listings", "C");

    // set defaults
    if (!filter_surfaces.size()) {
      filter_surfaces.push_back(2);
      filter_surfaces.push_back(1000);
    }
  }
  // n_icons option processing
  else {
    // if these surfaces subsystem parameters are set with ncon_surf, error
    if (filter_surfaces.size() > 0)
      error("is not valid without -L", "K");

    if (ncon_range.size() > 0)
      error("is not valid without -L", "N");

    if (long_form)
      error("is not valid without -L", "J");

    if (filter_case2)
      error("is not valid without -L", "Z");

    // can cause ret to be set with -B
    // if (list_compounds)
    //  error("not valid without -L", "B");

    // default longitudes to use is 36
    if (longitudes.size() == 0) {
      longitudes.push_back(36);
      longitudes.push_back(36);
    }

    // default build methods (old)
    if (!build_method) {
      // when n/1, build method 1, otherwise build method 2
      if (d == 1 || (ncon_order - d) == 1)
        build_method = 1;
      else
        build_method = 2;

      // radii setting only valid with method 2
      if (!std::isnan(inner_radius) || !std::isnan(outer_radius))
        build_method = 2;

      // angle is only valid with method 3
      if (angle)
        build_method = 3;

      // if flood fill or color by compound, need method 2 (or 3)
      if ((build_method == 1) && strchr("fk", face_coloring_method))
        build_method = 2;
    }

    if (!strchr("fk", face_coloring_method))
      if (flood_fill_stop > 0)
        error(
            "flood fill stop must not be set if not using flood fill (-F f,k)",
            "X");

    if (build_method == 1) {
      if (gcd(ncon_order, d) != 1)
        error("when construction method 1 (-z), n and d must be co-prime", "n");

      if ((d > 1) && (twist > 1) && (closure.length())) {
        error("closure not valid in construction method 1 (-z) with d greater "
              "than 1 and twist greater than 1",
              "c");
        closure.clear();
      }

      if (((longitudes.back() == 1) ||
           (longitudes.front() - longitudes.back() == 1)) &&
          (strchr(closure.c_str(), 'h'))) {
        error("when construction method 1 (-z), horizontal closure when "
              "used\nwith -M <m,m2> "
              "needs m2 to be greater than 1 or m-m2 to be greater than 1",
              "c");
        closure.clear();
      }

      if (flood_fill_stop > 0) {
        error("flood fill stop has no effect in construction method 1 (-z)",
              "X");
        flood_fill_stop = 0;
      }
    }

    if (build_method == 2) {
      // cases where some twists won't match front and back
      if (d == 1 || (ncon_order - d) == 1) {
        if (!std::isnan(inner_radius)) {
          if (hybrid) {
            if (is_even(twist))
              error("for method 2 and d=1 or n-d=1, hybrid, inner radius "
                    "cannot be set when twist is even",
                    "r");
          }
          else if (!is_even(ncon_order))
            error("for method 2 and d=1 or n-d=1, inner radius cannot be set "
                  "when N is odd",
                  "r");
          else if (!point_cut)
            error("for method 2 and d=1 or n-d=1, inner radius cannot be set "
                  "when side cut",
                  "r");
          else if (!is_even(twist))
            error("for method 2 and d=1 or n-d=1, inner radius cannot be set "
                  "when twist is odd",
                  "r");
        }
        else if (!std::isnan(outer_radius))
          inner_radius = outer_radius;
      }

      if (strchr(closure.c_str(), 'h')) {
        error("closure of h not valid in construction method 2 (-z)", "c");
        closure.clear();
      }

      if (add_poles) {
        if (!point_cut) {
          error("poles not valid with construction method 2 (-z) and side cut "
                "(-s)",
                "A");
          add_poles = false;
        }
        else if (!is_even(ncon_order)) {
          error("poles not valid with construction method 2 (-z) when N is odd",
                "A");
          add_poles = false;
        }
      }
    }
    else {
      if (!std::isnan(inner_radius)) {
        error("inner radius is only valid in construction method 2 (-z)", "r");
        inner_radius = NAN;
      }

      if (!std::isnan(outer_radius)) {
        error("outer radius is only valid in construction method 2 (-z)", "R");
        outer_radius = NAN;
      }

      if (!hide_indent) {
        error("show indented edges only valid in construction method 2 (-z)",
              "Y");
        hide_indent = true;
      }
    }

    if (build_method == 3) {
      if (angle && !point_cut)
        error("side cut is not valid when angle is not 0", "s");

      if (closure.length()) {
        error("closure options not valid with construction method 3 (-z)", "c");
        closure.clear();
      }

      // cannot have odd M with this method
      if (!is_even(longitudes.front()))
        error("-M m must be even with construction method 3 (-z)", "M");
    }
    else {
      if (angle)
        error("angle is only valid in construction method 3 (-z)", "a");
    }

    if (!hide_indent && (edge_coloring_method == 'Q'))
      error(
          "indented edges cannot be shown unless without an edge coloring (-E)",
          "Y");

    if (!point_cut) {
      if (!is_even(ncon_order))
        error("side cut is not valid with n-icons of odd n", "s");
    }

    if (hybrid) {
      if (!is_even(ncon_order))
        error("hybrids must be even order", "n");

      if (twist == 0)
        error("hybrids have no twist 0", "t");

      if (!point_cut && build_method != 3)
        error("hybrids and side cut can only be specified for construction "
              "method 3 (-z)",
              "s");

      if (2 * longitudes.back() < longitudes.front()) {
        warning("for hybrids m2 cannot be less than half of full model. "
                "setting to half model",
                "M");
        longitudes.back() = longitudes.front() / 2;
      }

      if (closure.length()) {
        error("closure options not valid with hybrids -H", "c");
        closure.clear();
      }

      if (add_poles) {
        error("poles not valid with hybrids -H", "A");
        add_poles = false;
      }

      // This is true only of the face coloring. Edge coloring can be different
      // only in build method 1 it is possible for hybrids to be colored the
      // same using symmetric coloring
      // if (hybrid && symmetric_coloring && build_method == 1)
      //   warning("symmetric coloring is the same an non-symmetric coloring for
      //   hybrids","S");
    }

    // Let us allow globes (Twist = 0) to have uneven number of longitudes
    if ((twist != 0) && (!is_even(longitudes.front())))
      error("if twist -t is not 0 then -M m must be even", "M");

    // Don't have poles on half or whole side cut models
    if ((half_model(longitudes) || full_model(longitudes)) && !point_cut) {
      if (add_poles)
        error("poles not valid with half or full side cut model", "A");
      add_poles = false;
    }

    // Cover longitudinal on side cut or odd n doesn't need poles
    if (strchr(closure.c_str(), 'h') &&
        (!point_cut || (!is_even(ncon_order)))) {
      if (add_poles)
        error("poles not valid on side cut or when n is odd when -c h", "A");
      add_poles = false;
    }

    // Twist needs split. Can't have poles. (except for hybrids)
    if ((twist != 0) && !hybrid) {
      split = true;
      if (add_poles && (!point_cut || (!is_even(ncon_order) ||
                                       (twist != (ncon_order / 2))))) {
        error("poles not valid when twist is not 0 or n/2 and point cut", "A");
        add_poles = false;
      }
    }

    if (split_bypass) {
      if (build_method != 3)
        error("split bypass requires construction method 3 (-z)", "b");

      if (twist)
        error("split bypass requires twist 0 (-t 0)", "b");
    }

    // Full models have no need for closure
    if (full_model(longitudes)) {
      if (closure.length())
        warning("closure not needed for full model", "c");
      closure.clear();
    }

    longitudes_save = longitudes.back();
    if (lon_invisible) {
      if (longitudes.back() < longitudes.front()) {
        longitudes.back() = longitudes.front();
      }
      else {
        warning("transparent longitudes have no effect on full models", "g");
        lon_invisible = false;
      }

      if (closure.length()) {
        warning("closure has no effect with transparent longitudes", "g");
        closure.clear();
      }
    }
  }

  if (add_symmetry_polygon) {
    if (!(strchr("sc", face_coloring_method)) &&
        (edge_coloring_method != 's')) {
      error("adding symmetry polygon only has effect for -f s,c or -e s", "W");
      add_symmetry_polygon = false;
    }
  }

  // circuit table works with co-prime n/d
  // if n/d is co-prime (and d>1) angle must be 0
  if (face_coloring_method == 'q') {
    if (gcd(ncon_order, d) != 1)
      error("circuit coloring only works n and d must be co-prime. Use 's' or "
            "'f'",
            'f');
    if (d > 1 && (gcd(ncon_order, d) == 1) && double_ne(angle, 0.0, eps))
      error("When n/d is co-prime, angle must be 0. Use 's' or 'f'", "f");
  }
  else if (face_coloring_method == 'f') {
    if (build_method == 1)
      error("flood fill face coloring is for construction method (-z 2,3)",
            "f");
  }
  else if (face_coloring_method == 'k') {
    if (build_method == 1)
      error("compound coloring is for construction method (-z 2,3)", "f");
  }

  if (edge_coloring_method == 'f') {
    if (build_method == 1)
      error("flood fill edge coloring is for construction method (-z 2,3)",
            "e");
  }

  if (symmetric_coloring && (!(strchr("sfq", face_coloring_method) ||
                               strchr("sfq", edge_coloring_method))))
    error("symmetric coloring is only for coloring methods s,f", "S");

  if (circuit_coloring) {
    if (!(strchr("sfq", face_coloring_method) ||
          strchr("sfq", edge_coloring_method)))
      error("circuit coloring is only for coloring methods s,f", "C");

    if (!full_model(longitudes))
      error("circuit coloring is only for full models (-M is set)", "C");

    if (symmetric_coloring)
      error("circuit coloring will not work with symmetric coloring -S", "C");

    if (build_method == 1 && d > 1)
      error("circuit coloring will not work for d > 1 with construction method "
            "1 (-z)",
            "C");

    if (build_method == 2) {
      warning("circuit coloring will not work when using construction method 2 "
              "(-z)",
              "C");
      warning("switching to build method -z 3", "C");
      build_method = 3;
    }

    if (!info) {
      warning("circuit coloring turns on -I", "C");
      info = true;
    }
  }

  // only for odd order n_icons to be colored the same using symmetric coloring
  if (!is_even(ncon_order) && symmetric_coloring)
    warning("symmetric coloring is the same an non-symmetric coloring for odd "
            "order n-icons",
            "S");

  // set all maps in list
  for (unsigned int i = 0; i < map_files.size(); i++)
    print_status_or_exit(read_colorings(clrngs, map_files[i].c_str()), 'm');

  // fill in missing maps
  string default_map_name = "colorful";
  for (unsigned int i = 0; i < 3; i++) {
    string map_name = default_map_name;
    if (circuit_coloring)
      map_name = "circuits";
    if (!clrngs[i].get_cmaps().size())
      clrngs[i].add_cmap(colormap_from_name(map_name.c_str()));
  }

  // patch for setting edges with no color
  if (edge_set_no_color) {
    clrngs[EDGES].del_cmap();
    clrngs[EDGES].add_cmap(alloc_no_colorMap());
  }

  // vertex color map is the same as the edge color map
  // if((clrngs[VERTS].get_cmaps()).size())
  //   warning("vertex map has no effect","m");

  if (build_method == 3) {
    angle_is_side_cut =
        double_eq(fmod(angle_in_range(angle, eps), 360.0 / ncon_order),
                  (180.0 / ncon_order), eps);
  }
}

// if map index or invisible, alpha cannot be changed
Color set_opacity(Color &c, const int a)
{
  // in n_icons, if fail just return the input
  Color c_bak = c;
  if (!c.set_alpha(a))
    c = c_bak;
  // possible display problem with opengl with small alpha. set invisible.
  // else if (!a)
  //  c = Color::invisible;
  return c;
}

// safe transparency setting. don't allow alpha set if
// color is index
// color is invisible
// -T has not been set
// c is not changed
void set_vert_color(Geometry &geom, const int i, Color c,
                    const int opacity = -1)
{
  if (opacity > -1)
    c = set_opacity(c, opacity);
  geom.colors(VERTS).set(i, c);
}

void set_edge_col(Geometry &geom, const int i, Color c, const int opacity = -1)
{
  if (opacity > -1)
    c = set_opacity(c, opacity);
  geom.colors(EDGES).set(i, c);
}

void set_face_color(Geometry &geom, const int i, Color c,
                    const int opacity = -1)
{
  if (opacity > -1)
    c = set_opacity(c, opacity);
  geom.colors(FACES).set(i, c);
}

void set_edge_and_verts_col(Geometry &geom, const int i, Color c,
                            const int opacity = -1)
{
  set_edge_col(geom, i, c, opacity);

  set_vert_color(geom, geom.edges()[i][0], c, opacity);
  set_vert_color(geom, geom.edges()[i][1], c, opacity);
}

void set_edge_color(Geometry &geom, const int i, Color c,
                    const int opacity = -1)
{
  set_edge_and_verts_col(geom, i, c, opacity);
}

bool triangle_zero_area(const Geometry &geom, const int idx1, const int idx2,
                        const int idx3, const double eps)
{
  const vector<Vec3d> &verts = geom.verts();
  Vec3d xprod = vcross(verts[idx1] - verts[idx2], verts[idx1] - verts[idx3]);
  return (double_eq(xprod[0], 0.0, eps) && double_eq(xprod[1], 0.0, eps) &&
          double_eq(xprod[2], 0.0, eps));
}

int longitudinal_faces(const int ncon_order, const bool point_cut)
{
  int lf = (int)floor((double)ncon_order / 2);
  if (is_even(ncon_order) && !point_cut)
    lf--;
  return (lf);
}

int num_lats(const int ncon_order, const bool point_cut)
{
  int lats = 0;
  if (is_even(ncon_order) && point_cut)
    lats = (ncon_order / 2);
  else if (is_even(ncon_order) && !point_cut)
    lats = (ncon_order / 2) + 1;
  else if (!is_even(ncon_order))
    lats = (int)ceil((double)ncon_order / 2);
  return lats;
}

int num_lats_faces(const vector<faceList *> &face_list)
{
  int lats = 0;
  for (auto i : face_list)
    if (i->lat > lats)
      lats = i->lat;
  // account for lat 0
  return lats + 1;
}

int num_lats_edges(const vector<edgeList *> &edge_list)
{
  int lats = 0;
  for (auto i : edge_list)
    if (i->lat > lats)
      lats = i->lat;
  return lats;
}

void add_coord(Geometry &geom, vector<coordList *> &coordinates,
               const Vec3d &vert)
{
  coordinates.push_back(new coordList(geom.add_vert(vert)));
}

void clear_coord(vector<coordList *> &coordinates)
{
  for (auto &coordinate : coordinates)
    delete coordinate;
  coordinates.clear();
}

void add_face(Geometry &geom, vector<faceList *> &face_list,
              const vector<int> &face, const int lat, const int lon)
{
  face_list.push_back(new faceList(geom.add_face(face), lat, lon, 0));
}

void add_face(Geometry &geom, vector<faceList *> &face_list,
              const vector<int> &face, const int lat, const int lon,
              const int polygon_no)
{
  face_list.push_back(new faceList(geom.add_face(face), lat, lon, polygon_no));
}

void clear_faces(vector<faceList *> &face_list)
{
  for (auto &i : face_list)
    delete i;
  face_list.clear();
}

void delete_face_list_items(vector<faceList *> &face_list,
                            const vector<int> &f_nos)
{
  vector<int> dels = f_nos;
  if (!dels.size())
    return;
  sort(dels.begin(), dels.end());
  unsigned int del_faces_cnt = 0;
  int map_to;
  for (unsigned int i = 0; i < face_list.size(); i++) {
    if (del_faces_cnt < dels.size() && (int)i == dels[del_faces_cnt]) {
      del_faces_cnt++;
    }
    else {
      map_to = i - del_faces_cnt;
      face_list[map_to] = face_list[i];
    }
  }
  face_list.resize(face_list.size() - del_faces_cnt);
}

// pass edge by value from make_edge()
// only add_edge_raw can be used else edge count is not correct for n_icons
void add_edge(Geometry &geom, vector<edgeList *> &edge_list,
              const vector<int> &edge, const int lat, const int lon)
{
  edge_list.push_back(new edgeList(geom.add_edge_raw(edge), lat, lon));
}

void clear_edges(vector<edgeList *> &edge_list)
{
  for (auto &i : edge_list)
    delete i;
  edge_list.clear();
}

void delete_edge_list_items(vector<edgeList *> &edge_list,
                            const vector<int> &f_nos)
{
  vector<int> dels = f_nos;
  if (!dels.size())
    return;
  sort(dels.begin(), dels.end());
  unsigned int del_edges_cnt = 0;
  int map_to;
  for (unsigned int i = 0; i < edge_list.size(); i++) {
    if (del_edges_cnt < dels.size() && (int)i == dels[del_edges_cnt]) {
      del_edges_cnt++;
    }
    else {
      map_to = i - del_edges_cnt;
      edge_list[map_to] = edge_list[i];
    }
  }
  edge_list.resize(edge_list.size() - del_edges_cnt);
}

class vertexMap {
public:
  int old_vertex;
  int new_vertex;
  vertexMap(int o, int n) : old_vertex(o), new_vertex(n) {}
};

void remap_elems(vector<vector<int>> &elems,
                 const vector<vertexMap> &coordinate_pairs)
{
  for (auto &elem : elems) {
    for (int &j : elem) {
      for (auto coordinate_pair : coordinate_pairs) {
        if (j == coordinate_pair.old_vertex)
          j = coordinate_pair.new_vertex;
      }
    }
  }
}

void merge_halves(Geometry &geom, vector<polarOrb *> &polar_orbit,
                  const ncon_opts &opts)
{
  const vector<Vec3d> &verts = geom.verts();

  vector<vertexMap> coordinate_pairs;

  for (unsigned int i = 0; i < polar_orbit.size(); i++) {
    int c1 = polar_orbit[i]->coord_no;
    for (unsigned int j = i + 1; j < polar_orbit.size(); j++) {
      int c2 = polar_orbit[j]->coord_no;
      if (!compare(verts[c1], verts[c2], opts.eps)) {
        coordinate_pairs.push_back(vertexMap(c1, c2));
        break;
      }
    }
  }

  remap_elems(geom.raw_faces(), coordinate_pairs);
  remap_elems(geom.raw_edges(), coordinate_pairs);

  geom.del(VERTS, geom.get_info().get_free_verts());
}

// method 3: prime polygon is analog of prime meridian
void build_prime_polygon(Geometry &geom, vector<int> &prime_meridian,
                         vector<coordList *> &coordinates,
                         const vector<poleList *> &pole, const ncon_opts &opts)
{
  // for finding poles, the accuracy must be less than the default
  double epsilon_local = 1e-8;
  if (opts.eps > epsilon_local)
    epsilon_local = opts.eps;

  int num_polygons = gcd(opts.ncon_order, opts.d);
  int base_polygon =
      (num_polygons == 1) ? opts.ncon_order : opts.ncon_order / num_polygons;

  bool compound = (num_polygons == 1) ? false : true;

  double arc = 360.0 / base_polygon * (opts.d / num_polygons);
  double interior_angle = (180.0 - arc) / 2.0;
  double radius = sin(deg2rad(interior_angle)) / sin(deg2rad(arc));

  // patch for 2N/N
  if (double_eq(arc, 180.0, opts.eps))
    radius = sin(deg2rad(90.0)) / sin(deg2rad(90.0));

  double ang = opts.angle;
  ang -= 90.0;

  // side cut
  if (is_even(opts.ncon_order) && !opts.point_cut)
    ang += 180.0 / opts.ncon_order;

  // ang = angle_in_range(ang,eps);

  for (int i = 0; i < opts.ncon_order; i++) {
    prime_meridian.push_back(i);
    add_coord(
        geom, coordinates,
        Vec3d(cos(deg2rad(ang)) * radius, sin(deg2rad(ang)) * radius, 0.0));
    if (double_eq(fmod(angle_in_range(ang, opts.eps), 360.0), 90.0,
                  epsilon_local)) {
      pole[0]->idx = geom.verts().size() - 1;
      pole[0]->lat = 0;
    }
    else if (double_eq(fmod(angle_in_range(ang, opts.eps), 360.0), 270.0,
                       epsilon_local)) {
      pole[1]->idx = geom.verts().size() - 1;
      // all coloring and circuits considered make it point cut. make it true
      pole[1]->lat = num_lats(opts.ncon_order, true);
    }
    ang += arc;
    if (compound) {
      if (!((i + 1) % base_polygon))
        ang += 360.0 / opts.ncon_order;
    }
  }
}

// reverse polygon indexes of a polygon mirrored on Y
void reverse_poly_indexes_on_y(Geometry &geom, vector<int> &polygon,
                               const ncon_opts &opts)
{
  const vector<Vec3d> &verts = geom.verts();
  vector<bool> swapped(polygon.size());

  // default epsilon too small
  double epsilon_local = 1e-8;
  if (opts.eps > epsilon_local)
    epsilon_local = opts.eps;

  for (unsigned int i = 0; i < polygon.size() - 1; i++) {
    if (swapped[i])
      continue;
    for (unsigned int j = i; j < polygon.size(); j++) {
      if (swapped[j])
        continue;
      // if the points have equal Y and have the same fabs(X) then the indexes
      // are mirror/swapped on Y
      if (double_eq(verts[polygon[i]][1], verts[polygon[j]][1],
                    epsilon_local) &&
          double_eq(fabs(verts[polygon[i]][0]), fabs(verts[polygon[j]][0]),
                    epsilon_local)) {
        swap(polygon[i], polygon[j]);
        swapped[i] = true;
        swapped[j] = true;
      }
    }
  }
}

// bypass is for testing. rotation will not work if true
vector<vector<int>> split_bow_ties(Geometry &geom,
                                   vector<coordList *> &coordinates,
                                   vector<int> &face, const ncon_opts &opts)
{
  const vector<Vec3d> &verts = geom.verts();
  vector<vector<int>> faces;

  // double the digon
  if (face.size() == 2) {
    face.push_back(face[0]);
    face.push_back(face[1]);
  }

  Vec3d intersection;
  if ((face.size() == 4) && !opts.split_bypass) {
    for (unsigned int i = 0; i < 2; i++) {
      intersection = segments_intersection(verts[face[i]], verts[face[i + 1]],
                                           verts[face[i + 2]],
                                           verts[face[(i + 3) % 4]], opts.eps);
      if (intersection.is_set()) {
        // make two points, move Z off z-plane plus and minus a little. to be
        // restored to zero later
        intersection[2] = opts.eps * 2.0;
        int v_front = find_vert_by_coords(geom, intersection, opts.eps);
        int v_back = v_front + 1;
        if (v_front == -1) {
          intersection[2] = opts.eps * 2.0;
          add_coord(geom, coordinates, intersection);
          v_front = verts.size() - 1;
          intersection[2] = -opts.eps * 2.0;
          add_coord(geom, coordinates, intersection);
          v_back = verts.size() - 1;
        }

        vector<int> triangle;
        triangle.push_back(face[1]);
        triangle.push_back(face[(i == 0) ? 2 : 0]);
        Vec3d ecent = centroid(verts, triangle);
        triangle.push_back((ecent[2] > 0.0) ? v_front : v_back);
        faces.push_back(triangle);

        triangle.clear();
        triangle.push_back(face[(i == 0) ? 0 : 2]);
        triangle.push_back(face[3]);
        ecent = centroid(verts, triangle);
        triangle.push_back((ecent[2] > 0.0) ? v_front : v_back);
        faces.push_back(triangle);

        break;
      }
    }
  }

  // if no faces were formed just return original face
  if (!faces.size())
    faces.push_back(face);

  return faces;
}

vector<int> find_face_by_lat_lon(const vector<faceList *> &face_list,
                                 const int lat, const int lon)
{
  vector<int> idx;
  for (unsigned int i = 0; i < face_list.size(); i++) {
    if (face_list[i]->lat == lat && face_list[i]->lon == lon)
      idx.push_back(i);
  }
  return idx;
}

vector<int> find_edge_by_lat_lon(const vector<edgeList *> &edge_list,
                                 const int lat, const int lon)
{
  vector<int> idx;
  for (unsigned int i = 0; i < edge_list.size(); i++) {
    if (edge_list[i]->lat == lat && edge_list[i]->lon == lon)
      idx.push_back(i);
  }
  return idx;
}

bool add_edge_wrapper(Geometry &geom, vector<edgeList *> &edge_list,
                      const vector<int> &edge, const int lat,
                      const int lon_front, const int lon_back)
{
  const vector<Vec3d> &verts = geom.verts();

  int edge_no = find_edge_in_edge_list(geom.edges(), edge);
  if (edge_no < 0) {
    double edge_z = centroid(verts, edge)[2];

    int lon = (edge_z > 0.0) ? lon_front : lon_back;
    add_edge(geom, edge_list, make_edge(edge[0], edge[1]), lat, lon);

    if (edge_z > 0.0) // || double_eq(edge_z,0.0,eps))
      edge_list.back()->rotate = true;
  }

  return (edge_no < 0 ? false : true);
}

// split_face_indexes is cleared after use
void apply_latitudes(const Geometry &geom,
                     vector<vector<int>> &split_face_indexes,
                     const vector<faceList *> &face_list,
                     const vector<edgeList *> &edge_list,
                     const vector<poleList *> &pole, const ncon_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<vector<int>> &edges = geom.edges();
  const vector<Vec3d> &verts = geom.verts();

  int n = opts.ncon_order;
  if (opts.build_method == 2)
    n /= 2;

  // save edge latitudes for restoring them in some cases
  vector<int> edge_lats_save;
  if (opts.build_method == 2) {
    for (unsigned int i = 0; i < edge_list.size(); i++) {
      int j = edge_list[i]->edge_no;
      edge_lats_save.push_back(edge_list[j]->lat);
    }
  }

  // collect Y value of edges and map them
  map<int, vector<int>> levels_edges;
  if (opts.build_method == 2) {
    for (auto i : edge_list) {
      int j = i->edge_no;
      if (i->lat < 0)
        continue;
      int level = i->lat - 1;
      levels_edges[level].push_back(j);
    }

    // if indented edges are used there will be gaps
    map<int, vector<int>> levels_edges_r;
    if (!opts.hide_indent) {
      unsigned int level_l = 0;
      unsigned int level_r = 0;
      while (level_l < levels_edges.size()) {
        if (levels_edges[level_l].size()) {
          levels_edges_r[level_r] = levels_edges[level_l];
          level_r++;
        }
        level_l++;
      }
      // levels_edges.clear();
      levels_edges = levels_edges_r;
    }

    // clear face latitudes
    for (auto i : face_list)
      i->lat = -1;
    // clear edge latitudes
    for (auto i : edge_list) {
      if (i->lat > -1)
        i->lat = -1;
    }
  }
  else if (opts.build_method == 3) {
    vector<pair<double, int>> edge_ys;
    for (auto i : edge_list) {
      int j = i->edge_no;
      double y = verts[edges[j][0]][1];
      edge_ys.push_back(make_pair(y, j));
    }

    sort(edge_ys.begin(), edge_ys.end());
    reverse(edge_ys.begin(), edge_ys.end());

    double last_y = std::numeric_limits<double>::max();
    int level = -1;
    for (auto &edge_y : edge_ys) {
      if (double_ne(edge_y.first, last_y, opts.eps))
        level++;
      last_y = edge_y.first;
      levels_edges[level].push_back(edge_y.second);
    }
  }

  bool do_faces = strchr("lbqfck", opts.face_coloring_method);
  bool do_edges = strchr("lb", opts.edge_coloring_method);
  if (do_faces || do_edges) {
    // put split faces into a map
    map<int, int> split_face_map;
    for (auto &split_face_index : split_face_indexes) {
      if (split_face_index.size() == 1) {
        split_face_map[split_face_index[0]] = split_face_index[0];
      }
      else {
        split_face_map[split_face_index[0]] = split_face_index[1];
        split_face_map[split_face_index[1]] = split_face_index[0];
      }
    }
    // split_face_indexes no longer needed
    split_face_indexes.clear();

    // each face is associated with one or two edges
    map<int, vector<int>> faces_edges_map;
    for (unsigned int i = 0; i < faces.size(); i++) {
      vector<int> face = faces[i];
      unsigned int sz = face.size();
      for (unsigned int j = 0; j < sz; j++) {
        vector<int> edge(2);
        edge[0] = face[j];
        edge[1] = face[(j + 1) % sz];
        int ret = find_edge_in_edge_list(edges, edge);
        if (ret > -1)
          faces_edges_map[i].push_back(ret);
      }
    }

    // each edge is associated with two faces
    map<int, vector<int>> edge_faces_map;
    for (unsigned int i = 0; i < faces_edges_map.size(); i++) {
      vector<int> face_idx = faces_edges_map[i];
      for (int k : face_idx) {
        edge_faces_map[k].push_back(i);
      }
    }

    vector<int> last_edges;
    int first_level = -1;
    int lat = 1;
    bool early_ending = false;
    bool early_mode = false;

    int part_number = 1;
    while (part_number) {
      // if south pole is included, colors are backward
      // if ((part_number == 1) && ((pole[0]->lat > -1) || (pole[1]->lat > -1)))
      // {
      //   int v_idx = (pole[0]->lat > -1) ? pole[0]->idx : pole[1]->idx;
      if ((part_number == 1) && (pole[0]->lat > -1)) {
        int v_idx = pole[0]->idx;
        vector<int> faces_with_index = find_faces_with_vertex(faces, v_idx);
        for (int j : faces_with_index) {
          face_list[j]->lat = lat - 1;
          int k = split_face_map[j];
          face_list[k]->lat = lat - 1;
          vector<int> edge_idx = faces_edges_map[k];
          for (int m : edge_idx) {
            if (edge_list[m]->lat == -1) {
              edge_list[m]->lat = lat;
              last_edges.push_back(m);
            }
          }
        }
      }
      // else need to prime side cut for level 0
      else {
        // find first level
        if (part_number == 1) {
          // if n is even, simple math
          if (is_even(n)) {
            first_level = opts.d / 2;
            if (opts.double_sweep)
              first_level *= 2;
          }
          // if n is odd, use vertex number to find level
          // first 'flat' level from the bottom
          else {
            if (opts.d_of_one)
              first_level = 0;
            else {
              if (is_even(opts.d))
                first_level = (n / 2) - (opts.d / 2);
              else
                first_level = opts.d / 2;
            }
          }
        }

        // set latitude on first edge level
        for (int j : levels_edges[first_level]) {
          edge_list[j]->lat = lat;
          last_edges.push_back(j);
        }

        // prime system to set latitudes on first set of faces
        for (int &last_edge : last_edges) {
          vector<int> face_idx = edge_faces_map[last_edge];

          // find higher face in terms of Y
          if (centroid(verts, faces[face_idx[1]])[1] >
              centroid(verts, faces[face_idx[0]])[1])
            swap(face_idx[0], face_idx[1]);
          // exception: if odd n, paint upper faces when y level in positive y
          double first_level_y =
              verts[edges[levels_edges[first_level][0]][0]][1];
          if (!is_even(n) && (!opts.d_of_one) &&
              double_gt(first_level_y, 0, opts.eps))
            swap(face_idx[0], face_idx[1]);

          int face_painted = -1;
          double edge_y = (verts[edges[last_edge][0]])[1];
          for (unsigned int j = 0; j < face_idx.size(); j++) {
            double face_cent_y = centroid(verts, faces[face_idx[j]])[1];
            if (double_eq(edge_y, face_cent_y, opts.eps)) {
              face_painted = j;
              break;
            }
          }

          // if not found: even n, face 0; odd n, face 1
          if (face_painted == -1)
            face_painted = (is_even(n)) ? 0 : 1;

          int j = face_idx[face_painted];
          int k = split_face_map[j];
          if ((face_list[j]->lat != -1) || (face_list[k]->lat != -1))
            continue;

          int para = 0;
          if (part_number == 1)
            para = 0;
          else
            para = lat;

          if (early_mode)
            para--;

          face_list[j]->lat = para;
          face_list[k]->lat = para;
        }
      }

      // loop to set rest of latitudes
      while (last_edges.size()) {
        vector<int> next_edges;
        for (unsigned int i = 0; i < last_edges.size(); i++) {
          vector<int> face_idx = edge_faces_map[last_edges[i]];

          // test for early ending...
          if (i == 0) {
            early_ending = true;
            for (int k : face_idx) {
              if (face_list[k]->lat == -1)
                early_ending = false;
            }
          }
          if (early_ending)
            break;

          for (int k : face_idx) {
            int l = split_face_map[k];
            if ((face_list[k]->lat != -1) || (face_list[l]->lat != -1))
              continue;

            int para = 0;
            if (part_number == 1)
              para = lat;
            else
              para = lat + 1;

            if (early_mode)
              para--;

            face_list[k]->lat = para;
            face_list[l]->lat = para;
            vector<int> edges_idx = faces_edges_map[l];
            for (int n : edges_idx) {
              if (edge_list[n]->lat == -1)
                next_edges.push_back(n);
            }
          }
        }

        lat++;

        for (int next_edge : next_edges)
          edge_list[next_edge]->lat = lat;
        last_edges = next_edges;
      }

      // check for any more unset edges
      bool found = false;
      for (unsigned int i = 0; i < levels_edges.size(); i++) {
        for (unsigned int j = 0; j < levels_edges[i].size(); j++) {
          int k = levels_edges[i][j];
          if (edge_list[k]->lat == -1) {
            found = true;
            first_level = i;
            break;
          }
        }
        // comment out to have prime circuits of parts>1 upside down
        if (found)
          break;
      }

      if (early_ending)
        early_mode = true;
      early_ending = false;

      if (found)
        part_number++;
      else
        part_number = 0;
    }
  }

  // restore edges so circuit table works
  bool reset_done = false;
  if (strchr("sf", opts.edge_coloring_method)) {
    if (opts.build_method == 2) {
      reset_done = true;
      for (unsigned int i = 0; i < edge_lats_save.size(); i++) {
        int lat = edge_lats_save[i];
        if (lat < -1)
          lat = (opts.hide_indent) ? -1 : std::abs(lat + 2);
        edge_list[i]->lat = lat;
      }
    }
    else if (opts.build_method == 3) {
      for (unsigned int i = 0; i < levels_edges.size(); i++) {
        for (unsigned int j = 0; j < levels_edges[i].size(); j++) {
          int k = levels_edges[i][j];
          edge_list[k]->lat = i + 1;
        }
      }
    }
  }

  // marked indented latitudes reset
  if (opts.build_method == 2 && !reset_done) {
    for (unsigned int i = 0; i < edge_lats_save.size(); i++) {
      int lat = edge_lats_save[i];
      if (lat < -1) {
        lat = (opts.hide_indent) ? -1 : std::abs(lat + 2);
        edge_list[i]->lat = lat;
      }
    }
  }
}

// This was the old method of apply_latitudes, it still works for double_sweep
// and d=1 for method 3: set latitude numbers based on height of Y
// set face latitudes based on edge latitudes
void apply_latitudes(const Geometry &geom,
                     const vector<vector<int>> &original_faces,
                     const vector<vector<int>> &split_face_indexes,
                     const vector<faceList *> &face_list,
                     const vector<edgeList *> &edge_list,
                     const vector<poleList *> &pole, const ncon_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<vector<int>> &edges = geom.edges();
  const vector<Vec3d> &verts = geom.verts();

  // possible future use when not coloring compounds
  // if false give each latitude unique number, if true pair each layer split by
  // angle
  bool double_sw; // = opts.double_sweep;
  double_sw = false;

  // possible future use
  // hybrid patch: for a hybrid when angle causes a double sweep, one side will
  // be off by 1
  bool hybrid_patch =
      (opts.hybrid && opts.point_cut && double_sw) ? true : false;

  // do edges first and use them as reference latitudes for faces
  // collect Y value of edges and sort them
  vector<pair<double, int>> edge_ys;
  for (unsigned int i = 0; i < edges.size(); i++)
    edge_ys.push_back(make_pair(verts[edges[i][0]][1], i));

  sort(edge_ys.begin(), edge_ys.end());
  reverse(edge_ys.begin(), edge_ys.end());

  int lat = 1;
  int wait = (hybrid_patch) ? 0 : 1;

  double last_y = edge_ys[0].first;
  for (auto &edge_y : edge_ys) {
    if (double_ne(edge_y.first, last_y, opts.eps)) {
      lat++;
      if (double_sw) {
        lat -= wait;
        wait = (wait) ? 0 : 1;
      }
    }
    int j = edge_y.second;
    edge_list[j]->lat = lat;
    last_y = edge_y.first;
  }

  // find faces connected to edges for latitude assignment
  // optimization: if face lats not needed then bypass
  bool do_faces = strchr("lbqfck", opts.face_coloring_method);
  if (!do_faces)
    return;

  // collect minimum Y values of faces
  vector<pair<double, int>> face_ys;
  for (unsigned int i = 0; i < original_faces.size(); i++) {
    vector<int> face = original_faces[i];
    double min_y = std::numeric_limits<double>::max();
    for (int j : face) {
      double y = verts[j][1];
      if (y < min_y)
        min_y = y;
    }
    face_ys.push_back(make_pair(min_y, i));
  }

  lat = 1;
  int max_lat_used = 0;

  // if there is a north pole
  if (pole[0]->idx != -1) {
    Vec3d np = verts[pole[0]->idx];
    for (unsigned int i = 0; i < faces.size(); i++) {
      vector<int> face = faces[i];
      for (int j : face) {
        Vec3d v = verts[j];
        if (!compare(v, np, opts.eps)) {
          face_list[i]->lat = 0;
          break;
        }
      }
    }
    lat++;
  }

  wait = (double_sw) ? 2 : 0;

  last_y = edge_ys[0].first;
  for (auto &edge_y : edge_ys) {
    if (double_ne(edge_y.first, last_y, opts.eps)) {
      lat = max_lat_used + 2;
      if (double_sw) {
        lat -= wait;
        wait = (wait) ? 0 : 2;
      }
      last_y = edge_y.first;
    }

    int j = edge_y.second;
    vector<int> face_idx = find_faces_with_edge(original_faces, edges[j]);
    // edge must have 2 faces
    if (face_idx.size() != 2)
      continue;

    // find higher face in terms of Y
    if (face_ys[face_idx[1]].first > face_ys[face_idx[0]].first)
      swap(face_idx[0], face_idx[1]);

    bool first_lat_used = true;
    for (unsigned int k = 0; k < 2; k++) {
      int l = face_idx[k];
      for (unsigned int m = 0; m < split_face_indexes[l].size(); m++) {
        int n = split_face_indexes[l][m];
        if (face_list[n]->lat == -1) {
          int lat_used = (!k || (k && !first_lat_used)) ? lat - 1 : lat;
          if (lat_used > max_lat_used)
            max_lat_used = lat_used;
          face_list[n]->lat = lat_used;
        }
        else {
          if (!k && (face_list[n]->lat != lat - 1))
            first_lat_used = false;
        }
      }
    }
  }
}

// method 3: fix polygon numbers for compound coloring
void fix_polygon_numbers(const vector<faceList *> &face_list,
                         const ncon_opts &opts)
{
  int lat = 0;
  unsigned int sz = 0;
  do {
    int polygon_min = std::numeric_limits<int>::max();
    for (unsigned int j = 0; j < 2; j++) {
      int lon = (opts.longitudes.front() / 2) - j;
      vector<int> idx = find_face_by_lat_lon(face_list, lat, lon);
      sz = idx.size();
      for (unsigned int k = 0; k < sz; k++) {
        int polygon_no = face_list[idx[k]]->polygon_no;
        if (polygon_no < polygon_min)
          polygon_min = polygon_no;
      }
    }

    if (polygon_min != std::numeric_limits<int>::max()) {
      for (auto j : face_list) {
        if (j->lat == lat)
          j->polygon_no = polygon_min;
      }
    }
    lat++;
  } while (sz);
}

// for method 3: analog to form_globe()
// maximum latitudes is set
void form_angular_model(Geometry &geom, const vector<int> &prime_meridian,
                        vector<coordList *> &coordinates,
                        vector<faceList *> &face_list,
                        vector<edgeList *> &edge_list,
                        const vector<poleList *> &pole,
                        vector<vector<int>> &original_faces,
                        vector<vector<int>> &split_face_indexes,
                        const int polygons_total, const ncon_opts &opts)
{
  const vector<Vec3d> &verts = geom.verts();

  double arc = 360.0 / opts.longitudes.front();

  int num_polygons = gcd(opts.ncon_order, opts.d);
  int base_polygon =
      (num_polygons == 1) ? opts.ncon_order : opts.ncon_order / num_polygons;

  vector<int> meridian_last;
  vector<int> meridian;

  for (int i = 1; i <= polygons_total; i++) {
    // move current meridian one back
    meridian_last = (i == 1) ? prime_meridian : meridian;

    if (i == polygons_total) {
      // if full sweep this works
      meridian = prime_meridian;
      if (!opts.double_sweep)
        reverse_poly_indexes_on_y(geom, meridian, opts);
    }
    else {
      // add one 'meridian' of points
      meridian.clear();
      for (int j = 0; j < opts.ncon_order; j++) {
        int m = prime_meridian[j];
        if (m == pole[0]->idx || m == pole[1]->idx)
          meridian.push_back(m);
        else {
          // Rotate Point Counter-Clockwise about Y Axis (looking down through Y
          // Axis)
          Vec3d v = Trans3d::rotate(0, deg2rad(arc * i), 0) * verts[m];
          add_coord(geom, coordinates, v);
          meridian.push_back(verts.size() - 1);
        }
      }
    }

    // calculate longitude in face list for coloring faces later. latitude
    // calculated after model creation
    int lon_back = (i - 1) % (opts.longitudes.front() / 2);
    int lon_front = lon_back + (opts.longitudes.front() / 2);

    vector<int> top_edge;
    vector<int> bottom_edge;
    vector<int> first_edge;

    int polygon_no = 0;

    for (int p = 0; p < num_polygons; p++) {
      int m_start = base_polygon * p;
      for (int j = 0; j <= base_polygon; j++) {
        // prime bottom edge
        if (j == 0) {
          int m = meridian[j + m_start];
          int ml = meridian_last[j + m_start];
          bottom_edge.clear();
          bottom_edge.push_back(m);
          if (m != ml) // not a pole
            bottom_edge.push_back(ml);
          first_edge = bottom_edge;
          continue;
        }

        // everything else is the top edge
        if (j == base_polygon)
          top_edge = first_edge;
        else {
          int m = meridian[j + m_start];
          int ml = meridian_last[j + m_start];
          top_edge.clear();
          top_edge.push_back(m);
          if (m != ml) // not a pole
            top_edge.push_back(ml);
        }

        vector<int> face;
        for (int &k : bottom_edge)
          face.insert(face.end(), k);
        // top edge is reversed);
        for (int k = top_edge.size() - 1; k >= 0; k--)
          face.insert(face.end(), top_edge[k]);

        vector<vector<int>> face_parts =
            split_bow_ties(geom, coordinates, face, opts);

        vector<int> split_face_idx;

        // if bow tie is split there will be 2 faces (2 triangles)
        for (auto &face_part : face_parts) {
          double face_z = centroid(verts, face_part)[2];

          int lon = (face_z > 0.0) ? lon_front : lon_back;
          add_face(geom, face_list, face_part, -1, lon, polygon_no);

          // keep track of the split face indexes
          split_face_idx.push_back(face_list.size() - 1);

          // the front face is what is rotated
          if (face_z > 0.0)
            face_list.back()->rotate = true;

          // always add edges to discover face latitudes
          if (top_edge.size() == 2)
            add_edge_wrapper(geom, edge_list, top_edge, -1, lon_front,
                             lon_back);
          if (bottom_edge.size() == 2)
            add_edge_wrapper(geom, edge_list, bottom_edge, -1, lon_front,
                             lon_back);
        }

        if (split_face_idx.size()) {
          original_faces.push_back(face);
          split_face_indexes.push_back(split_face_idx);
        }
        split_face_idx.clear();

        bottom_edge = top_edge;
      }

      polygon_no++;
    }
  }
}

// for method 2: to hide uneeded edges
// note that function used to reverse indented based on manual inner and outer
// radii
void mark_indented_edges_invisible(const vector<edgeList *> &edge_list,
                                   const vector<poleList *> &pole,
                                   const bool radius_reverse,
                                   const ncon_opts &opts)
{
  // for method 2 we used n/2
  int n = opts.ncon_order / 2;

  for (auto i : edge_list) {
    int lat = i->lat;
    bool set_invisible = (is_even(n) && opts.point_cut && !is_even(lat)) ||
                         (is_even(n) && !opts.point_cut && is_even(lat)) ||
                         (!is_even(n) && is_even(lat));
    if (radius_reverse)
      set_invisible = (set_invisible) ? false : true;
    // negative latitudes to temporarily label indented edges
    if (set_invisible) {
      int lat = i->lat;
      lat = -lat - 2;
      i->lat = lat;
    }
  }

  for (unsigned int i = 0; i < 2; i++) {
    if (pole[i]->idx > -1) {
      int lat = pole[i]->lat;
      bool set_invisible = (is_even(n) && opts.point_cut && !is_even(lat)) ||
                           (is_even(n) && !opts.point_cut && is_even(lat)) ||
                           (!is_even(n) && is_even(lat));
      if (radius_reverse)
        set_invisible = (set_invisible) ? false : true;
      if (set_invisible)
        pole[i]->lat = -1;
    }
  }
}

void restore_indented_edges(const vector<edgeList *> &edge_list,
                            const ncon_opts &opts)
{
  for (auto i : edge_list) {
    int lat = i->lat;
    if (lat < -1) {
      lat = (opts.hide_indent) ? -1 : std::abs(lat + 2);
      i->lat = lat;
    }
  }
}

// for method 2
// note: it is called with n and not 2n
// d is a copy
vector<pair<int, int>> get_lat_pairs(const int n, int d, const bool point_cut)
{
  vector<pair<int, int>> lat_pairs;
  pair<int, int> lats;

  // d < n/2
  if (d > n / 2)
    d = n - d;

  int max_lats = n - 1;

  bool toggle = (!is_even(n) || !point_cut) ? true : false;
  for (int i = 0; i < n; i++) {
    int next = (toggle) ? i - (2 * d - 1) : i + (2 * d - 1);
    toggle = (toggle) ? false : true;
    if (next > max_lats)
      next = n - (next % max_lats);
    else if (next < 0)
      next = std::abs(next) - 1;

    lats.first = i;
    lats.second = next;
    if (lats.first > lats.second)
      swap(lats.first, lats.second);

    lat_pairs.push_back(lats);
  }

  sort(lat_pairs.begin(), lat_pairs.end());
  auto li = unique(lat_pairs.begin(), lat_pairs.end());
  lat_pairs.erase(li, lat_pairs.end());

  return lat_pairs;
}

// for method 2: set latitude numbers for pairs of faces of shell models
// then split_face_indexes is filled
void find_split_faces_shell_model(const Geometry &geom,
                                  const vector<faceList *> &face_list,
                                  const vector<edgeList *> &edge_list,
                                  const vector<poleList *> &pole,
                                  vector<vector<int>> &split_face_indexes,
                                  const ncon_opts &opts)
{
  // for method 2 we used n/2
  int n = opts.ncon_order / 2;

  const vector<vector<int>> &faces = geom.faces();
  const vector<Vec3d> &verts = geom.verts();

  int lat = 0;
  if (opts.n_doubled) {
    // if radii were inverted, point_cut will have been reversed but not what
    // function is expecting
    bool pc = opts.point_cut;
    if (opts.radius_inversion)
      pc = !pc;
    vector<pair<int, int>> lat_pairs = get_lat_pairs(n, opts.d, pc);

    for (auto &lat_pair : lat_pairs) {
      for (auto j : face_list) {
        if (j->lat == lat_pair.first || j->lat == lat_pair.second)
          j->lat = lat;
      }
      lat++;
    }

    if (opts.hide_indent) {
      for (auto i : edge_list) {
        lat = i->lat;
        if (lat > 1) {
          int adjust = (is_even(lat)) ? 0 : 1;
          i->lat = (int)floor((double)lat / 2) + adjust;
        }
      }

      // fix south pole
      if (pole[1]->idx != -1) {
        lat = pole[1]->lat;
        if (lat > 1) {
          int adjust = (is_even(lat)) ? 0 : 1;
          pole[1]->lat = (int)floor((double)n / 2) + adjust;
        }
      }
    }
  }

  // collect split faces
  lat = 0;
  vector<pair<double, int>> face_zs;
  do {
    face_zs.clear();
    for (unsigned int i = 0; i < face_list.size(); i++) {
      // collect Z centroids of faces
      if (face_list[i]->lat == lat) {
        Vec3d face_cent = centroid(verts, faces[i]);
        double angle = angle_around_axis(face_cent, Vec3d(1, 0, 0), Vec3d::Y);
        face_zs.push_back(make_pair(angle, i));
      }
    }

    sort(face_zs.begin(), face_zs.end());
    reverse(face_zs.begin(), face_zs.end());

    vector<int> split_face_idx;
    double last_z = std::numeric_limits<double>::max();
    for (unsigned int i = 0; i < face_zs.size(); i++) {
      if (double_ne(face_zs[i].first, last_z, opts.eps)) {
        if (i > 0) {
          split_face_indexes.push_back(split_face_idx);
          split_face_idx.clear();
        }
        last_z = face_zs[i].first;
      }
      split_face_idx.push_back(face_zs[i].second);
    }
    if (split_face_idx.size())
      split_face_indexes.push_back(split_face_idx);
    lat++;
  } while (face_zs.size());
}

// if return_calc is true, it returns the calculated values of radius even if
// they are set before hand
// inner_radius, outer_radius, arc, d are changed
void calc_radii(double &inner_radius, double &outer_radius, double &arc,
                const int N, int &d, const ncon_opts &opts,
                const bool return_calc)
{
  // if shell model is created opts.ncon_order needs to be divided by 2 to get
  // the correct outer radius, except when d is 1
  arc = 360.0 / (N / ((opts.build_method == 2 && opts.n_doubled) ? 2 : 1)) * d;
  // but this causes a problem for 2N/N so make an exception (always 360/4 = 90)
  if (opts.build_method == 2 && double_eq(arc, 180.0, opts.eps))
    arc = 360.0 / N * d;

  double interior_angle = (180.0 - arc) / 2.0;
  double inner_radius_calc = 0;
  double outer_radius_calc = sin(deg2rad(interior_angle)) / sin(deg2rad(arc));
  if (std::isnan(outer_radius))
    outer_radius = outer_radius_calc;

  if (opts.build_method == 2) {
    // reculate arc
    arc = 360.0 / N;
    // interior angle (not used)
    // interior_angle = (180.0 - arc) / 2.0;

    int n_calc = N / ((opts.n_doubled) ? 2 : 1);
    if (2 * d > n_calc)
      d = n_calc - d;
    // formula furnished by Adrian Rossiter
    // r = R * cos(pi*m/n) / cos(pi*(m-1)/n)
    inner_radius_calc = (outer_radius_calc * cos(M_PI * d / n_calc) /
                         cos(M_PI * (d - 1) / n_calc));
    if (std::isnan(inner_radius))
      inner_radius = (!opts.n_doubled) ? outer_radius_calc : inner_radius_calc;
  }

  // patch: radii cannot be exactly 0
  if (double_eq(inner_radius, 0.0, opts.eps))
    inner_radius += 1e-4;
  if (double_eq(outer_radius, 0.0, opts.eps))
    outer_radius += 1e-4;

  if (return_calc) {
    outer_radius = outer_radius_calc;
    inner_radius = (!opts.n_doubled) ? outer_radius_calc : inner_radius_calc;
  }
}

// for methods 1 and 2: first longitude of vertices to form globe
// inner_radius, outer_radius set
// point_cut_calc changed from side cut to point cut if build method 2 and d > 1
void build_prime_meridian(Geometry &geom, vector<int> &prime_meridian,
                          vector<coordList *> &coordinates,
                          double &inner_radius, double &outer_radius,
                          bool &point_cut_calc, const ncon_opts &opts)
{
  int n = opts.ncon_order; // pass n and d as const
  int d = opts.d;
  double arc = 0;

  // for build method 2, n will be doubled
  calc_radii(inner_radius, outer_radius, arc, n, d, opts, false);

  bool radii_swapped = false;
  double angle = -90.0;
  // side cut
  if (is_even(opts.ncon_order) && !point_cut_calc) {
    if ((opts.build_method == 2) && opts.n_doubled) {
      swap(outer_radius, inner_radius);
      radii_swapped = true;
      // now treat it like a point cut
      point_cut_calc = true;
    }
    else {
      // angle += 180.0/opts.ncon_order;
      angle += (arc / 2.0);
    }
  }

  int num_vertices = longitudinal_faces(opts.ncon_order, point_cut_calc) + 1;
  for (int i = 0; i < num_vertices; i++) {
    prime_meridian.push_back(i);
    double radius =
        ((opts.build_method != 2) || is_even(i)) ? outer_radius : inner_radius;
    add_coord(
        geom, coordinates,
        Vec3d(cos(deg2rad(angle)) * radius, sin(deg2rad(angle)) * radius, 0));
    angle += arc;
  }

  // swap radii back for future reference
  if (radii_swapped)
    swap(outer_radius, inner_radius);
}

// methods 1 and 2: calculate polygon numbers
vector<int> calc_polygon_numbers(int n, int d, bool point_cut)
{
  int num_polygons = gcd(n, d);

  vector<int> polygon_numbers(n);
  int polygon_no = 0;
  for (int i = 0; i < n; i++) {
    polygon_numbers[i] = polygon_no;
    polygon_no++;
    if (polygon_no >= num_polygons)
      polygon_no = 0;
  }

  int lats = num_lats(n, point_cut);
  if (!point_cut)
    lats -= 2;

  int start = (point_cut) ? 1 : 0;
  unsigned int end = polygon_numbers.size() - 1;

  for (int i = start; i <= lats; i++) {
    if (polygon_numbers[i] > polygon_numbers[end])
      polygon_numbers[i] = polygon_numbers[end];
    end--;
  }

  polygon_numbers.resize(lats + (is_even(n) ? 1 : 0));
  // will be upside down versus lat in form_globe
  reverse(polygon_numbers.begin(), polygon_numbers.end());

  return (polygon_numbers);
}

// methods 1 and 2
void form_globe(Geometry &geom, const vector<int> &prime_meridian,
                vector<coordList *> &coordinates, vector<faceList *> &face_list,
                vector<edgeList *> &edge_list, const bool point_cut_calc,
                const bool second_half, const ncon_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<Vec3d> &verts = geom.verts();

  int half = opts.longitudes.front() / 2;

  // patch for polygon number in shell models
  vector<int> polygon_numbers;
  if (opts.build_method == 2)
    polygon_numbers = calc_polygon_numbers(
        opts.ncon_order / (opts.n_doubled ? 2 : 1), opts.d, opts.point_cut);

  // note: the algorithm can make "opts.longitudes.back()" faces but for method
  // 2 needs the whole model
  int longitudes_back = (opts.build_method == 1 || opts.hybrid)
                            ? opts.longitudes.back()
                            : opts.longitudes.front();
  int half_model_marker = 0;

  double arc = 360.0 / opts.longitudes.front();

  int lon_faces = longitudinal_faces(opts.ncon_order, point_cut_calc);
  // used to coordinate latitudes with bands
  int inc1 = (is_even(opts.ncon_order) && point_cut_calc) ? 0 : 1;

  // We close an open model longitudinally only if is desired, not a full model,
  // or only 1 zone is shown
  // only works for build method 1, build method 2 generates the whole model
  int longitudinal_cover = 0;
  if (strchr(opts.closure.c_str(), 'h') && !full_model(opts.longitudes) &&
      longitudes_back != 1)
    longitudinal_cover++;

  for (int i = 1; i <= longitudes_back + longitudinal_cover; i++) {
    for (unsigned int j = 0; j < prime_meridian.size(); j++) {
      if (((is_even(opts.ncon_order) && point_cut_calc) && (j != 0) &&
           ((int)j < prime_meridian.back())) ||
          (is_even(opts.ncon_order) && !point_cut_calc) ||
          (!is_even(opts.ncon_order) && (j > 0))) {
        if ((i != opts.longitudes.front()) && (i != longitudes_back + 1)) {
          // Rotate Point Counter-Clockwise about Y Axis (looking down through Y
          // Axis)
          Vec3d v = Trans3d::rotate(0, deg2rad(arc * i), 0) *
                    verts[prime_meridian[j]];
          add_coord(geom, coordinates, v);
        }
        if ((i == ((opts.longitudes.front() / 2) + 1)) &&
            (half_model_marker == 0))
          half_model_marker = verts.size() - 2;
      }

      bool use_prime;
      int k = 0;
      int l = 0;

      // store in face list for coloring faces later
      int lat = lon_faces - j + inc1;
      int lon = i - 1;

      if ((i == 1) || (i == opts.longitudes.front()) ||
          (i == longitudes_back + 1)) {
        use_prime = true;
        l = 0;

        // Reset coordinate count for last row of faces
        if ((i == opts.longitudes.front()) || (i == longitudes_back + 1)) {
          k = lon_faces - j;
          if (is_even(opts.ncon_order) && point_cut_calc)
            k--;
          // no color for opts.longitudes that span more than one meridian
          if (opts.longitudes.front() - longitudes_back > 1) {
            lat = -1;
            lon = -1;
          }
        }
      }
      else {
        use_prime = false;
        l = faces.size() - lon_faces;
      }

      // for second half of hybrid, adjust longitudes for later mirror image
      lon = (second_half ? std::abs(lon - half) + half - 1 : lon);

      // when j = 0 "prime the system"
      if (j > 0) {

        // patch for polygon number in shell models
        int polygon_no = 0;
        if (opts.build_method == 2) {
          double a = (double)lat / 2.0;
          int idx =
              (is_even(opts.ncon_order / 2)) ? (int)ceil(a) : (int)floor(a);
          if (!opts.point_cut && !is_even(j))
            idx--;
          polygon_no = polygon_numbers[idx];
        }

        if (((is_even(opts.ncon_order) && point_cut_calc) ||
             !is_even(opts.ncon_order)) &&
            (j == 1)) {
          // fprintf(stderr,"South Triangle\n");
          vector<int> face;
          face.push_back(verts.size() - 1 - k);
          face.push_back(prime_meridian.front());

          if (use_prime)
            face.push_back(prime_meridian[1]);
          else
            face.push_back(faces[l][0]);
          add_face(geom, face_list, face, lat, lon, polygon_no);

          // always add edge and can be deleted later. logic for method 1
          // unchanged
          if ((opts.edge_coloring_method != 'Q') || opts.build_method > 1)
            add_edge(geom, edge_list, make_edge(face[0], face[2]), lat, lon);
        }
        else if ((is_even(opts.ncon_order) && point_cut_calc) &&
                 (j == prime_meridian.size() - 1)) {
          // fprintf(stderr,"North Triangle\n");
          vector<int> face;
          face.push_back(verts.size() - 1);
          face.push_back(prime_meridian.back());

          if (use_prime)
            face.push_back(prime_meridian.back() - 1);
          else
            face.push_back(faces[l][0]);
          add_face(geom, face_list, face, lat, lon, polygon_no);
        }
        else {
          // fprintf(stderr,"Square\n");
          vector<int> face;
          face.push_back(verts.size() - 1 - k);
          face.push_back(verts.size() - 2 - k);

          if (use_prime) {
            face.push_back(prime_meridian[prime_meridian[j - 1]]);
            face.push_back(prime_meridian[prime_meridian[j]]);
          }
          else {
            face.push_back(faces[l][1]);
            face.push_back(faces[l][0]);
          }
          add_face(geom, face_list, face, lat, lon, polygon_no);

          // always add edge and can be deleted later. logic for method 1
          // unchanged
          if ((opts.edge_coloring_method != 'Q') || opts.build_method > 1) {
            add_edge(geom, edge_list, make_edge(face[0], face[3]), lat, lon);

            // patch with the extra edge below, one rotate flag gets missed
            if (half_model_marker != 0)
              edge_list.back()->rotate = true;

            // the last square has two edges if there is no bottom triangle
            if (lat == lon_faces &&
                (is_even(opts.ncon_order) && !point_cut_calc))
              add_edge(geom, edge_list, make_edge(face[1], face[2]), lat + 1,
                       lon);
          }
        }

        if (half_model_marker != 0) {
          face_list.back()->rotate = true;
          if ((opts.edge_coloring_method != 'Q') || opts.build_method > 1)
            edge_list.back()->rotate = true;
        }
      }
    }
  }
}

// add caps to method 1 and 2 models
// caps indexes are retained
void add_caps(Geometry &geom, vector<coordList *> &coordinates,
              vector<faceList *> &face_list, const vector<poleList *> &pole,
              vector<int> &caps, const bool point_cut_calc,
              const ncon_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<Vec3d> &verts = geom.verts();

  // note: faces but for method 2 needs the whole model
  vector<int> lons(2);
  lons.front() = opts.longitudes.front();
  lons.back() = (opts.build_method == 2 && !opts.hybrid)
                    ? opts.longitudes.front()
                    : opts.longitudes.back();

  int lats = num_lats(opts.ncon_order, point_cut_calc);
  // if twist is 0, there is only one cap so lower longitude value by 1 (method
  // 2)
  int lon_front =
      (opts.build_method == 2)
          ? lons.front() / 2 -
                ((opts.posi_twist == 0 || (opts.hybrid && !opts.point_cut)) ? 1
                                                                            : 0)
          : -1;
  int lon_back = (opts.build_method == 2) ? lon_front - 1 : -1;

  pole[0]->idx = -1;
  pole[0]->lat = -1;
  pole[1]->idx = -1;
  pole[1]->lat = lats;

  // Even order and point cut always have poles (and south pole = 0)
  if (is_even(opts.ncon_order) && point_cut_calc) {
    pole[0]->idx = longitudinal_faces(opts.ncon_order, point_cut_calc);
    pole[1]->idx = 0;
  }
  else if (!is_even(opts.ncon_order) || opts.add_poles) {
    if (opts.add_poles)
      pole[0]->idx = 0;
    if (!half_model(lons) || opts.add_poles)
      pole[1]->idx = 0;
  }

  if (pole[0]->idx > -1)
    pole[0]->lat = 0;

  if (opts.add_poles) {
    if (!strchr(opts.hide_elems.c_str(), 't') &&
        ((is_even(opts.ncon_order) && !point_cut_calc) ||
         !is_even(opts.ncon_order))) {
      Vec3d v;
      v[0] = 0;
      v[1] = verts.back()[1];
      v[2] = 0;
      add_coord(geom, coordinates, v);
      pole[0]->idx = verts.size() - 1;

      if (!strchr(opts.hide_elems.c_str(), 'b') &&
          ((is_even(opts.ncon_order) && !point_cut_calc))) {
        Vec3d v;
        v[0] = 0;
        v[1] = verts.front()[1];
        v[2] = 0;
        add_coord(geom, coordinates, v);
        pole[1]->idx = verts.size() - 1;
      }
    }
  }

  // Top
  if (!strchr(opts.hide_elems.c_str(), 't') &&
      ((is_even(opts.ncon_order) && !point_cut_calc) ||
       !is_even(opts.ncon_order))) {
    int j = 1;
    bool split_done = false;
    vector<int> face;
    for (int i = longitudinal_faces(opts.ncon_order, point_cut_calc) - 1;
         j <= lons.back();
         i += longitudinal_faces(opts.ncon_order, point_cut_calc)) {
      if (j == lons.back()) {
        if (!full_model(lons))
          face.push_back(faces[i].back());
        if (opts.split && !split_done && opts.add_poles &&
            (lons.back() > lons.front() / 2))
          face.push_back(pole[0]->idx);
        else if (!opts.split || (opts.split && split_done) ||
                 half_model(lons) ||
                 (!half_model(lons) && (lons.back() != (lons.front() / 2) + 1)))
          face.push_back(faces[i].front());
      }
      else
        face.push_back(faces[i].back());

      if (opts.split && !split_done && (j == (lons.front() / 2) + 1)) {
        add_face(geom, face_list, face, 0, lon_back);
        caps.push_back((int)face_list.size() - 1);
        face.clear();
        i -= longitudinal_faces(opts.ncon_order, point_cut_calc);
        j--;
        split_done = true;
      }
      j++;
    }

    if (split_done && !opts.add_poles)
      face.push_back((faces.back()).front());

    if (opts.add_poles)
      face.push_back(pole[0]->idx);

    add_face(geom, face_list, face, 0, lon_front);
    caps.push_back((int)face_list.size() - 1);
    if (!opts.hybrid)
      face_list.back()->rotate = true;
  }

  // Bottom
  if (!strchr(opts.hide_elems.c_str(), 'b') &&
      ((is_even(opts.ncon_order) && !point_cut_calc))) {
    int j = 1;
    bool split_done = false;
    vector<int> face;

    for (int i = 0; j <= lons.back();
         i += longitudinal_faces(opts.ncon_order, point_cut_calc)) {
      if (j == lons.back()) {
        if (!full_model(lons))
          face.push_back(faces[i][2]);
        if (opts.split && !split_done && opts.add_poles &&
            (lons.back() > lons.front() / 2))
          face.push_back(pole[1]->idx);
        else if (!opts.split || (opts.split && split_done) ||
                 half_model(lons) ||
                 (!half_model(lons) && (lons.back() != (lons.front() / 2) + 1)))
          face.push_back(faces[i][1]);
      }
      else
        face.push_back(faces[i][2]);

      if (opts.split && !split_done && (j == (lons.front() / 2) + 1)) {
        add_face(geom, face_list, face, lats - 1, lon_back);
        caps.push_back((int)face_list.size() - 1);
        face.clear();
        i -= longitudinal_faces(opts.ncon_order, point_cut_calc);
        j--;
        split_done = true;
      }

      j++;
    }

    if (split_done && !opts.add_poles)
      face.push_back((faces.back()).front());

    if (opts.add_poles)
      face.push_back(pole[1]->idx);

    add_face(geom, face_list, face, lats - 1, lon_front);
    caps.push_back((int)face_list.size() - 1);
    if (!opts.hybrid)
      face_list.back()->rotate = true;
  }
}

// for method 1 covering
void close_latitudinal(Geometry &geom, vector<faceList *> &face_list,
                       const vector<poleList *> &pole, const ncon_opts &opts)
{
  bool point_cut_calc = opts.point_cut;
  if (opts.build_method == 2 && !opts.d_of_one)
    point_cut_calc = true;

  vector<vector<int>> face(3);

  // Cover one side
  if (opts.add_poles && (is_even(opts.ncon_order) && !point_cut_calc))
    face[0].push_back(pole[1]->idx);

  int j = 0;
  for (int i = 1; i <= longitudinal_faces(opts.ncon_order, point_cut_calc) + 1;
       i++) {
    face[0].push_back(j);
    j++;
  }

  // If half model it is all one opts.closure, continue face[0] as face[1]
  if (half_model(opts.longitudes))
    face[1] = face[0];
  else {
    if (opts.add_poles && ((is_even(opts.ncon_order) && !point_cut_calc) ||
                           !is_even(opts.ncon_order)))
      face[0].push_back(pole[0]->idx);

    if (strchr(opts.closure.c_str(), 'v') && (face[0].size() > 2)) {
      add_face(geom, face_list, face[0], -2, -2);
      face_list.back()->rotate = true;
    }
  }

  // Cover other side
  j = geom.verts().size() - 1;

  if (opts.add_poles) {
    if (!is_even(opts.ncon_order))
      j--;
    else if (is_even(opts.ncon_order) && !point_cut_calc)
      j -= 2;
  }

  int k = longitudinal_faces(opts.ncon_order, point_cut_calc) + 1;
  if (opts.add_poles || (is_even(opts.ncon_order) && point_cut_calc)) {
    if (!half_model(opts.longitudes))
      face[1].push_back(pole[0]->idx);
    if (is_even(opts.ncon_order) && point_cut_calc)
      k--;
  }

  for (int i = 1; i < k; i++)
    face[1].push_back(j--);

  if (is_even(opts.ncon_order) && !point_cut_calc)
    face[1].push_back(j);

  if (!half_model(opts.longitudes)) {
    if (opts.add_poles || (((is_even(opts.ncon_order) && point_cut_calc) ||
                            !is_even(opts.ncon_order))))
      face[1].push_back(pole[1]->idx);
  }

  if (strchr(opts.closure.c_str(), 'v') && (face[1].size() > 2)) {
    add_face(geom, face_list, face[1], -2, -2);
    face_list.back()->rotate = true;
  }

  // In cases of odd opts.longitudes sides or even opts.longitudes side cuts
  // which are not half models this generates a third side to cover
  if (!opts.add_poles &&
      ((is_even(opts.ncon_order) && !point_cut_calc) ||
       !is_even(opts.ncon_order)) &&
      (!half_model(opts.longitudes)) && (strchr(opts.closure.c_str(), 'v'))) {
    face[2].push_back(0);
    if (is_even(opts.ncon_order) && !point_cut_calc)
      face[2].push_back(face[1].back());
    face[2].push_back(face[1].front());
    face[2].push_back(face[0].back());
    add_face(geom, face_list, face[2], -2, -2);
    face_list.back()->rotate = true;
  }
}

bool cmp_angle(const pair<pair<double, double>, int> &a,
               const pair<pair<double, double>, int> &b, const double eps)
{
  pair<double, double> ar_a = a.first;
  pair<double, double> ar_b = b.first;
  bool ret = double_eq(ar_a.first, ar_b.first, eps);
  if (ret)
    ret = double_lt(ar_a.second, ar_b.second, eps);
  else
    ret = double_lt(ar_a.first, ar_b.first, eps);
  return ret;
}

class angle_cmp {
public:
  double eps;
  angle_cmp(double ep) : eps(ep) {}
  bool operator()(const pair<pair<double, double>, int> &a,
                  const pair<pair<double, double>, int> &b) const
  {
    return cmp_angle(a, b, eps);
  }
};

// untangle polar orbit
void sort_polar_orbit(Geometry &geom, vector<polarOrb *> &polar_orbit,
                      const ncon_opts &opts)
{
  const vector<Vec3d> &verts = geom.verts();

  Vec3d v0 = verts[polar_orbit[0]->coord_no];
  unsigned int sz = polar_orbit.size();
  vector<pair<pair<double, double>, int>> angles(sz);
  for (unsigned int i = 0; i < sz; i++) {
    int j = polar_orbit[i]->coord_no;
    angles[i].second = j;
    pair<double, double> angle_and_radius;
    angle_and_radius.first = angle_in_range(
        rad2deg(angle_around_axis(v0, verts[j], Vec3d(0, 0, 1))), opts.eps);
    angle_and_radius.second = verts[j].len();
    angles[i].first = angle_and_radius;
  }

  // sort on angles
  sort(angles.begin(), angles.end(), angle_cmp(opts.eps));

  for (unsigned int i = 0; i < sz; i++)
    polar_orbit[i]->coord_no = angles[i].second;
}

void find_polar_orbit(Geometry &geom, vector<polarOrb *> &polar_orbit,
                      const ncon_opts &opts)
{
  vector<Vec3d> &verts = geom.raw_verts();
  for (unsigned int i = 0; i < verts.size(); i++) {
    if (double_eq(verts[i][2], 0.0, opts.eps)) {
      polar_orbit.push_back(new polarOrb(i));
    }
    // in case of build method 3, some points were set aside so not to get
    // into twist plane
    else if ((opts.build_method == 3) && fabs(verts[i][2]) < opts.eps * 3.0) {
      verts[i][2] = 0.0;
    }
  }

  // update for n/m models. works for all, so do it this way now
  sort_polar_orbit(geom, polar_orbit, opts);
}

// opts.twist_angle is set
void ncon_twist(Geometry &geom, const vector<polarOrb *> &polar_orbit,
                const vector<coordList *> &coordinates,
                const vector<faceList *> &face_list,
                const vector<edgeList *> &edge_list, const int ncon_order,
                const int twist, ncon_opts &opts)
{
  // this function wasn't designed for twist 0
  if (twist == 0)
    return;

  vector<vector<int>> &faces = geom.raw_faces();
  vector<vector<int>> &edges = geom.raw_edges();
  vector<Vec3d> &verts = geom.raw_verts();

  opts.twist_angle = (360.0 / ncon_order) * twist * (+1);
  Trans3d rot = Trans3d::rotate(0, 0, deg2rad(-opts.twist_angle));

  for (unsigned int i = 0; i < face_list.size(); i++) {
    if (face_list[i]->rotate) {
      for (int coord_no : faces[i]) {
        if (!coordinates[coord_no]->rotated) {
          verts[coord_no] = rot * verts[coord_no];
          coordinates[coord_no]->rotated = true;
        }
      }
    }
  }

  // Patch - If an open model, some of the polar circle coordinates are not yet
  // rotated. Also digons are sometimes a problem
  if (!full_model(opts.longitudes) || opts.digons)
    for (auto i : polar_orbit) {
      int j = i->coord_no;
      if (!coordinates[j]->rotated) {
        verts[j] = rot * verts[j];
        coordinates[j]->rotated = true;
      }
    }

  // Create Doubly Circularly Linked List
  for (unsigned int i = 0; i < polar_orbit.size(); i++)
    polar_orbit[i]->forward = i + 1;
  polar_orbit.back()->forward = 0;
  for (unsigned int i = polar_orbit.size() - 1; i > 0; i--)
    polar_orbit[i]->backward = i - 1;
  polar_orbit.front()->backward = polar_orbit.size() - 1;

  for (unsigned int k = 0; k < polar_orbit.size(); k++) {
    int p, q;
    p = polar_orbit[k]->coord_no;
    if (twist > 0)
      q = polar_orbit[k]->forward;
    else
      q = polar_orbit[k]->backward;
    for (int n = 1; n < std::abs(twist); n++) {
      if (twist > 0)
        q = polar_orbit[q]->forward;
      else
        q = polar_orbit[q]->backward;
    }
    q = polar_orbit[q]->coord_no;

    for (unsigned int i = 0; i < faces.size(); i++)
      if (!face_list[i]->rotate)
        for (int &j : faces[i])
          if (j == p)
            j = q * (-1);

    for (unsigned int i = 0; i < edges.size(); i++)
      if (!edge_list[i]->rotate)
        for (int &j : edges[i])
          if (j == p)
            j = q * (-1);
  }

  for (auto &face : faces)
    for (int &j : face)
      if (j < 0)
        j = std::abs(j);

  for (auto &edge : edges)
    for (int &j : edge)
      if (j < 0)
        j = std::abs(j);

  // edges may have been made to be out of numerical order
  for (auto &edge : edges)
    if (edge[0] > edge[1])
      swap(edge[0], edge[1]);
}

// ncon_order, d, point_cut, hybrid are not from opts
void find_circuit_count(const int twist, const int ncon_order, const int d,
                        const bool point_cut, const bool hybrid,
                        surfaceData &sd)
{
  bool digons = (ncon_order == 2 * d);

  // coding for total surface counts furnished by Adrian Rossiter
  int axis_edges = 1;
  int n = ncon_order;
  int t_mod = 0;

  if (hybrid) {
    n *= 2;
    t_mod = 1;
  }
  else if (is_even(ncon_order) && !point_cut)
    axis_edges = 2;

  sd.total_surfaces = (int)((gcd(n, 2 * twist - t_mod) + axis_edges) / 2);

  // continuous and discontinuous surfaces
  sd.c_surfaces = sd.total_surfaces - axis_edges;
  if ((is_even(ncon_order) && point_cut) && !hybrid)
    sd.c_surfaces++;
  sd.d_surfaces = sd.total_surfaces - sd.c_surfaces;

  // for twist 0
  int posi_twist = std::abs(twist % n);

  // edges. posi_twist 0 has no discontinuous edges
  // also adjustments for d > 1 and digon cases
  if (hybrid) {
    sd.c_edges = sd.total_surfaces - 1;
    sd.d_edges = 1;

    // adjust for digons
    if (digons) {
      sd.c_surfaces = sd.total_surfaces;
      sd.d_surfaces = 0;
      sd.total_surfaces = sd.c_surfaces + sd.d_surfaces;
    }
  }
  else if (is_even(ncon_order) && point_cut) {
    sd.c_edges = sd.c_surfaces - 1;
    sd.d_edges = (posi_twist == 0) ? 0 : 2;

    // adjust for d > 1
    if (digons) {
      sd.c_surfaces = sd.total_surfaces - 1;
      if (twist > 0)
        sd.c_surfaces += 2;
      sd.d_surfaces = 0;
      sd.total_surfaces = sd.c_surfaces + sd.d_surfaces;
    }
    else if ((d > 1) && is_even(d)) {
      sd.c_surfaces -= 1;
      sd.d_surfaces += 2;
      sd.total_surfaces = sd.c_surfaces + sd.d_surfaces;
    }
  }
  else if (is_even(ncon_order) && !point_cut) {
    sd.c_edges = sd.total_surfaces - 1;
    sd.d_edges = 0;

    // adjust for d > 1
    if (digons) {
      sd.c_surfaces = sd.total_surfaces - 1;
      sd.d_surfaces = 0;
      sd.total_surfaces = sd.c_surfaces + sd.d_surfaces;
    }
    else if ((d > 1) && !digons && is_even(d)) {
      sd.c_surfaces += 1;
      sd.d_surfaces -= 2;
      sd.total_surfaces = sd.c_surfaces + sd.d_surfaces;
    }
  }
  else if (!is_even(ncon_order)) {
    sd.c_edges = sd.c_surfaces;
    sd.d_edges = (posi_twist == 0) ? 0 : 1;
  }
  sd.total_edges = sd.c_edges + sd.d_edges;

  // calculate repeat twists
  int case1_twist = twist;
  sd.case2 = false;
  // not for minumum total surfaces
  if (sd.total_surfaces > 1) {
    if (hybrid)
      sd.case2 = (n % (2 * twist - 1) == 0) ? false : true;
    else
      // can't allow mod 0
      sd.case2 = (twist == 0) ? false : ((n % twist == 0) ? false : true);
    case1_twist = sd.total_surfaces;
    if (is_even(ncon_order) && !point_cut)
      case1_twist--;
  }
  sd.case1_twist = (sd.case2) ? case1_twist : twist;

  // compound parts
  // only formula for hybrids of d mod 4 have issues and only the following
  // models tested up to n=200 d=200
  // 144  : {144/48+23}
  // 192  : {192/48+23}
  // 192  : {192/48+47}
  // 144  : {144/96+23}
  // 192  : {192/144+23}
  // 192  : {192/144+47}

  n = ncon_order; // not 2n

  // digons case. may not necessarily be true
  if (n == 2 * d)
    sd.compound_parts = sd.total_surfaces;
  else {
    int de = (d > (n / 2)) ? n - d : d;
    int num_polygons = (int)gcd(n, de);
    sd.compound_parts = num_polygons;

    if (hybrid) {
      /* these statements are true but no longer needed
           // hybrids which have only 1 part based on d
           // when n is a power of 2
           if (ceil(log2(n)) == floor(log2(n)))
             sd.compound_parts = 1;
           // n-d covered by de
           // when gcd(n, d) and d is not a factor of n
           if ((int)gcd(n, de) == 1)
             sd.compound_parts = 1;
           // when gcd(n, d) is 2, number of polygons is 2
           if ((int)gcd(n, de) == 2)
             sd.compound_parts = 1;
     */
      // when d is a power of 2
      // not using de because d<(n/2) is tested for (n-d) being power of 2
      // example 56/24 is the same as 56/(56-24) or 56/32
      if (ceil(log2(d)) == floor(log2(d)))
        sd.compound_parts = 1;
      if (ceil(log2(n - d)) == floor(log2(n - d)))
        sd.compound_parts = 1;

      // sequence advanced by d
      t_mod = 0;
      if (!is_even(d))
        t_mod = de / 2;
      else {
        int dl = d;
        int dh = ncon_order - d;
        if (dl > dh)
          swap(dl, dh);
        de = dl;
        if ((dl % 4 == 0) && (dh % 4 != 0))
          de = dh;

        if (de % 4 != 0)
          t_mod = (de - 2) / 4;

        else if (de % 4 == 0) {
          // t_mod = (de - 4) / 8;
          t_mod = 200;
          for (int k = 1604; k > 8; k -= 8) {
            if (de % k == 0) {
              break;
            }
            t_mod -= 1;
          }
        }
      }
    }

    if (sd.compound_parts > 1) {
      int np = is_even(num_polygons) ? num_polygons / 2 : num_polygons;
      int gcd_calc = (int)gcd(np, twist + t_mod);
      sd.compound_parts =
          gcd_calc + (((is_even(n) && point_cut) || hybrid) ? 1 : 0);
      if (hybrid) {
        // all hybrids get one division
        sd.compound_parts /= 2;
        // hybrids of d mod 4 still has errors (e.g. n/d >= 144/48)
        if (de % 4 == 0) {
          // these cases need a second division
          if (is_even(twist + t_mod)) {
            sd.compound_parts = (int)ceil((double)sd.compound_parts / 2);
            // these cases need a third division
            // if ((gcd_calc % 4 == 0) && ((twist + t_mod) % np == 0)) {
            if (gcd_calc % 4 == 0) {
              sd.compound_parts = (int)ceil((double)sd.compound_parts / 2);
            }
          }
        }
      }
      // non-hybrids that have further calculations
      else if (!is_even(d) || (!is_even(n) && is_even(d))) {
        sd.compound_parts /= 2;
        if ((is_even(n) && !point_cut) || !is_even(n))
          sd.compound_parts++;
      }
    }
  }
}

// surface_table, sd will be changed
// ncon_order, point_cut, twist, hybrid, info are not from opts
void ncon_info(const int ncon_order, const int d, const bool point_cut,
               const int twist, const bool hybrid, const bool info,
               surfaceData &sd)
{
  int first, last, forms, chiral, nonchiral, unique;
  bool digons = (ncon_order == 2 * d);

  if (info) {
    fprintf(stderr, "\n");
    fprintf(stderr, "This is %s n-icon of order %d twisted %d time%s\n",
            (hybrid ? "a hybrid" : (point_cut ? "point cut" : "side cut")),
            ncon_order, twist, ((twist == 1) ? "" : "s"));

    fprintf(stderr, "\n");
    if (hybrid) {
      fprintf(stderr,
              "This is an order %d, hybrid n-icon. By hybrid, it means "
              "it is one half a\n",
              ncon_order);
      fprintf(stderr, "point cut n-icon joined to half of a side cut n-icon. "
                      "Hybrid n-icons are\n");
      fprintf(stderr, "of even order and have at least one discontinuous "
                      "surface and one\n");
      fprintf(stderr, "discontinuous edge. As a non-faceted smooth model it "
                      "would be self dual.\n");
      fprintf(stderr, "Circuit patterns are self dual.\n");
    }
    else if (is_even(ncon_order) && point_cut) {
      fprintf(stderr,
              "The order of this n-icon, %d, is even. It is, what is "
              "termed, a Point Cut.\n",
              ncon_order);
      fprintf(stderr, "Even Order, Point Cut n-icons have at least one "
                      "continuous surface and\n");
      fprintf(stderr, "two discontinuous edges. As a non-faceted smooth model "
                      "it would be dual\n");
      fprintf(stderr, "to the Side Cut n-icon. Circuit patterns are dual to "
                      "Side Cut n-icons.\n");
    }
    else if (is_even(ncon_order) && !point_cut) {
      fprintf(stderr,
              "The order of this n-icon, %d, is even. It is, what is "
              "termed, a Side Cut.\n",
              ncon_order);
      fprintf(stderr, "Even Order, Side Cut n-icons have at least two "
                      "discontinuous surfaces and\n");
      fprintf(stderr, "one continuous edge. As a non-faceted smooth model it "
                      "would be dual to the\n");
      fprintf(stderr, "Point Cut n-icon. Circuit patterns are dual to Point "
                      "Cut n-icons.\n");
    }
    else if (!is_even(ncon_order)) {
      fprintf(stderr,
              "The order of this n-icon, %d, is odd. It could be "
              "termed both a Point Cut\n",
              ncon_order);
      fprintf(stderr, "and a Side Cut depending on the poles. Odd order "
                      "n-icons have at least one\n");
      fprintf(stderr, "discontinuous surface and one discontinuous edge. As a "
                      "non-faceted smooth\n");
      fprintf(stderr,
              "model it would be self dual. Circuit patterns are self dual.\n");
    }
  }

  if (info && twist == 0) {
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Since it is twisted 0 times, it is as a globe with %d latitudes\n",
            num_lats(ncon_order, point_cut));

    if (is_even(ncon_order) && point_cut) {
      fprintf(stderr, "It has a north and south pole\n");
      fprintf(stderr,
              "It is the polyhedral dual of the twist 0 side cut order %d "
              "n-icon\n",
              ncon_order);
    }
    else if (is_even(ncon_order) && !point_cut) {
      fprintf(stderr, "It has two polar caps\n");
      fprintf(stderr,
              "It is the polyhedral dual of the twist 0 point cut "
              "order %d n-icon\n",
              ncon_order);
    }
    else if (!is_even(ncon_order)) {
      fprintf(stderr, "It has a north polar cap and south pole\n");
      fprintf(stderr, "As a polyhedra it is self dual\n");
    }
  }

  if (info && hybrid) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Note: Hybrid n-icons have no twist 0\n");
  }

  int base_form = 1;
  if (hybrid)
    base_form--;

  last = (int)floor((double)ncon_order / 2);
  first = -last;
  if (is_even(ncon_order) && !hybrid)
    first++;

  if (info) {
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Using coloring, there are %d distinct twists ranging from "
            "%d through +%d\n",
            ncon_order, first, last);
  }

  int mod_twist = twist % ncon_order;
  if (hybrid && mod_twist == 0)
    mod_twist = -1;
  int color_twist = mod_twist % last;
  if (is_even(ncon_order) && color_twist == -last)
    color_twist = std::abs(color_twist);
  if (mod_twist != color_twist) {
    if (mod_twist > last)
      color_twist = first + color_twist - 1;
    else if (mod_twist < first)
      color_twist = last + color_twist;
    else
      color_twist = mod_twist;
  }

  if (info) {
    if (color_twist != twist)
      fprintf(stderr, "This twist is the same as color twist %d\n",
              color_twist);
  }

  nonchiral = 0;
  if (is_even(ncon_order)) {
    if (hybrid)
      last = (int)floor((double)(ncon_order + 2) / 4);
    else
      last = (int)floor((double)ncon_order / 4);
    forms = (last * 2) + 1;
    first = -last;
    if ((!hybrid && ncon_order % 4 == 0) ||
        (hybrid && (ncon_order + 2) % 4 == 0)) {
      forms--;
      first++;
    }
    // no twist 0
    if (hybrid)
      forms--;
  }
  else {
    forms = ncon_order;
    last = (int)floor((double)forms / 2);
    first = -last;
  }

  chiral = last;
  if ((!hybrid && ncon_order % 4 == 0) ||
      (hybrid && (ncon_order + 2) % 4 == 0)) {
    nonchiral = 1;
    chiral--;
  }
  unique = forms - last - base_form + nonchiral;

  if (info) {
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Without coloring, there are %d distinct twists ranging "
            "from %d through +%d\n",
            forms, first, last);
    fprintf(stderr, "   %d base model (twist 0)\n", base_form);
    fprintf(stderr, "   %d form%s chiral and %s mirror image%s\n", chiral,
            ((chiral == 1) ? " is" : "s are"),
            ((chiral == 1) ? "has a" : "have"), ((chiral == 1) ? "" : "s"));
    fprintf(stderr, "   %d form%s non-chiral\n", nonchiral,
            ((nonchiral == 1) ? " is" : "s are"));
    fprintf(stderr, "   %d unique form%s of order %d twisted n-icon%s\n\n",
            unique, ((unique == 1) ? "" : "s"), ncon_order,
            ((unique == 1) ? "" : "s"));
  }

  int tmp_forms = forms;
  if (is_even(ncon_order))
    tmp_forms = ncon_order / 2;

  // also correct for no base twist 0 in hybrids
  int base_twist = twist;
  if (base_twist > 0)
    while (base_twist > last) {
      base_twist -= tmp_forms;
      if (hybrid && (base_twist <= 0 && base_twist + tmp_forms > 0))
        base_twist--;
    }
  else if (base_twist < 0)
    while (base_twist < first) {
      base_twist += tmp_forms;
      if (hybrid && (base_twist >= 0 && base_twist - tmp_forms < 0))
        base_twist++;
    }

  // using abs(base_twist) so much it might as well be stored
  int posi_twist = std::abs(base_twist);

  sd.nonchiral =
      (((chiral == 0 || base_twist == 0 ||
         (!hybrid && is_even(ncon_order) && (ncon_order % 4 == 0) &&
          base_twist == last) ||
         ((hybrid && (ncon_order + 2) % 4 == 0) && base_twist == last)))
           ? true
           : false);

  if (info) {
    if (twist < first || twist > last)
      fprintf(stderr, "It is the same as an n-icon of order %d twisted %d %s\n",
              ncon_order, base_twist, ((base_twist == 1) ? "time" : "times"));

    if (sd.nonchiral)
      fprintf(stderr, "It is not chiral\n");
    else
      fprintf(stderr,
              "It is the mirror image of an n-icon of order %d twisted %d "
              "time%s\n",
              ncon_order, -base_twist, ((-base_twist == 1) ? "" : "s"));
  }

  if (info) {
    fprintf(stderr, "\n");
    double twist_angle = ((double)360 / ncon_order) * twist;
    if (hybrid) {
      if (twist_angle > 0)
        twist_angle -= (double)360 / (ncon_order * 2);
      else
        twist_angle += (double)360 / (ncon_order * 2);
    }
    fprintf(stderr, "It has an absolute twist of %lf degrees, %lf radians\n",
            twist_angle, deg2rad(twist_angle));

    if (twist != base_twist) {
      twist_angle = ((double)360 / ncon_order) * base_twist;
      if (hybrid) {
        if (twist_angle > 0)
          twist_angle -= (double)360 / (ncon_order * 2);
        else
          twist_angle += (double)360 / (ncon_order * 2);
      }
      fprintf(stderr, "which is the same as one at %lf degrees, %lf radians\n",
              twist_angle, deg2rad(twist_angle));
    }
  }

  sd.c_surfaces = 0;
  sd.c_edges = 0;
  sd.d_surfaces = 0;
  sd.d_edges = 0;
  sd.total_surfaces = 0;
  sd.total_edges = 0;

  sd.case2 = false;
  sd.case1_twist = 0;

  sd.compound_parts = 0;

  // info will not be set from report subsystem
  find_circuit_count((info ? posi_twist : twist), ncon_order, d, point_cut,
                     hybrid, sd);

  if (!(d == 1 || (ncon_order - d) == 1)) {
    if (info) {
      fprintf(stderr, "\n");
      fprintf(stderr, "When d(%d) > 1, ", d);
    }
    if (hybrid) {
      if (info) {
        fprintf(stderr,
                "Hybrid n-icons have the same surface counts as when d = 1\n");
        if (digons)
          fprintf(stderr,
                  "But when n/d makes digons, all surfaces are continuous\n");
      }
    }
    else if (is_even(ncon_order)) {
      if (info) {
        if (digons)
          fprintf(stderr, "when digons, all surfaces are continuous\n");
        else
          fprintf(stderr, "and d is %s, ", (is_even(d)) ? "even" : "odd");
      }

      if (point_cut) {
        if (info) {
          if (digons) {
            if (twist == 0)
              fprintf(stderr,
                      "when twist is zero, there are two less surfaces\n");
            else if (ncon_order % 4 == 0)
              fprintf(stderr,
                      "when n mod 4, there will be one extra surface\n");
          }
          else {
            fprintf(stderr, "Even Order Point Cut n-icons have\n");
            if (is_even(d))
              fprintf(stderr, "one less continuous surface "
                              "and two additional discontinuous surfaces\n");
            else
              fprintf(stderr, "the same surface counts as when d = 1\n");
          }
        }
      }
      // side cut
      else {
        if (info) {
          if (digons) {
            if (twist == 0)
              fprintf(stderr,
                      "when twist is zero, there is one less surface\n");
            else if (ncon_order % 4 == 0)
              fprintf(stderr, "when n mod 4, there will be one less surface\n");
          }
          else {
            fprintf(stderr, "Even Order Side Cut n-icons have\n");
            if (is_even(d))
              fprintf(stderr, "one additional continuous surface and "
                              "two less discontinuous surfaces\n");
            else
              fprintf(stderr, "the same surface counts as when d = 1\n");
          }
        }
      }
    }
    else if (!is_even(ncon_order)) {
      if (info)
        fprintf(
            stderr,
            "Odd Order n-icons have the same surface counts as when d = 1\n");
    }

    if (info)
      fprintf(stderr, "edge circuit counts are the same as when d = 1\n");
  }

  if (info) {
    fprintf(stderr, "\n");
    fprintf(stderr,
            "Treated as conic, it has a total of %d surface%s and a "
            "total of %d edge%s\n",
            sd.total_surfaces, ((sd.total_surfaces == 1) ? "" : "s"),
            sd.total_edges, ((sd.total_edges == 1) ? "" : "s"));
    fprintf(stderr, "   %d surface%s continuous\n", sd.c_surfaces,
            ((sd.c_surfaces == 1) ? " is" : "s are"));
    fprintf(stderr, "   %d surface%s discontinuous\n", sd.d_surfaces,
            ((sd.d_surfaces == 1) ? " is" : "s are"));
    fprintf(stderr, "   %d surface%s total\n", (sd.total_surfaces),
            (((sd.total_surfaces) == 1) ? "" : "s"));
    fprintf(stderr, "----\n");
    fprintf(stderr, "   %d edge%s continuous\n", sd.c_edges,
            ((sd.c_edges == 1) ? " is" : "s are"));
    fprintf(stderr, "   %d edge%s discontinuous\n", sd.d_edges,
            ((sd.d_edges == 1) ? " is" : "s are"));
    fprintf(stderr, "   %d edge%s total\n", (sd.total_edges),
            (((sd.total_edges) == 1) ? "" : "s"));
    fprintf(stderr, "----\n");
    fprintf(stderr, "   %d compound part%s total\n", (sd.compound_parts),
            (((sd.compound_parts) == 1) ? "" : "s"));
  }

  if (info) {
    if (sd.total_surfaces > 1) {
      fprintf(stderr, "\n");
      if (!sd.case2) {
        fprintf(stderr, "This is a Case 1 n-icon. Surfaces cannot be colored "
                        "based on an earlier twist\n");
      }
      else {
        char ntype = 'p';
        if (hybrid)
          ntype = 'h';
        else if (is_even(ncon_order) && !point_cut)
          ntype = 's';
        fprintf(stderr, "This is a Case 2 n-icon. Surfaces can be colored "
                        "based on an earlier twist\n");
        fprintf(stderr, "when using unique colors to color surfaces\n");
        char sign = (twist > 0) ? '+' : '-';
        fprintf(stderr,
                "It can be derived by twisting N%d%cT%d%c %c%d increments\n",
                ncon_order, sign, sd.case1_twist, ntype, sign,
                posi_twist - sd.case1_twist);
      }
    }
    fprintf(stderr, "\n");
  }
}

// only faces of one color exist in paths
bool find_continuous_faces(Geometry &paths)
{
  bool continuous = true;

  // some caps are one face
  unsigned int sz = paths.faces().size();
  if (sz == 1)
    return !continuous;

  // orient even though it might not be orientable
  paths.orient(1);

  // coplanar faces imply caps and so discontinuous path
  // some caps have more than one face all on the same plane
  Geometry paths_no_zeros;
  paths_no_zeros.raw_verts() = paths.verts();
  for (unsigned int i = 0; i < sz; i++) {
    vector<int> face = paths.faces(i);
    // don't measure connector faces which can cause false 180.0
    if (triangle_zero_area(paths, face[0], face[1], face[2], anti::epsilon)) {
      continue;
    }
    paths_no_zeros.add_face(face);
  }

  GeometryInfo info(paths_no_zeros);
  vector<double> dihedrals = info.get_edge_dihedrals();
  bool coplanar_found = false;
  for (unsigned int i = 0; i < dihedrals.size(); i++) {
    double angle = dihedrals[i];
    if (double_eq(angle, M_PI, anti::epsilon)) {
      coplanar_found = true;
      break;
    }
  }
  if (coplanar_found)
    return !continuous;

  // test remaining paths to be continuous
  int connected_face_count = 0;
  for (unsigned int i = 0; i < sz; i++) {
    vector<int> face = paths.faces(i);
    unsigned int fsz = face.size();
    for (unsigned int j = 0; j < fsz; j++) {
      int v1 = face[j];
      int v2 = face[(j + 1) % fsz];
      vector<int> edge = make_edge(v1, v2);
      int edge_no = find_edge_in_edge_list(paths.edges(), edge);
      if (!(paths.colors(EDGES).get(edge_no)).is_set()) {
        vector<int> face_idx = find_faces_with_edge(paths.faces(), edge);
        connected_face_count += face_idx.size() - 1;
      }
    }
    if (connected_face_count == 1) {
      continuous = false;
      break;
    }
    connected_face_count = 0;
  }

  return continuous;
}

// only edges of one color exist in paths
bool find_continuous_edges(const Geometry &paths)
{
  bool continuous = true;

  int connected_edge_count = 0;
  for (unsigned int i = 0; i < paths.edges().size(); i++) {
    vector<int> edge = paths.edges(i);
    for (unsigned int j = 0; j < 2; j++) {
      vector<int> edge_idx = find_edges_with_vertex(paths.edges(), edge[j]);
      connected_edge_count += edge_idx.size() - 1;
    }
    if (connected_edge_count == 1) {
      continuous = false;
      break;
    }
    connected_edge_count = 0;
  }

  return continuous;
}

void color_circuits(Geometry &geom, const vector<pair<Color, int>> col_elems,
                    const bool face_elems, const Color current_col,
                    const bool continuous, const ncon_opts &opts)
{
  // map color 0 is for a continuous circuit that has no end point
  int n = (continuous) ? 0 : 1;
  int opq = 255;

  for (unsigned int i = 0; i < col_elems.size(); i++) {
    pair<Color, int> c = col_elems[i];
    if (c.first == current_col) {
      int j = c.second;
      if (face_elems) {
        opq = opts.face_pattern[n % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        set_face_color(geom, j, opts.clrngs[FACES].get_col(n), opq);
      }
      else {
        opq = opts.edge_pattern[n % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_edge_color(geom, j, opts.clrngs[EDGES].get_col(n), opq);
      }
    }
  }
}

void model_info(Geometry &geom, const ncon_opts &opts)
{
  GeometryInfo info(geom);
  bool closed = info.is_closed();
  fprintf(stderr, "the model is %sclosed\n", (closed) ? "" : "not ");
  fprintf(stderr, "the model is %scomplete\n",
          (full_model(opts.longitudes) || opts.lon_invisible) ? "" : "not ");
  if (opts.build_method == 2)
    opts.warning(
        "circuit counts are not measured with construction method 2 (-z)");

  // measured values
  surfaceData sdm;
  sdm.c_surfaces = 0;
  sdm.c_edges = 0;
  sdm.d_surfaces = 0;
  sdm.d_edges = 0;
  sdm.compound_parts = 0;

  bool sdm_surfaces = false;
  bool sdm_edges = false;
  bool sdm_compound_parts = false;

  // build method 2 has random coplanar faces
  // for method 2 was: opts.hide_indent && !opts.radius_set);
  bool measure = (!opts.symmetric_coloring &&
                  (full_model(opts.longitudes) || opts.lon_invisible) &&
                  (opts.build_method != 2));
  // when digons, will be miscounted if not edge_coloring_method s,q
  if (opts.digons && !(strchr("sq", opts.edge_coloring_method)))
    measure = false;

  if (strchr("qfsck", opts.face_coloring_method) && !opts.flood_fill_stop &&
      measure) {
    vector<Color> cols;
    vector<pair<Color, int>> col_faces;
    for (unsigned int i = 0; i < geom.faces().size(); i++) {
      Color c = geom.colors(FACES).get(i);
      if (c.is_set() && !c.is_invisible() && (c != opts.face_default_color)) {
        cols.push_back(c);
        col_faces.push_back(make_pair(c, i));
      }
    }

    if (strchr("sfq", opts.face_coloring_method)) {

      fprintf(stderr, "circuit counts are dependent on enough unique colors "
                      "in the map\n");

      sdm_surfaces = true;
      sort(col_faces.begin(), col_faces.end());

      Geometry paths = geom;
      paths.clear(FACES);

      pair<Color, int> c = col_faces[0];
      Color current_col = c.first;
      bool continuous = false;
      for (unsigned int i = 0; i < col_faces.size(); i++) {
        pair<Color, int> c = col_faces[i];
        if (current_col != c.first) {
          continuous = find_continuous_faces(paths);
          if (continuous)
            sdm.c_surfaces++;
          else
            sdm.d_surfaces++;

          if (opts.circuit_coloring)
            color_circuits(geom, col_faces, true, current_col, continuous,
                           opts);

          current_col = c.first;
          paths.clear(FACES);
        }
        paths.add_face(geom.faces(c.second));
      }

      // do the remaining path
      continuous = find_continuous_faces(paths);
      if (continuous)
        sdm.c_surfaces++;
      else
        sdm.d_surfaces++;

      if (opts.circuit_coloring)
        color_circuits(geom, col_faces, true, current_col, continuous, opts);

      fprintf(stderr, "%d face circuit%s continuous\n", sdm.c_surfaces,
              (sdm.c_surfaces == 1 ? "" : "s"));
      fprintf(stderr, "%d face circuit%s discontinuous \n", sdm.d_surfaces,
              (sdm.d_surfaces == 1 ? "" : "s"));

      sdm.total_surfaces = sdm.c_surfaces + sdm.d_surfaces;
    }

    sort(cols.begin(), cols.end());
    auto li = unique(cols.begin(), cols.end());
    cols.erase(li, cols.end());

    int sz = (int)cols.size();
    string s = (strchr("sfq", opts.face_coloring_method)) ? "face circuit"
                                                          : "compound part";
    fprintf(stderr, "%d %s%s counted\n", sz, s.c_str(),
            (sz == 1 ? " was" : "s were"));

    if (strchr("ck", opts.face_coloring_method)) {
      sdm_compound_parts = true;
      sdm.compound_parts = sz;

      fprintf(stderr, "compound counts are dependent on enough unique colors "
                      "in the map\n");
    }

    fprintf(stderr, "---------\n");
  }

  if ((strchr("sfq", opts.edge_coloring_method)) && measure) {
    vector<Color> cols;
    vector<pair<Color, int>> col_edges;
    for (unsigned int i = 0; i < geom.edges().size(); i++) {
      Color c = geom.colors(EDGES).get(i);
      if (c.is_set() && !c.is_invisible() && (c != opts.edge_default_color)) {
        col_edges.push_back(make_pair(c, i));
        cols.push_back(c);
      }
    }

    // something can go wrong in method 2 where all edges are invisible
    if (col_edges.size()) {
      sdm_edges = true;
      sort(col_edges.begin(), col_edges.end());

      Geometry paths = geom;
      paths.clear(FACES);
      paths.clear(EDGES);

      pair<Color, int> c = col_edges[0];
      Color current_col = c.first;
      bool continuous = false;
      for (unsigned int i = 0; i < col_edges.size(); i++) {
        pair<Color, int> c = col_edges[i];
        if (current_col != c.first) {
          bool continuous = find_continuous_edges(paths);
          if (continuous)
            sdm.c_edges++;
          else
            sdm.d_edges++;

          if (opts.circuit_coloring)
            color_circuits(geom, col_edges, false, current_col, continuous,
                           opts);

          current_col = c.first;
          paths.clear(FACES);
          paths.clear(EDGES);
        }
        paths.add_edge(geom.edges(c.second), c.first);
      }

      // do the remaining path
      continuous = find_continuous_edges(paths);
      if (continuous)
        sdm.c_edges++;
      else
        sdm.d_edges++;

      if (opts.circuit_coloring)
        color_circuits(geom, col_edges, false, current_col, continuous, opts);

      fprintf(stderr, "%d edge circuit%s continuous\n", sdm.c_edges,
              (sdm.c_edges == 1 ? "" : "s"));
      fprintf(stderr, "%d edge circuit%s discontinuous \n", sdm.d_edges,
              (sdm.d_edges == 1 ? "" : "s"));

      sdm.total_edges = sdm.c_edges + sdm.d_edges;
    }

    sort(cols.begin(), cols.end());
    auto li = unique(cols.begin(), cols.end());
    cols.erase(li, cols.end());

    int sz = (int)cols.size();
    fprintf(stderr, "%d edge circuit%s counted\n", sz,
            (sz == 1 ? " was" : "s were"));
  }

  int actual_order = opts.ncon_order;
  int actual_d = opts.d;
  int actual_twist = opts.twist;
  bool actual_cut = opts.point_cut;
  bool actual_hybrid = opts.hybrid;

  if (opts.angle != 0) {
    actual_cut = false; // side cut... unless...
    bool aligned = angle_on_aligned_polygon(opts.angle, actual_order, opts.eps);
    if (aligned)
      actual_cut = (opts.angle_is_side_cut) ? false : true;
    else {
      actual_order = opts.ncon_order * 2;
      actual_d = opts.d * 2;
      actual_twist = opts.twist * 2;
      actual_hybrid = false;
    }

    string model_str = "model";
    if (opts.hybrid) {
      if (!aligned)
        actual_twist -= ((opts.twist > 0) ? 1 : -1);
      model_str = "hybrid";
    }
    fprintf(stderr,
            "With angle %g, this %s is %s N = %d/%d twist "
            "= %d %s cut\n",
            opts.angle, model_str.c_str(), (aligned) ? "a" : "similar to",
            actual_order, actual_d, actual_twist,
            (actual_cut) ? "point" : "side");
  }

  if (opts.build_method == 2) {
    fprintf(stderr, "shell model radii: outer = %.17lf  inner = %.17lf\n",
            opts.outer_radius, opts.inner_radius);

    bool changed = false;
    if (opts.radius_set) {
      if (double_eq(opts.inner_radius, opts.outer_radius, opts.eps)) {
        // D != 1 or N-D != 1
        if (opts.n_doubled) {
          actual_order = opts.ncon_order * 2;
          actual_d = 1;
          actual_twist = opts.twist * 2;
          actual_cut = true;
          if (opts.hybrid) {
            actual_hybrid = false;
            actual_twist -= ((opts.twist > 0) ? 1 : -1);
          }
          changed = true;
        }
        // D == 1
        else {
          if (opts.hybrid) {
            actual_hybrid = false;
            changed = true;
          }
        }
      }
      else if (opts.radius_inversion && !actual_hybrid) {
        actual_cut = !actual_cut;
        changed = true;
      }
    }
    if (changed)
      fprintf(stderr, "n_icon has changed to %d/%d twist %d %s cut\n",
              actual_order, actual_d, actual_twist,
              (actual_cut) ? "point" : "side");
  }

  unsigned long fsz = geom.faces().size();
  unsigned long vsz = geom.verts().size();
  int edge_count = 0;
  for (unsigned int i = 0; i < geom.edges().size(); i++) {
    Color c = geom.colors(EDGES).get(i);
    if (c.is_set() && (c != opts.edge_default_color))
      edge_count++;
  }
  fprintf(stderr,
          "The model has %lu faces, %lu vertices, and %d colored edges\n", fsz,
          vsz, edge_count);

  fprintf(stderr, "========================================\n");

  surfaceData sd;
  ncon_info(actual_order, actual_d, actual_cut, actual_twist, actual_hybrid,
            opts.info, sd);

  if (sdm_surfaces || sdm_edges || sdm_compound_parts)
    fprintf(stderr, "comparison: ============================\n");

  if (sdm_surfaces) {
    fprintf(stderr, "measured continuous surfaces %s (%+d)\n",
            ((sdm.c_surfaces == sd.c_surfaces) ? "Agree" : "DISAGREE"),
            (sdm.c_surfaces - sd.c_surfaces));
    fprintf(stderr, "measured discontinuous surfaces %s (%+d)\n",
            ((sdm.d_surfaces == sd.d_surfaces) ? "Agree" : "DISAGREE"),
            (sdm.d_surfaces - sd.d_surfaces));
    fprintf(stderr, "total surfaces %s (%+d)\n",
            ((sdm.total_surfaces == sd.total_surfaces) ? "Agree" : "DISAGREE"),
            (sdm.total_surfaces - sd.total_surfaces));
  }

  if (sdm_edges) {
    fprintf(stderr, "measured continuous edges %s (%+d)\n",
            ((sdm.c_edges == sd.c_edges) ? "Agree" : "DISAGREE"),
            (sdm.c_edges - sd.c_edges));
    fprintf(stderr, "measured discontinuous edges %s (%+d)\n",
            ((sdm.d_edges == sd.d_edges) ? "Agree" : "DISAGREE"),
            (sdm.d_edges - sd.d_edges));
    fprintf(stderr, "total edges %s (%+d)\n",
            ((sdm.total_edges == sd.total_edges) ? "Agree" : "DISAGREE"),
            (sdm.total_edges - sd.total_edges));
  }

  if (sdm_compound_parts) {
    fprintf(stderr, "total compound parts %s (%+d)\n",
            ((sdm.compound_parts == sd.compound_parts) ? "Agree" : "DISAGREE"),
            (sdm.compound_parts - sd.compound_parts));
  }

  if (sdm_surfaces || sdm_edges || sdm_compound_parts)
    fprintf(stderr, "========================================\n");
}

void color_uncolored_faces(Geometry &geom, const ncon_opts &opts)
{
  for (unsigned int i = 0; i < geom.faces().size(); i++) {
    if (!(geom.colors(FACES).get(i)).is_set())
      geom.colors(FACES).set(i, opts.face_default_color);
  }
}

void color_uncolored_edges(Geometry &geom, const ncon_opts &opts)
{
  const vector<vector<int>> &edges = geom.edges();
  const vector<Vec3d> &verts = geom.verts();

  for (unsigned int i = 0; i < edges.size(); i++) {
    if (!(geom.colors(EDGES).get(i)).is_set())
      geom.colors(EDGES).set(i, opts.edge_default_color);
  }

  for (unsigned int i = 0; i < verts.size(); i++) {
    if (!(geom.colors(VERTS).get(i)).is_set())
      geom.colors(VERTS).set(i, opts.edge_default_color);
  }
}

void color_unused_edges(Geometry &geom, const Color &unused_edge_color)
{
  geom.add_missing_impl_edges();
  for (unsigned int i = 0; i < geom.edges().size(); i++) {
    if (!(geom.colors(EDGES).get(i)).is_set())
      geom.colors(EDGES).set(i, unused_edge_color);
  }

  for (unsigned int i = 0; i < geom.verts().size(); i++) {
    if (!(geom.colors(VERTS).get(i)).is_set())
      geom.colors(VERTS).set(i, unused_edge_color);
  }
}

void reassert_colored_verts(Geometry &geom, const Color &default_color,
                            const Color &unused_edge_color)
{
  const vector<vector<int>> &edges = geom.edges();

  for (unsigned int i = 0; i < geom.edges().size(); i++) {
    Color c = geom.colors(EDGES).get(i);
    // if it is an index or it is a value and is completely opaque
    // and not the unused edge color
    if (c.is_set() && (c.is_index() || !c.get_transparency()) &&
        c != unused_edge_color) {
      int v1 = edges[i][0];
      int v2 = edges[i][1];
      Color cv1 = geom.colors(VERTS).get(v1);
      Color cv2 = geom.colors(VERTS).get(v2);
      // let default color be dominant
      if (cv1.is_set() && cv1 != default_color)
        geom.colors(VERTS).set(v1, c);
      if (cv2.is_set() && cv2 != default_color)
        geom.colors(VERTS).set(v2, c);
    }
  }
}

void unset_marked_elements(Geometry &geom)
{
  // rarely faces can be max index
  for (unsigned int i = 0; i < geom.faces().size(); i++) {
    Color c = geom.colors(FACES).get(i);
    if (c.is_maximum_index()) {
      geom.colors(FACES).set(i, Color());
    }
  }

  for (unsigned int i = 0; i < geom.edges().size(); i++) {
    Color c = geom.colors(EDGES).get(i);
    if (c.is_maximum_index()) {
      geom.colors(EDGES).set(i, Color());
    }
  }

  for (unsigned int i = 0; i < geom.verts().size(); i++) {
    Color c = geom.colors(VERTS).get(i);
    if (c.is_maximum_index()) {
      geom.colors(VERTS).set(i, Color());
    }
  }
}

// point cut is not from opts
void ncon_edge_coloring(Geometry &geom, const vector<edgeList *> &edge_list,
                        const vector<poleList *> &pole,
                        map<int, pair<int, int>> &edge_color_table,
                        const bool point_cut_calc, const ncon_opts &opts)
{
  const vector<vector<int>> &edges = geom.edges();
  const vector<Vec3d> &verts = geom.verts();

  // special situation
  bool pc = opts.point_cut;
  if (opts.build_method == 3 && opts.hybrid && opts.angle_is_side_cut) {
    if (pc != point_cut_calc)
      pc = point_cut_calc;
  }
  if (opts.radius_inversion)
    pc = !pc;

  int opq = 255;

  if (opts.edge_coloring_method == 'q') {
    int circuit_count = 0;

    for (auto i : edge_list) {
      int j = i->edge_no;
      int lat = i->lat;

      if (i->lat < 0) {
        set_edge_color(geom, j, Color(), opts.opacity[EDGES]);
      }
      else {
        int col_idx = 0;
        if (i->rotate || (opts.hybrid && pc)) // front side
          col_idx = edge_color_table[lat].second;
        else
          col_idx = edge_color_table[lat].first;

        if (col_idx > circuit_count)
          circuit_count = col_idx;

        opq = opts.edge_pattern[col_idx % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_edge_color(geom, j, opts.clrngs[EDGES].get_col(col_idx), opq);
      }
    }

    for (unsigned int i = 0; i < 2; i++) {
      if (pole[i]->idx > -1) {
        int lat = pole[i]->lat;
        if (lat < 0)
          continue;
        int col_idx = edge_color_table[lat].second;
        opq = opts.edge_pattern[col_idx % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_vert_color(geom, pole[i]->idx, opts.clrngs[EDGES].get_col(col_idx),
                       opq);
      }
    }
  }
  else if (opts.edge_coloring_method == 'l') {
    for (auto i : edge_list) {
      int j = i->edge_no;
      if (i->lat < 0) {
        set_edge_color(geom, j, Color(), opts.opacity[EDGES]);
      }
      else {
        int lat = i->lat;
        opq = opts.edge_pattern[lat % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_edge_color(geom, j, opts.clrngs[EDGES].get_col(lat), opq);
      }
    }

    for (unsigned int i = 0; i < 2; i++) {
      if (pole[i]->idx > -1) {
        int lat = pole[i]->lat;
        opq = opts.edge_pattern[lat % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_vert_color(geom, pole[i]->idx, opts.clrngs[EDGES].get_col(lat),
                       opq);
      }
    }
  }
  else if (opts.edge_coloring_method == 'm') {
    for (auto i : edge_list) {
      int j = i->edge_no;
      if (i->lon < 0) {
        set_edge_color(geom, j, Color(), opts.opacity[EDGES]);
      }
      else {
        int lon = i->lon;
        opq = opts.edge_pattern[lon % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_edge_color(geom, j, opts.clrngs[EDGES].get_col(lon), opq);
      }
    }

    // poles don't have any longitude
    for (unsigned int i = 0; i < 2; i++) {
      if (pole[i]->idx > -1) {
        set_vert_color(geom, pole[i]->idx, opts.edge_default_color,
                       opts.opacity[EDGES]);
      }
    }
  }
  else if (opts.edge_coloring_method == 'b') {
    for (auto i : edge_list) {
      int j = i->edge_no;
      if (i->lat < 0)
        set_edge_color(geom, j, Color(), opts.opacity[EDGES]);
      else {
        int n = -1;
        if ((is_even(i->lat) && is_even(i->lon)) ||
            (!is_even(i->lat) && !is_even(i->lon)))
          n = 0;
        else
          n = 1;

        opq = opts.edge_pattern[n % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_edge_color(geom, j, opts.clrngs[EDGES].get_col(n), opq);
      }
    }

    // poles will be colored based North/South
    for (unsigned int i = 0; i < 2; i++) {
      if (pole[i]->idx > -1) {
        opq = opts.edge_pattern[i % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_vert_color(geom, pole[i]->idx, opts.clrngs[EDGES].get_col(i), opq);
      }
    }
  }
  else if (opts.edge_coloring_method == 'n') {
    // keep track of index beyond loop
    unsigned int k = 0;
    for (unsigned int i = 0; i < edge_list.size(); i++) {
      int j = edge_list[i]->edge_no;
      opq = opts.edge_pattern[i % opts.edge_pattern.size()] == '1'
                ? opts.opacity[EDGES]
                : 255;
      set_edge_color(geom, j, opts.clrngs[EDGES].get_col(i), opq);
      k++;
    }

    for (unsigned int i = 0; i < 2; i++) {
      int col_idx = k;
      if (pole[i]->idx > -1) {
        opq = opts.edge_pattern[k % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        set_vert_color(geom, pole[i]->idx, opts.clrngs[EDGES].get_col(col_idx),
                       opq);
      }
      k++;
    }
  }
  else if (strchr("xyz", opts.edge_coloring_method)) {
    for (auto i : edge_list) {
      int j = i->edge_no;
      double d = 0.0;
      for (int k : edges[j]) {
        if (opts.edge_coloring_method == 'x')
          d += verts[k][0];
        else if (opts.edge_coloring_method == 'y')
          d += verts[k][1];
        else if (opts.edge_coloring_method == 'z')
          d += verts[k][2];
      }

      // The opts.hybrid base portion will end up in +Z
      if (opts.hybrid && pc)
        d *= -1;

      Color c;
      int n = -1;
      if (d > 0.0)
        n = 0;
      else if (d < 0.0)
        n = 1;
      else {
        n = -1;
        opq = opts.opacity[EDGES];
        c = opts.edge_default_color;
      }

      if (n > -1) {
        opq = opts.edge_pattern[n % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        c = opts.clrngs[EDGES].get_col(n);
      }
      set_edge_color(geom, j, c, opq);
    }

    // poles are on the axis
    for (unsigned int i = 0; i < 2; i++) {
      if (pole[i]->idx > -1) {
        set_vert_color(geom, pole[i]->idx, opts.edge_default_color,
                       opts.opacity[EDGES]);
      }
    }
  }
  else if (opts.edge_coloring_method == 'o') {
    for (auto i : edge_list) {
      int j = i->edge_no;
      double dx = 0.0;
      double dy = 0.0;
      double dz = 0.0;
      for (int k : edges[j]) {
        dx += verts[k][0];
        dy += verts[k][1];
        dz += verts[k][2];
      }

      // The opts.hybrid base portion will end up in +Z
      if (opts.hybrid && pc)
        dz *= -1;

      Color c;
      int n = -1;
      // by octant number 1 to 8
      if ((dx > 0.0) && (dy > 0.0) && (dz > 0.0))
        n = 0;
      else if ((dx < 0.0) && (dy > 0.0) && (dz > 0.0))
        n = 1;
      else if ((dx < 0.0) && (dy < 0.0) && (dz > 0.0))
        n = 2;
      else if ((dx > 0.0) && (dy < 0.0) && (dz > 0.0))
        n = 3;
      else if ((dx > 0.0) && (dy > 0.0) && (dz < 0.0))
        n = 4;
      else if ((dx < 0.0) && (dy > 0.0) && (dz < 0.0))
        n = 5;
      else if ((dx < 0.0) && (dy < 0.0) && (dz < 0.0))
        n = 6;
      else if ((dx > 0.0) && (dy < 0.0) && (dz < 0.0))
        n = 7;
      else {
        n = -1;
        opq = opts.opacity[EDGES];
        c = opts.edge_default_color;
      }

      if (n > -1) {
        opq = opts.edge_pattern[n % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
        c = opts.clrngs[EDGES].get_col(n);
      }
      set_edge_color(geom, j, c, opq);
    }

    // poles are on the axis
    for (unsigned int i = 0; i < 2; i++) {
      if (pole[i]->idx > -1) {
        set_vert_color(geom, pole[i]->idx, opts.edge_default_color,
                       opts.opacity[EDGES]);
      }
    }
  }
}

void ncon_face_coloring(Geometry &geom, const vector<faceList *> &face_list,
                        map<int, pair<int, int>> &face_color_table,
                        const bool point_cut_calc, const ncon_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<Vec3d> &verts = geom.verts();

  // special situation
  bool pc = opts.point_cut;
  if (opts.build_method == 3 && opts.hybrid && opts.angle_is_side_cut) {
    if (pc != point_cut_calc)
      pc = point_cut_calc;
  }
  if (opts.radius_inversion)
    pc = !pc;

  int opq = 255;

  if (opts.face_coloring_method == 'q') {
    int circuit_count = 0;

    for (auto i : face_list) {
      int j = i->face_no;
      int lat = i->lat;

      if (i->lat < 0) {
        set_face_color(geom, j, Color(), opts.opacity[FACES]);
      }
      else {
        int col_idx = 0;
        if (i->rotate || (opts.hybrid && pc)) // front side
          col_idx = face_color_table[lat].second;
        else
          col_idx = face_color_table[lat].first;

        if (col_idx > circuit_count)
          circuit_count = col_idx;

        opq = opts.face_pattern[col_idx % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        set_face_color(geom, j, opts.clrngs[FACES].get_col(col_idx), opq);
      }
    }
  }
  else if (opts.face_coloring_method == 'l') {
    for (auto i : face_list) {
      int j = i->face_no;
      if (i->lat < 0) {
        set_face_color(geom, j, Color(), opts.opacity[FACES]);
      }
      else {
        int lat = i->lat;
        opq = opts.face_pattern[lat % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        set_face_color(geom, j, opts.clrngs[FACES].get_col(lat), opq);
      }
    }
  }
  else if (opts.face_coloring_method == 'm') {
    for (auto i : face_list) {
      int j = i->face_no;
      if (i->lon < 0) {
        set_face_color(geom, j, Color(), opts.opacity[FACES]);
      }
      else {
        int lon = i->lon;
        opq = opts.face_pattern[lon % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        set_face_color(geom, j, opts.clrngs[FACES].get_col(lon), opq);
      }
    }
  }
  else if (opts.face_coloring_method == 'b') {
    for (auto i : face_list) {
      int j = i->face_no;
      if (i->lat < 0)
        set_face_color(geom, j, Color(), opts.opacity[FACES]);
      else {
        int n = -1;
        if ((is_even(i->lat) && is_even(i->lon)) ||
            (!is_even(i->lat) && !is_even(i->lon)))
          n = 0;
        else
          n = 1;

        opq = opts.face_pattern[n % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        set_face_color(geom, j, opts.clrngs[FACES].get_col(n), opq);
      }
    }
  }
  else if (opts.face_coloring_method == 'n') {
    for (unsigned int i = 0; i < face_list.size(); i++) {
      int j = face_list[i]->face_no;
      opq = opts.face_pattern[i % opts.face_pattern.size()] == '1'
                ? opts.opacity[FACES]
                : 255;
      set_face_color(geom, j, opts.clrngs[FACES].get_col(i), opq);
    }
  }
  else if (strchr("xyz", opts.face_coloring_method)) {
    for (auto i : face_list) {
      int j = i->face_no;
      double d = 0.0;
      for (int k : faces[j]) {
        if (opts.face_coloring_method == 'x')
          d += verts[k][0];
        else if (opts.face_coloring_method == 'y')
          d += verts[k][1];
        else if (opts.face_coloring_method == 'z')
          d += verts[k][2];
      }

      // The opts.hybrid base portion will end up in +Z
      if (opts.hybrid && pc)
        d *= -1;

      Color c;
      int n = -1;
      if (d > 0.0)
        n = 0;
      else if (d < 0.0)
        n = 1;
      else {
        n = -1;
        opq = opts.opacity[FACES];
        c = opts.face_default_color;
      }

      if (n > -1) {
        opq = opts.face_pattern[n % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        c = opts.clrngs[FACES].get_col(n);
      }
      set_face_color(geom, j, c, opq);
    }
  }
  else if (opts.face_coloring_method == 'o') {
    for (auto i : face_list) {
      int j = i->face_no;
      double dx = 0.0;
      double dy = 0.0;
      double dz = 0.0;
      for (int k : faces[j]) {
        dx += verts[k][0];
        dy += verts[k][1];
        dz += verts[k][2];
      }

      // The opts.hybrid base portion will end up in +Z
      if (opts.hybrid && pc)
        dz *= -1;

      // by octant number 1 to 8
      Color c;
      int n = -1;
      if ((dx > 0.0) && (dy > 0.0) && (dz > 0.0))
        n = 0;
      else if ((dx < 0.0) && (dy > 0.0) && (dz > 0.0))
        n = 1;
      else if ((dx < 0.0) && (dy < 0.0) && (dz > 0.0))
        n = 2;
      else if ((dx > 0.0) && (dy < 0.0) && (dz > 0.0))
        n = 3;
      else if ((dx > 0.0) && (dy > 0.0) && (dz < 0.0))
        n = 4;
      else if ((dx < 0.0) && (dy > 0.0) && (dz < 0.0))
        n = 5;
      else if ((dx < 0.0) && (dy < 0.0) && (dz < 0.0))
        n = 6;
      else if ((dx > 0.0) && (dy < 0.0) && (dz < 0.0))
        n = 7;
      else {
        n = -1;
        opq = opts.opacity[FACES];
        c = opts.face_default_color;
      }

      if (n > -1) {
        opq = opts.face_pattern[n % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
        c = opts.clrngs[FACES].get_col(n);
      }
      set_face_color(geom, j, c, opq);
    }
  }
}

vector<int> find_adjacent_face_idx_in_channel(
    const Geometry &geom, const int face_idx,
    const vector<vector<int>> &bare_implicit_edges,
    map<vector<int>, vector<int>> &faces_by_edge, const bool prime)
{
  const vector<vector<int>> &faces = geom.faces();

  vector<int> face_idx_ret;
  vector<vector<int>> adjacent_edges;

  vector<int> face = faces[face_idx];
  unsigned int sz = face.size();
  for (unsigned int i = 0; i < sz; i++) {
    vector<int> edge(2);
    edge[0] = face[i];
    edge[1] = face[(i + 1) % sz];
    if (find_edge_in_edge_list(bare_implicit_edges, edge) > -1) {
      adjacent_edges.push_back(make_edge(edge[0], edge[1]));
    }
  }

  vector<int> adjacent_face_idx;
  for (auto &adjacent_edge : adjacent_edges) {
    vector<int> face_idx_tmp = faces_by_edge[adjacent_edge];
    for (int &j : face_idx_tmp) {
      if (j != face_idx)
        adjacent_face_idx.push_back(j);
    }
  }

  // the first time we "prime" so we return faces in both directions. the
  // second face becomes the "stranded" face. there may be faces which would
  // get pinched off (stranded), so there may be more than one
  for (int &i : adjacent_face_idx) {
    if (prime || !(geom.colors(FACES).get(i)).is_set()) {
      face_idx_ret.push_back(i);
    }
  }

  return face_idx_ret;
}

// flood_fill_count is changed
int set_face_colors_by_adjacent_face(
    Geometry &geom, const int start, const Color &c, const int opq,
    const int flood_fill_stop, int &flood_fill_count,
    const vector<vector<int>> &bare_implicit_edges,
    map<vector<int>, vector<int>> &faces_by_edge)
{
  if (flood_fill_stop && (flood_fill_count >= flood_fill_stop))
    return 0;

  vector<int> stranded_faces;

  vector<int> face_idx = find_adjacent_face_idx_in_channel(
      geom, start, bare_implicit_edges, faces_by_edge, true);
  while (face_idx.size()) {
    for (unsigned int i = 0; i < face_idx.size(); i++) {
      if (flood_fill_stop && (flood_fill_count >= flood_fill_stop))
        return 0;
      set_face_color(geom, face_idx[i], c, opq);
      flood_fill_count++;
      if (i > 0)
        stranded_faces.push_back(face_idx[i]);
    }
    face_idx = find_adjacent_face_idx_in_channel(
        geom, face_idx[0], bare_implicit_edges, faces_by_edge, false);
  }

  // check if stranded faces
  for (unsigned int i = 0; i < stranded_faces.size(); i++) {
    face_idx = find_adjacent_face_idx_in_channel(
        geom, stranded_faces[i], bare_implicit_edges, faces_by_edge, false);
    while (face_idx.size()) {
      for (unsigned int i = 0; i < face_idx.size(); i++) {
        if (flood_fill_stop && (flood_fill_count >= flood_fill_stop))
          return 0;
        set_face_color(geom, face_idx[i], c, opq);
        flood_fill_count++;
        if (i > 0)
          stranded_faces.push_back(face_idx[i]);
      }
      face_idx = find_adjacent_face_idx_in_channel(
          geom, face_idx[0], bare_implicit_edges, faces_by_edge, false);
    }
  }

  return (flood_fill_stop ? 1 : 0);
}

void fill_bare_implicit_edges(const Geometry &geom,
                              vector<vector<int>> &bare_implicit_edges)
{
  const vector<vector<int>> &edges = geom.edges();
  vector<vector<int>> implicit_edges;
  geom.get_impl_edges(implicit_edges);

  for (auto &implicit_edge : implicit_edges) {
    if (find_edge_in_edge_list(edges, implicit_edge) < 0)
      bare_implicit_edges.push_back(implicit_edge);
  }
}

void fill_faces_by_edge(const Geometry &geom,
                        map<vector<int>, vector<int>> &faces_by_edge)
{
  const vector<vector<int>> &faces = geom.faces();

  for (unsigned int i = 0; i < faces.size(); i++) {
    unsigned int sz = faces[i].size();
    for (unsigned int j = 0; j < sz; j++)
      faces_by_edge[make_edge(faces[i][j], faces[i][(j + 1) % sz])].push_back(
          i);
  }
}

int ncon_face_coloring_by_adjacent_face(Geometry &geom,
                                        const vector<faceList *> &face_list,
                                        const ncon_opts &opts)
{
  bool debug = false;

  int flood_fill_count = 0;
  int ret = 0;

  // all face colors need to be cleared
  Coloring clrng(&geom);
  clrng.f_one_col(Color());

  vector<vector<int>> bare_implicit_edges;
  fill_bare_implicit_edges(geom, bare_implicit_edges);

  map<vector<int>, vector<int>> faces_by_edge;
  fill_faces_by_edge(geom, faces_by_edge);

  int map_count = 0;
  int lat = 0;
  int lon = opts.longitudes.front() / 2 - 1;
  unsigned int sz = 0;
  do {
    bool painted = false;
    vector<int> idx = find_face_by_lat_lon(face_list, lat, lon);
    sz = idx.size();

    Color c = map_count;

    for (int j : idx) {
      int f_idx = face_list[j]->face_no;
      if ((geom.colors(FACES).get(f_idx)).is_set())
        continue;
      if (opts.flood_fill_stop && (flood_fill_count >= opts.flood_fill_stop))
        break;

      set_face_color(geom, f_idx, c, 255);
      flood_fill_count++;
      ret = set_face_colors_by_adjacent_face(
          geom, f_idx, c, 255, opts.flood_fill_stop, flood_fill_count,
          bare_implicit_edges, faces_by_edge);

      painted = true;
    }
    if (painted)
      map_count++;

    if (opts.flood_fill_stop && (flood_fill_count >= opts.flood_fill_stop))
      break;

    lat++;
  } while (sz);

  if (debug) {
    fprintf(stderr, "flood fill face color map:\n");
    lon = opts.longitudes.front() / 2;
    for (int l = 0; l < 2; l++) {
      lat = 0;
      lon -= l;
      do {
        vector<int> idx = find_face_by_lat_lon(face_list, lat, lon);
        sz = idx.size();
        if (sz) {
          int f_idx = face_list[idx[0]]->face_no;
          int k = geom.colors(FACES).get(f_idx).get_index();
          fprintf(stderr, "%d ", k);
        }
        lat++;
      } while (sz);
      fprintf(stderr, "\n");
    }
  }

  vector<int> color_table;
  for (int i = 0; i < map_count; i++)
    color_table.push_back(i);

  // colors for method 2 and 3 are reversed from method 1
  // accept when it is a 1/2 opts.twist
  // don't do this with flood fill stop as the colors change as fill gets
  // larger
  if (!opts.flood_fill_stop) {
    if (opts.posi_twist != 0 && (opts.ncon_order / opts.posi_twist == 2)) {
      reverse(color_table.begin(), color_table.end());
    }
  }

  // equivalent symmetric coloring. odd n_icons are not affected
  if (opts.symmetric_coloring && is_even(opts.ncon_order)) {
    unsigned int sz = color_table.size();
    unsigned int j = sz - 1;
    for (unsigned int i = 0; i < sz / 2; i++) {
      int idx = color_table[i];
      color_table[j] = idx;
      j--;
    }
  }

  // for (unsigned int i=0;i<color_table.size();i++)
  //   fprintf(stderr,"color table = %d\n",color_table[i]);

  // resolve map indexes
  for (unsigned int i = 0; i < geom.faces().size(); i++) {
    Color c_idx = geom.colors(FACES).get(i);
    if (c_idx.is_index()) {
      int idx = c_idx.get_index();
      idx = color_table[idx];
      Color c = opts.clrngs[FACES].get_col(idx);
      int opq = 255;
      if (opts.opacity[FACES] > -1)
        opq = opts.face_pattern[idx % opts.face_pattern.size()] == '1'
                  ? opts.opacity[FACES]
                  : 255;
      set_face_color(geom, i, c, opq);
    }
  }

  return ret;
}

void ncon_edge_coloring_by_adjacent_edge(Geometry &geom,
                                         const vector<edgeList *> &edge_list,
                                         const vector<poleList *> &pole,
                                         const ncon_opts &opts)
{
  bool debug = false;

  bool pc = opts.point_cut;
  if (opts.radius_inversion)
    pc = !pc;

  vector<vector<int>> edges;
  vector<int> edge_no;
  if (!opts.hybrid) {
    for (auto i : edge_list) {
      edges.push_back(geom.edges(i->edge_no));
      edge_no.push_back(i->edge_no);
    }
  }
  else {
    // using edge_list doesn't work for hybrids
    for (unsigned int i = 0; i < geom.edges().size(); i++) {
      Color c = geom.colors(EDGES).get(i);
      // need to include invisible edges or crash
      if (c.is_maximum_index() || c.is_invisible()) {
        edges.push_back(geom.edges(i));
        edge_no.push_back(i);
      }
    }
  }

  // unset colors
  for (unsigned int i = 0; i < geom.edges().size(); i++) {
    Color c = geom.colors(EDGES).get(i);
    if (c.is_maximum_index())
      set_edge_color(geom, i, Color(), 255);
  }

  int map_count = 0;
  int circuit_count = 0;

  // if there is a north pole increment the color map
  // not a circuit, so don't increment circuit count
  // if (is_even(opts.ncon_order) && pc && opts.posi_twist == 0 &&
  // !opts.double_sweep)
  if (pole[0]->idx != -1 && !opts.double_sweep && !opts.hybrid)
    map_count++;

  // edge lats start at 1
  int lat = 1;
  int lon = opts.longitudes.front() / 2 - 1;
  unsigned int sz = 0;
  do {
    bool painted = false;
    vector<int> idx = find_edge_by_lat_lon(edge_list, lat, lon);
    sz = idx.size();

    Color c = map_count;

    for (unsigned int j = 0; j < sz; j++) {
      int e_idx = edge_list[idx[j]]->edge_no;
      vector<int> edge = edges[e_idx];
      vector<int> es = find_edges_with_vertex(edges, edge[0]);
      vector<int> es_tmp = find_edges_with_vertex(edges, edge[1]);
      es.insert(es.end(), es_tmp.begin(), es_tmp.end());

      for (int k = (int)es.size() - 1; k >= 0; k--) {
        if ((geom.colors(EDGES).get(edge_no[es[k]])).is_set())
          es.erase(es.begin() + k);
      }

      while (es.size()) {
        // continue to color in one direction, then the other
        for (int e : es) {
          set_edge_color(geom, edge_no[e], c, 255);
          painted = true;
        }

        vector<int> es_next;
        for (int e : es) {
          edge = edges[e];
          if (!es_next.size())
            es_next = find_edges_with_vertex(edges, edge[0]);
          else {
            es_tmp = find_edges_with_vertex(edges, edge[0]);
            es_next.insert(es_next.end(), es_tmp.begin(), es_tmp.end());
          }

          es_tmp = find_edges_with_vertex(edges, edge[1]);
          es_next.insert(es_next.end(), es_tmp.begin(), es_tmp.end());
        }

        es = es_next;
        for (int k = (int)es.size() - 1; k >= 0; k--) {
          if ((geom.colors(EDGES).get(edge_no[es[k]])).is_set())
            es.erase(es.begin() + k);
        }
      }
    }

    if (painted) {
      map_count++;
      circuit_count++;
    }

    lat++;
  } while (sz);

  // north pole, even opts.point_cut only
  // if there is a pole in build_method 3 it is meant to be there
  int n = ((opts.build_method == 2) && opts.n_doubled) ? opts.ncon_order / 2
                                                       : opts.ncon_order;
  if ((pole[0]->idx != -1) && ((is_even(n) && pc) || opts.build_method == 3) &&
      !opts.hybrid) {
    int opq = opts.edge_pattern[0 % (int)opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
    Color c = opts.clrngs[EDGES].get_col(0);
    set_vert_color(geom, pole[0]->idx, c, opq);
  }

  // south pole
  if ((pole[1]->idx != -1) && (!is_even(n) || (is_even(n) && pc)) &&
      !opts.hybrid) {
    int l = (opts.symmetric_coloring && is_even(opts.ncon_order)) ? 0 : lat - 1;
    int opq = opts.edge_pattern[l % (int)opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
    Color c = opts.clrngs[EDGES].get_col(l);
    set_vert_color(geom, pole[1]->idx, c, opq);
  }

  // some models will have a stranded edge
  bool painted = false;
  for (auto &edge : edges) {
    int edge_no = find_edge_in_edge_list(geom.edges(), edge);
    if (!(geom.colors(EDGES).get(edge_no)).is_set()) {
      set_edge_color(geom, edge_no, Color(0), 255);
      painted = true;
    }
  }
  if (painted) {
    map_count++;
    circuit_count++;
  }

  vector<int> color_table;
  for (int i = 1; i < map_count; i++)
    color_table.push_back(i);

  // equivilent to z=1
  if (is_even(opts.ncon_order) && pc && !opts.hybrid && !opts.double_sweep &&
      ((opts.posi_twist != 0) && (opts.ncon_order % opts.posi_twist == 0)))
    color_table[color_table.size() - 1] = 0;

  // equivalent symmetric coloring. odd n_icons are not affected
  if (opts.symmetric_coloring && is_even(opts.ncon_order)) {
    sz = color_table.size();
    unsigned int j = sz - 1;
    for (unsigned int i = 0; i < sz / 2; i++) {
      int idx = color_table[i];
      color_table[j] = idx;
      j--;
    }
  }

  // for alignment to color map
  color_table.insert(color_table.begin(), 0);

  // colors for method 2 and 3 are reversed from method 1
  // accept when it is a 1/2 opts.twist
  // don't do this with flood fill stop as the colors change as fill gets
  // larger
  if (!opts.flood_fill_stop) {
    if (opts.posi_twist != 0 && (opts.ncon_order / opts.posi_twist == 2)) {
      reverse(color_table.begin(), color_table.end());
    }
  }

  // note: edge_list is a problem for hybrids
  if (debug) {
    fprintf(stderr, "flood fill edge color map:\n");
    lon = opts.longitudes.front() / 2;
    for (int l = 0; l < 2; l++) {
      lon -= l;
      // edge lats start at 1
      lat = 1;
      do {
        vector<int> idx = find_edge_by_lat_lon(edge_list, lat, lon);
        sz = idx.size();
        if (sz) {
          int e_idx = edge_list[idx[0]]->edge_no;
          int k = geom.colors(EDGES).get(e_idx).get_index();
          fprintf(stderr, "%d ", k);
        }
        lat++;
      } while (sz);
      fprintf(stderr, "\n");
    }
  }

  // resolve map indexes
  for (unsigned int i = 0; i < edges.size(); i++) {
    Color c_idx = geom.colors(EDGES).get(edge_no[i]);
    if (c_idx.is_index()) {
      int idx = c_idx.get_index();
      idx = color_table[idx];
      Color c = opts.clrngs[EDGES].get_col(idx);
      int opq = 255;
      if (opts.opacity[EDGES] > -1)
        opq = opts.edge_pattern[idx % opts.edge_pattern.size()] == '1'
                  ? opts.opacity[EDGES]
                  : 255;
      set_edge_color(geom, edge_no[i], c, opq);
    }
  }
}

int ncon_face_coloring_by_compound_flood(Geometry &geom,
                                         const vector<faceList *> &face_list,
                                         const vector<int> &caps,
                                         const ncon_opts &opts)
{
  int flood_fill_count = 0;
  int ret = 0;
  int opq = 255;

  // all face indexes need to be cleared
  Coloring clrng(&geom);
  clrng.f_one_col(Color());

  vector<pair<int, int>> polygon_table;

  int sz = 0;
  int lat = 0;
  int lon = opts.longitudes.front() / 2;
  if (!opts.hybrid)
    lon--;
  do {
    vector<int> idx = find_face_by_lat_lon(face_list, lat, lon);
    sz = idx.size();
    for (int j = 0; j < sz; j++) {
      int polygon_no = face_list[idx[j]]->polygon_no;
      polygon_table.push_back(make_pair(polygon_no, idx[j]));
    }
    lat++;
  } while (sz);

  sort(polygon_table.begin(), polygon_table.end());
  auto li = unique(polygon_table.begin(), polygon_table.end());
  polygon_table.erase(li, polygon_table.end());

  vector<vector<int>> bare_implicit_edges;
  fill_bare_implicit_edges(geom, bare_implicit_edges);

  map<vector<int>, vector<int>> faces_by_edge;
  fill_faces_by_edge(geom, faces_by_edge);

  int map_count = 0;
  int polygon_no_last = -1;
  for (auto polygons : polygon_table) {
    int polygon_no = polygons.first;
    int face_no = polygons.second;

    if ((geom.colors(FACES).get(face_no)).is_set())
      continue;
    if (opts.flood_fill_stop && (flood_fill_count >= opts.flood_fill_stop))
      return 0;

    Color c = opts.clrngs[FACES].get_col(polygon_no);
    if (opts.opacity[FACES] > -1)
      opq = opts.face_pattern[polygon_no % opts.face_pattern.size()] == '1'
                ? opts.opacity[FACES]
                : 255;
    set_face_color(geom, face_no, c, opq);

    flood_fill_count++;
    ret = set_face_colors_by_adjacent_face(
        geom, face_no, c, opq, opts.flood_fill_stop, flood_fill_count,
        bare_implicit_edges, faces_by_edge);

    if (polygon_no != polygon_no_last)
      map_count++;
    polygon_no_last = polygon_no;
  }

  // if opts.build_method 1 or 2, twist 0, then end caps will not be colored
  // color them the first map color
  if (opts.build_method < 3 && (opts.posi_twist == 0)) {
    for (int cap : caps)
      geom.colors(FACES).set(face_list[cap]->face_no,
                             opts.clrngs[FACES].get_col(0));
  }

  return ret;
}

/* unused function to line a face with edges of color col
void add_edges_to_face(Geometry &geom, vector<int> face, Color col)
{
   unsigned int sz = face.size();
   for (unsigned int i=0;i<sz;i++)
      geom.add_col_edge(make_edge(face[i],face[(i+1)%sz]),col);
}
*/

// average edge colors with simple RGB
Color ncon_average_edge_color(const vector<Color> &cols)
{
  /* average RGB colors */
  Vec4d col(0.0, 0.0, 0.0, 0.0);

  int j = 0;
  for (auto i : cols) {
    if (i.is_set() && i.is_visible_value()) {
      col += i.get_vec4d();
      j++;
    }
  }

  if (j)
    col /= j;
  return col;
}

// find poles by Y value. Set north and south if found
void find_poles(Geometry &geom, int &north, int &south, const ncon_opts &opts)
{
  // find poles
  north = -1;
  double y_north = std::numeric_limits<double>::min();
  south = -1;
  double y_south = std::numeric_limits<double>::max();
  for (unsigned int i = 0; i < geom.verts().size(); i++) {
    if (double_gt(geom.verts(i)[1], y_north, opts.eps)) {
      y_north = geom.verts(i)[1];
      north = i;
    }
    if (double_lt(geom.verts(i)[1], y_south, opts.eps)) {
      y_south = geom.verts(i)[1];
      south = i;
    }
  }
}

void ncon_edge_coloring_from_faces(Geometry &geom, const ncon_opts &opts)
{
  // save invisible edges
  vector<int> inv_edges;
  for (unsigned int i = 0; i < geom.edges().size(); i++) {
    if ((geom.colors(EDGES).get(i)).is_invisible())
      inv_edges.push_back(i);
  }

  // save invisible verts
  vector<int> inv_verts;
  for (unsigned int i = 0; i < geom.verts().size(); i++) {
    if ((geom.colors(VERTS).get(i)).is_invisible())
      inv_verts.push_back(i);
  }

  Coloring clrng(&geom);
  clrng.add_cmap(opts.clrngs[FACES].clone());
  clrng.e_from_adjacent(FACES);
  clrng.v_from_adjacent(FACES);

  // restore invisible edges
  for (int inv_edge : inv_edges)
    geom.colors(EDGES).set(inv_edge, Color::invisible);

  // restore invisible verts
  for (int inv_vert : inv_verts)
    geom.colors(VERTS).set(inv_vert, Color::invisible);
}

// mark edge circuits for methods 2 and 3 color by adjacent edge
// also for method 1, color by symmetry
void mark_edge_circuits(Geometry &geom, const vector<edgeList *> &edge_list)
{
  for (auto i : edge_list) {
    int j = i->edge_no;
    if (!geom.colors(EDGES).get(j).is_invisible())
      set_edge_color(geom, j, Color::maximum_index, 255);
  }
}

// for method 2, if indented edges are not shown, overwrite them as invisible
void set_indented_edges_invisible(Geometry &geom,
                                  const vector<edgeList *> &edge_list,
                                  const vector<poleList *> &pole)
{
  for (auto i : edge_list) {
    int j = i->edge_no;
    int lat = i->lat;
    if (lat == -1)
      set_edge_color(geom, j, Color::invisible, 255);
  }

  for (unsigned int i = 0; i < 2; i++) {
    if (pole[i]->idx > -1) {
      int lat = pole[i]->lat;
      if (lat == -1)
        set_vert_color(geom, pole[i]->idx, Color::invisible, 255);
    }
  }
}

void make_sequential_map(map<int, pair<int, int>> &color_table)
{
  unsigned int sz = color_table.size();

  // add two to avoid zeroes and make all elements negative.
  for (unsigned int i = 0; i < sz; i++) {
    color_table[i].first = -(color_table[i].first + 2);
    color_table[i].second = -(color_table[i].second + 2);
  }

  int k = 0;
  for (unsigned int i = 0; i < sz; i++) {
    int element = color_table[i].first;
    if (element < 0) {
      for (unsigned int j = i; j < sz; j++) {
        if (color_table[j].first == element)
          color_table[j].first = k;
      }
      for (unsigned int j = 0; j < sz; j++) {
        if (color_table[j].second == element)
          color_table[j].second = k;
      }
      k++;
    }
  }
}

// coding idea for circuit coloring furnished by Adrian Rossiter
void build_circuit_table(const int ncon_order, const int twist,
                         const bool hybrid, const bool symmetric_coloring,
                         map<int, int> &circuit_table)
{
  // use a double size polygon
  int n = 2 * ncon_order;
  int t = 2 * twist;

  if (hybrid)
    t--;

  int t_mult = symmetric_coloring ? 1 : 2;

  int d = gcd(n, t_mult * t);

  for (int i = 0; i <= d / 2; i++) {
    for (int j = i; j < n; j += d)
      circuit_table[j] = i;
    for (int j = d - i; j < n; j += d)
      circuit_table[j] = i;
  }
}

void build_color_table(map<int, pair<int, int>> &color_table,
                       const int ncon_order, const int twist, const bool hybrid,
                       const bool symmetric_coloring, const int increment)
{
  bool debug = false;

  // boolean for testing. this was face coloring options 't'
  bool sequential_colors = true;

  // for the color tables twist is made positive. The positive twist colors
  // work for negative twists.
  unsigned int n = ncon_order;
  unsigned int t = std::abs(twist);

  // for even n, twist 0, symmetric coloring does't happen, so temporarily
  // twist half way
  if (symmetric_coloring && is_even(n) && t == 0) {
    int mod_twist = t % n;
    if (!mod_twist)
      t = n / 2;
  }

  map<int, int> circuit_table;
  build_circuit_table(n, t, hybrid, symmetric_coloring, circuit_table);

  // for (unsigned int i=0;i<circuit_table.size();i++)
  //   fprintf(stderr,"circuit_table[%d] = %d\n",i,circuit_table[i]);

  if (debug)
    fprintf(stderr, "color table:\n");
  for (unsigned int i = 0; i < n; i++) {
    int position = 2 * i + increment;
    // when increment = -1, pos can equal -1, use last position
    if (position < 0)
      position = 2 * n + position;
    unsigned int circuit_idx = position % (int)circuit_table.size();
    color_table[i].first = circuit_table[circuit_idx];
    if (debug)
      fprintf(stderr, "%d ", color_table[i].first);
  }
  if (debug)
    fprintf(stderr, "\n");

  // first is back half.
  // second is front half.
  for (unsigned int i = 0; i < n; i++) {
    color_table[i].second = color_table[(t + i) % n].first;
    if (debug)
      fprintf(stderr, "%d ", color_table[i].second);
  }
  if (debug)
    fprintf(stderr, "\n");

  // circuit table does not start from 1 and is not sequential
  if (sequential_colors)
    make_sequential_map(color_table);
  if (debug) {
    fprintf(stderr, "sequential color table:\n");
    for (unsigned int i = 0; i < n; i++)
      fprintf(stderr, "%d ", color_table[i].first);
    fprintf(stderr, "\n");
    for (unsigned int i = 0; i < n; i++)
      fprintf(stderr, "%d ", color_table[i].second);
    fprintf(stderr, "\n");
  }
}

// point_cut is not that of opts
void ncon_coloring(Geometry &geom, const vector<faceList *> &face_list,
                   const vector<edgeList *> &edge_list,
                   const vector<poleList *> &pole, const bool point_cut_calc,
                   const int lat_mode, const ncon_opts &opts)
{
  map<int, pair<int, int>> edge_color_table;
  map<int, pair<int, int>> face_color_table;

  // for build_color_tables with build_method 2, else use what is passed
  int t = ((opts.build_method == 2) && opts.n_doubled) ? opts.twist / 2
                                                       : opts.twist;
  int n = ((opts.build_method == 2) && opts.n_doubled) ? opts.ncon_order / 2
                                                       : opts.ncon_order;

  bool hyb = opts.hybrid;

  // works with old method of assigning latitudes with angles in method 3,
  // faces are actually not a hybrid but a normal point cut of n*2 with a
  // twist of t*2-1
  if (lat_mode == 2 && opts.double_sweep) {
    n *= 2;
    t *= 2;
    if (opts.hybrid) {
      hyb = false;
      t--;
    }
  }

  // fprintf(stderr,"opts.point_cut = %s\n",opts.point_cut ? "point" :
  // "side"); fprintf(stderr,"point_cut_calc = %s (used for
  // face_increment)\n",point_cut_calc ? "point" : "side");

  bool pc = (!opts.hide_indent || opts.double_sweep || opts.angle_is_side_cut)
                ? point_cut_calc
                : opts.point_cut;

  bool hi = opts.hide_indent;
  if (opts.build_method == 2 && opts.d_of_one)
    hi = false;

  if (opts.build_method == 2 && hi) {
    if (opts.radius_inversion)
      pc = !pc;
  }
  // fprintf(stderr,"pc = %s\n",pc ? "point" : "side");

  // increment rules for d = 1
  int face_increment = ((is_even(n) && pc) && !opts.hybrid) ? 1 : 0;
  int edge_increment = face_increment - 1;

  // faces work with old method of apply_latitudes when d=1
  if (lat_mode == 2 && opts.double_sweep)
    face_increment = 1;

  // fprintf(stderr,"face_increment = %d\n", face_increment);
  // fprintf(stderr,"edge_increment = %d\n", edge_increment);

  // fprintf(stderr,"face color table: n = %d t = %d face_increment =
  // %d\n",n,t,face_increment);
  build_color_table(face_color_table, n, t, hyb, opts.symmetric_coloring,
                    face_increment);

  // when not hiding indent of method 2, edges of hybrid
  // are actually not a hybrid but a normal point cut of n*2 with a twist of
  // t*2-1
  if (((opts.build_method == 2) && opts.n_doubled) && !hi) {
    n *= 2;
    t *= 2;
    if (opts.hybrid) {
      hyb = false;
      t--;
      // edge increment for point cut
      edge_increment = 0;
    }
  }

  // fprintf(stderr,"edge color table: n = %d t = %d edge_increment =
  // %d\n",n,t,edge_increment);
  build_color_table(edge_color_table, n, t, hyb, opts.symmetric_coloring,
                    edge_increment);

  // front and back twisting is opposite, fix when twisted half way
  if ((opts.posi_twist != 0) &&
      double_eq((double)opts.ncon_order / opts.posi_twist, 2.0, opts.eps)) {
    for (auto &i : edge_color_table)
      swap(i.second.first, i.second.second);
    for (auto &i : face_color_table)
      swap(i.second.first, i.second.second);
  }

  // point_cut_calc needed below if build_method 3 angle causes side cut for
  // hybrid coloring

  // don't do this when circuits use flood fill in methods 2 or 3 as they will
  // be colored later
  if (!(opts.build_method > 1 && strchr("fksc", opts.face_coloring_method)))
    ncon_face_coloring(geom, face_list, face_color_table, point_cut_calc, opts);

  // don't do this when circuits use flood fill in methods 2 or 3 as they will
  // be colored later
  if ((opts.edge_coloring_method != 'Q') &&
      !(opts.build_method > 1 && strchr("fs", opts.edge_coloring_method)))
    ncon_edge_coloring(geom, edge_list, pole, edge_color_table, point_cut_calc,
                       opts);

  // hide indented edges of method 2
  if ((opts.build_method == 2) && opts.n_doubled && hi)
    set_indented_edges_invisible(geom, edge_list, pole);

  // mark edge circuits for methods 2 and 3 flood fill, or color by symmetry
  if ((opts.build_method > 1 && strchr("fs", opts.edge_coloring_method)) ||
      (opts.build_method == 1 && strchr("s", opts.edge_coloring_method)))
    mark_edge_circuits(geom, edge_list);
}

// inner_radius and outer_radius is calculated within
// double sweep is set in build_globe()
// radius_inversion is set
void build_globe(Geometry &geom, vector<coordList *> &coordinates,
                 vector<faceList *> &face_list, vector<edgeList *> &edge_list,
                 vector<poleList *> &pole, vector<int> &caps,
                 double &inner_radius, double &outer_radius,
                 bool &radius_inversion, bool &double_sweep,
                 const bool second_half, const ncon_opts &opts)
{
  // point cut is changed so save a copy
  bool point_cut_calc = opts.point_cut;

  // in methods 2 and 3, most faces are split in two
  vector<vector<int>> split_face_indexes;

  // in method 3, original faces as though they were not split
  vector<vector<int>> original_faces;

  vector<int> prime_meridian;
  if (opts.build_method == 3) {
    build_prime_polygon(geom, prime_meridian, coordinates, pole, opts);

    // if either pole was defined it is a point cut
    point_cut_calc = false;
    for (auto &i : pole)
      if (i->idx != -1)
        point_cut_calc = true;

    // forms with a vertex on Y axis only need 1/2 pass
    // forms with edge parallel with the Y axis only need 1/2 pass
    int polygons_total =
        (point_cut_calc ||
         angle_on_aligned_polygon(opts.angle, opts.ncon_order, opts.eps))
            ? opts.longitudes.front() / 2
            : opts.longitudes.front();
    double_sweep = (polygons_total == opts.longitudes.front()) ? true : false;

    // maximum latitudes is set
    form_angular_model(geom, prime_meridian, coordinates, face_list, edge_list,
                       pole, original_faces, split_face_indexes, polygons_total,
                       opts);
  }
  else {
    // inner_radius and outer_radius is calculated within
    // point_cut_calc changed from side cut to point cut
    // if build method 2 and d > 1
    build_prime_meridian(geom, prime_meridian, coordinates, inner_radius,
                         outer_radius, point_cut_calc, opts);

    if (opts.build_method == 2)
      radius_inversion =
          double_gt(fabs(opts.inner_radius), fabs(opts.outer_radius), opts.eps);

    // need original point cut for polygon numbers
    form_globe(geom, prime_meridian, coordinates, face_list, edge_list,
               point_cut_calc, second_half, opts);

    add_caps(geom, coordinates, face_list, pole, caps, point_cut_calc, opts);
    if ((opts.build_method == 2) && opts.n_doubled)
      // note that function used to reverse indented based on manual inner and
      // outer radii
      mark_indented_edges_invisible(edge_list, pole, radius_inversion, opts);
    find_split_faces_shell_model(geom, face_list, edge_list, pole,
                                 split_face_indexes, opts);
  }

  // apply latitude numbers at the end
  int lat_mode = 1;

  // old apply_latitudes was specifically for method 3
  if (opts.build_method == 3) {
    // the old apply_latitudes works for double_sweep so completes working
    // with d=1
    if (double_sweep)
      lat_mode = 2;
    // the old apply_latitudes must be used for method 3 compound coloring,
    // new method won't work. 'c' needed to fix fix_polygon_numbers
    if (strchr("ck", opts.face_coloring_method))
      lat_mode = 2;
    // non-co-prime compounds don't work with new method. Use old method
    // (faster)
    if (gcd(opts.ncon_order, opts.d) != 1)
      lat_mode = 2;
  }

  // if flood fill is going to be used, use old method (faster)
  // but build_method can be 2
  if (opts.face_coloring_method == 'f')
    lat_mode = 2;

  bool do_faces = strchr("lbqfck", opts.face_coloring_method);
  bool do_edges = strchr("lbqf", opts.edge_coloring_method);

  // latitudes are accessed in methods l,b,q,f,c,k
  // if not these then there is no need to set
  if (!do_faces && !do_edges)
    lat_mode = 0;

  if (lat_mode != 1)
    if (opts.build_method == 2)
      restore_indented_edges(edge_list, opts);

  if (lat_mode == 1) {
    // new 'sequential' apply_latitudes compatible with both methods 2 and 3
    apply_latitudes(geom, split_face_indexes, face_list, edge_list, pole, opts);
  }
  else if (lat_mode == 2) {
    if (opts.build_method == 3) {
      apply_latitudes(geom, original_faces, split_face_indexes, face_list,
                      edge_list, pole, opts);
      // old apply_latitudes needs to fix polygon numbers for compound
      // coloring
      if (strchr("ck", opts.face_coloring_method) && !double_sweep)
        fix_polygon_numbers(face_list, opts);
    }
  }

  // sending opts.point_cut
  ncon_coloring(geom, face_list, edge_list, pole, point_cut_calc, lat_mode,
                opts);
}

double hybrid_twist_angle(const ncon_opts &opts)
{
  int n = opts.ncon_order;
  int t = opts.twist;

  if ((opts.build_method == 2) && opts.n_doubled) {
    n /= 2;
    t /= 2;
  }

  // there is no twist 0 so positive twists have to be adjusted
  int twist = t;
  if (twist > 0)
    twist -= 1;

  // twist 1, 2, 3 is really twist 0.5, 1.5, 2.5 so add half twist
  double half_twist = 360.0 / (n * 2);
  double angle = half_twist + half_twist * twist * 2;

  return angle;
}

// if partial model, delete appropriate elements
void delete_unused_longitudes(Geometry &geom, vector<faceList *> &face_list,
                              vector<edgeList *> &edge_list,
                              const vector<int> &caps, const bool opposite,
                              const bool del_override, const ncon_opts &opts)
{
  if (full_model(opts.longitudes) &&
      (opts.longitudes_save == opts.longitudes.back()))
    return;

  int longitudes_back =
      (opts.hybrid) ? opts.longitudes.back() : opts.longitudes_save;
  bool longitudes_inv = opts.lon_invisible;
  if (longitudes_inv && del_override)
    longitudes_inv = false;

  // don't allow caps to be deleted
  if (opts.build_method == 2) {
    for (int cap : caps) {
      int lon = face_list[cap]->lon;
      if (lon < opts.longitudes.front() / 2)
        face_list[cap]->lon = -1;
    }
  }

  vector<int> delete_list;
  vector<int> delete_elem;
  for (unsigned int i = 0; i < face_list.size(); i++) {
    bool val = (face_list[i]->lon >= longitudes_back);
    if ((!opposite && val) || (opposite && !val)) {
      delete_list.push_back(i);
      // face_no doesn't work for method 3
      // int j = face_list[i]->face_no;
      delete_elem.push_back(i);
    }
  }

  if (delete_list.size()) {
    if (!longitudes_inv) {
      delete_face_list_items(face_list, delete_list);
      geom.del(FACES, delete_elem);
    }
    else {
      for (unsigned int i = 0; i < delete_elem.size(); i++) {
        geom.colors(FACES).set(delete_elem[i], Color::invisible);
      }
    }
  }

  delete_list.clear();
  delete_elem.clear();
  for (unsigned int i = 0; i < edge_list.size(); i++) {
    bool val = (edge_list[i]->lon >= longitudes_back);
    if ((!opposite && val) || (opposite && !val)) {
      delete_list.push_back(i);
      // edge_no doesn't work for method 3
      // int j = edge_list[i]->edge_no;
      delete_elem.push_back(i);
    }
  }

  if (delete_list.size()) {
    if (!longitudes_inv) {
      delete_edge_list_items(edge_list, delete_list);
      geom.del(EDGES, delete_elem);
    }
    else {
      for (unsigned int i = 0; i < delete_elem.size(); i++) {
        set_edge_color(geom, delete_elem[i], Color::invisible, 255);
      }
    }
  }

  geom.del(VERTS, geom.get_info().get_free_verts());

  if ((opts.build_method == 2 &&
       (!opts.point_cut || !is_even(opts.ncon_order))) ||
      (opts.build_method == 1 && opts.hybrid)) {
    vector<int> single_verts;
    for (unsigned int i = 0; i < geom.verts().size(); i++) {
      vector<int> faces_with_index = find_faces_with_vertex(geom.faces(), i);
      if (faces_with_index.size() == 1)
        single_verts.push_back(i);
    }
    geom.del(VERTS, single_verts);
  }
}

void add_triangles_to_close(Geometry &geom, vector<int> &added_triangles,
                            const ncon_opts &opts)
{
  vector<vector<int>> unmatched_edges = find_unmatched_edges(geom);

  Geometry unmatched = geom;
  unmatched.clear(FACES);
  unmatched.clear(EDGES);
  unmatched.raw_edges() = unmatched_edges;

  double outer_rad = unmatched.verts(0).len();

  vector<int> inner_v;
  for (unsigned int i = 0; i < unmatched_edges.size(); i++) {
    for (unsigned int j = 0; j < 2; j++) {
      int v = unmatched_edges[i][j];
      if (double_ne(unmatched.verts(v).len(), outer_rad, opts.eps))
        inner_v.push_back(v);
    }
  }

  sort(inner_v.begin(), inner_v.end());
  auto iv = unique(inner_v.begin(), inner_v.end());
  inner_v.erase(iv, inner_v.end());

  for (unsigned int i = 0; i < inner_v.size(); i++) {
    vector<int> edge_idx =
        find_edges_with_vertex(unmatched.edges(), inner_v[i]);
    if (!is_even((int)edge_idx.size()))
      opts.warning(
          "add_triangles_to_close: unmatched edges group unexpectedly odd");
    vector<int> outer_v;
    for (unsigned int j = 0; j < edge_idx.size(); j++) {
      unsigned int e = edge_idx[j];
      outer_v.push_back((unmatched.edges(e)[0] == inner_v[i])
                            ? unmatched.edges(e)[1]
                            : unmatched.edges(e)[0]);
    }
    for (unsigned int j = 0; j < outer_v.size() - 1; j++) {
      for (unsigned int k = j; k < outer_v.size(); k++) {
        if (outer_v[j] == outer_v[k])
          continue;
        if (triangle_zero_area(geom, inner_v[i], outer_v[j], outer_v[k],
                               opts.eps)) {
          vector<int> face;
          face.push_back(inner_v[i]);
          face.push_back(outer_v[j]);
          face.push_back(outer_v[k]);
          geom.add_face(face, Color::invisible);
          added_triangles.push_back(geom.faces().size() - 1);
        }
      }
    }
  }
}

// hybrids lose opts.longitudes.front()/2-1 from the edge_list
// be able to back it up and restore it using color elements that are carried
// along with them
void backup_flood_longitude_edges(Geometry &geom, vector<edgeList *> &edge_list,
                                  const ncon_opts &opts)
{
  int lat = 1;
  int lon = opts.longitudes.front() / 2 - 1;
  unsigned int sz = 0;
  do {
    vector<int> idx = find_edge_by_lat_lon(edge_list, lat, lon);
    sz = idx.size();
    for (unsigned int j = 0; j < sz; j++)
      geom.colors(EDGES).set(idx[j], Color(lat));
    lat++;
  } while (sz);
}

// after restore, change color index to INT_MAX for flood fill procedure
void restore_flood_longitude_edges(Geometry &geom,
                                   vector<edgeList *> &edge_list,
                                   const ncon_opts &opts)
{
  const vector<vector<int>> &edges = geom.edges();

  for (unsigned int i = 0; i < edges.size(); i++) {
    Color c = geom.colors(EDGES).get(i);
    if (!c.is_maximum_index() && !c.is_invisible()) {
      edge_list.push_back(
          new edgeList(i, c.get_index(), opts.longitudes.front() / 2 - 1));
      geom.colors(EDGES).set(i, Color::maximum_index);
    }
  }
}

void backup_flood_longitude_faces(Geometry &geom, vector<faceList *> &face_list,
                                  const ncon_opts &opts)
{
  int lat = 0;
  int lon = opts.longitudes.front() / 2 - 1;
  unsigned int sz = 0;
  do {
    vector<int> idx = find_face_by_lat_lon(face_list, lat, lon);
    sz = idx.size();
    for (unsigned int j = 0; j < sz; j++)
      geom.colors(FACES).set(idx[j], Color(lat));
    lat++;
  } while (sz);
}

// after restore, unset face color
void restore_flood_longitude_faces(Geometry &geom,
                                   vector<faceList *> &face_list,
                                   const ncon_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();

  for (unsigned int i = 0; i < faces.size(); i++) {
    if (geom.colors(FACES).get(i).is_index()) {
      int j = geom.colors(FACES).get(i).get_index();
      face_list.push_back(
          new faceList(i, j, opts.longitudes.front() / 2 - 1, 0));
      geom.colors(FACES).set(i, Color());
    }
  }
}

void build_compound_polygon(vector<Geometry> &polar_polygons,
                            const ncon_opts &opts)
{
  // duplicate polygon for non-hybrids
  if (polar_polygons.size() == 1)
    polar_polygons.push_back(polar_polygons[0]);

  polar_polygons[1].transform(
      Trans3d::rotate(0, 0, deg2rad(-opts.twist_angle)));

  unsigned int num_polygons = gcd(opts.ncon_order, opts.d);

  // speed up. if number of polygons is 1 they are all one color
  if (num_polygons == 1) {
    for (unsigned int i = 0; i < 2; i++) {
      Coloring(&(polar_polygons[i])).e_one_col(opts.clrngs[FACES].get_col(0));
    }
    return;
  }

  vector<pair<int, int>> edge_queue;
  map<int, vector<pair<int, int>>> edge_colors;
  int edge_map_color = 0;

  // maximum colors
  for (unsigned int i = 0; i < num_polygons; i++) {
    Color c = opts.clrngs[FACES].get_col(i);
    // find starter edges with color c in polygon 0
    vector<int> edges;
    for (unsigned int j = 0; j < polar_polygons[0].edges().size(); j++) {
      if (polar_polygons[0].colors(EDGES).get(j) == c) {
        // place polygon/edge(s) in unique queue
        pair<int, int> entry = make_pair(0, j);
        auto found = find(edge_queue.begin(), edge_queue.end(), entry);
        if (found == edge_queue.end()) {
          edge_queue.push_back(entry);
        }
      }
    }
    // process queue
    while (edge_queue.size()) {
      pair<int, int> polygon_pair = edge_queue[0];
      int polygon_no = polygon_pair.first;
      int edge_no = polygon_pair.second;

      // for that polygon edge
      // unset color
      polar_polygons[polygon_no].colors(EDGES).set(edge_no, Color());
      // collect edge to color post process
      edge_colors[edge_map_color].push_back(make_pair(polygon_no, edge_no));

      // find corresponding edge in opposite polygon
      Vec3d v1 = polar_polygons[polygon_no].verts(
          polar_polygons[polygon_no].edges(edge_no)[0]);
      Vec3d v2 = polar_polygons[polygon_no].verts(
          polar_polygons[polygon_no].edges(edge_no)[1]);
      int polygon_oppo = (polygon_no) ? 0 : 1;
      int edge_oppo =
          find_edge_by_coords(polar_polygons[polygon_oppo], v1, v2, opts.eps);
      if (edge_oppo == -1) {
        // wasn't found
        edge_queue.erase(edge_queue.begin());
        continue;
      }
      // if corresponding edge has color
      Color c_oppo = polar_polygons[polygon_oppo].colors(EDGES).get(edge_oppo);
      if (c_oppo.is_set()) {
        // for edges of that color
        for (unsigned int j = 0;
             j < polar_polygons[polygon_oppo].edges().size(); j++) {
          if (polar_polygons[polygon_oppo].colors(EDGES).get(j) == c_oppo) {
            // place polygon/edge(s) in unique queue
            pair<int, int> entry = make_pair(polygon_oppo, j);
            auto found = find(edge_queue.begin(), edge_queue.end(), entry);
            if (found == edge_queue.end()) {
              edge_queue.push_back(entry);
            }
          }
        }
      }

      // like pop_front();
      edge_queue.erase(edge_queue.begin());
    }
    // next map color for collected edges
    edge_map_color++;
  }

  // color polygons as compound polygons
  map<int, vector<pair<int, int>>>::iterator mi;
  for (mi = edge_colors.begin(); mi != edge_colors.end(); mi++) {
    int map_index = mi->first;
    vector<pair<int, int>> e_indexes = mi->second;
    for (unsigned int i = 0; i < e_indexes.size(); i++) {
      pair<int, int> edge_indexes = e_indexes[i];
      polar_polygons[edge_indexes.first].colors(EDGES).set(
          edge_indexes.second, opts.clrngs[FACES].get_col(map_index));
    }
  }
}

// geom copy
Geometry find_polar_polygon(Geometry geom, const vector<faceList *> &face_list,
                            const ncon_opts &opts)
{
  const vector<vector<int>> &edges = geom.edges();
  const vector<Vec3d> &verts = geom.verts();

  // some vertices are not right on the z plane
  double epsilon_local = 1e-8;
  if (opts.eps > epsilon_local)
    epsilon_local = opts.eps;

  // edges in polygon will be implicit
  geom.add_missing_impl_edges();

  // clear all colors
  geom.colors(FACES).clear();
  geom.colors(EDGES).clear();
  geom.colors(VERTS).clear();

  // color by polygon number
  for (unsigned int i = 0; i < face_list.size(); i++) {
    int face_no = (opts.hybrid) ? i : face_list[i]->face_no;
    int polygon_no = face_list[i]->polygon_no;
    // int lat = face_list[i]->lat;
    // fprintf(stderr,"face_no = %d polygon_no = %d lat = %d\n",
    //   face_no, polygon_no, lat);
    Color c = opts.clrngs[FACES].get_col(polygon_no);
    geom.colors(FACES).set(face_no, c);
  }

  vector<int> delete_elem;
  if (!opts.hybrid) {
    for (unsigned int i = 0; i < face_list.size(); i++) {
      int lon = face_list[i]->lon;
      // if (lon >= (opts.longitudes.front() / 2))
      if (!(lon == 0 || lon == ((opts.longitudes.front() / 2) - 1)))
        delete_elem.push_back(i);
    }
    geom.del(FACES, delete_elem);
  }

  // edge and vertex colors from faces
  Coloring clrng(&geom);
  clrng.add_cmap(opts.clrngs[FACES].clone());
  clrng.e_from_adjacent(FACES);

  Geometry polar_polygon;
  for (unsigned int i = 0; i < geom.verts().size(); i++) {
    Vec3d vert = verts[i];
    // in case of build method 3, some points were set aside so not to get
    // into twist plane, zero out
    if ((opts.build_method == 3) && fabs(vert[2]) < opts.eps * 3.0) {
      vert[2] = 0.0;
    }
    polar_polygon.add_vert(vert);
  }
  for (unsigned int i = 0; i < edges.size(); i++) {
    vector<int> edge = edges[i];
    double z0 = geom.verts(edge[0])[2];
    double z1 = geom.verts(edge[1])[2];
    if (geom.colors(EDGES).get(i).is_set() &&
        double_eq(z0, 0.0, epsilon_local) &&
        double_eq(z1, 0.0, epsilon_local)) {
      polar_polygon.add_edge(edges[i], geom.colors(EDGES).get(i));
    }
  }

  polar_polygon.del(VERTS, polar_polygon.get_info().get_free_verts());

  // remove inline vertices
  if (opts.build_method == 3) {
    vector<int> del_verts;
    for (unsigned int i = opts.ncon_order; i < polar_polygon.verts().size();
         i++) {
      vector<int> es = find_edges_with_vertex(polar_polygon.edges(), i);
      Color c = polar_polygon.colors(EDGES).get(es[0]);
      Vec3d v1 = polar_polygon.verts(i);
      vector<int> vs;
      for (unsigned int j = 0; j < es.size(); j++) {
        for (unsigned int k = 0; k < 2; k++) {
          unsigned int v = polar_polygon.edges(es[j])[k];
          if (v != i)
            vs.push_back(v);
        }
      }
      for (unsigned int l = 0; l < vs.size(); l++) {
        for (unsigned int m = l + 1; m < vs.size(); m++) {
          Vec3d v2 = polar_polygon.verts(vs[l]);
          Vec3d v3 = polar_polygon.verts(vs[m]);
          // if v1 is in line with v2 and v3 then mark it for deletion
          if (point_in_segment(v1, v2, v3, opts.eps).is_set()) {
            // add new edge
            polar_polygon.add_edge(make_edge(vs[m], vs[l]), c);
            del_verts.push_back(i);
          }
        }
      }
    }

    // (some duplicates)
    sort(del_verts.begin(), del_verts.end());
    auto dv = unique(del_verts.begin(), del_verts.end());
    del_verts.resize(dv - del_verts.begin());
    polar_polygon.del(VERTS, del_verts);
  }

  // don't need face
  polar_polygon.clear(FACES);

  return polar_polygon;
}

struct ht_less {
  static double get_eps() { return 1e-10; }
  bool operator()(const double h0, const double h1) const
  {
    return double_lt(h0, h1, get_eps());
  }
};

void lookup_face_color(Geometry &geom, const int f, const vector<Vec3d> &axes,
                       vector<map<double, Color, ht_less>> &heights,
                       const bool other_axis, const ncon_opts &opts)
{
  double epsilon_local = ht_less::get_eps();
  if (opts.eps > epsilon_local)
    epsilon_local = opts.eps;

  int ax = double_ge(geom.face_cent(f)[2], 0.0,
                     epsilon_local); // z-coordinate determines axis
  if (other_axis)
    ax = (!ax) ? 1 : 0;
  double hts[3];
  for (int v_idx = 0; v_idx < 3; v_idx++)
    hts[v_idx] = vdot(geom.face_v(f, v_idx), axes[ax]);
  // try to select non-horizontal edge
  int offset = double_eq(hts[0], hts[1], epsilon_local);
  double ht;
  if (offset && double_eq(hts[1], hts[2], epsilon_local))
    ht = hts[0] / fabs(hts[0]); // horizontal edge (on  horizontal face)
  else {
    // Find nearpoint of swept edge line, make unit, get height on axis
    Vec3d near_pt = nearest_point(Vec3d(0, 0, 0), geom.face_v(f, offset),
                                  geom.face_v(f, offset + 1));
    ht = vdot(near_pt.unit(), axes[ax]);
  }
  map<double, Color, ht_less>::iterator mi;
  if ((mi = heights[ax].find(ht)) != heights[ax].end())
    set_face_color(geom, f, mi->second);
}

void lookup_edge_color(Geometry &geom, const int e, const vector<Vec3d> &axes,
                       vector<map<double, Color, ht_less>> &heights,
                       const bool other_axis, const ncon_opts &opts)
{
  double epsilon_local = ht_less::get_eps();
  if (opts.eps > epsilon_local)
    epsilon_local = opts.eps;

  int ax = geom.edge_cent(e)[2] >= 0.0; // z-coordinate determines axis
  if (other_axis)
    ax = (!ax) ? 1 : 0;
  double hts[2];
  for (int v_idx = 0; v_idx < 2; v_idx++)
    hts[v_idx] = vdot(geom.edge_v(e, v_idx), axes[ax]);
  // select horizontal edges that don't intersect the axis
  if (double_eq(hts[0], hts[1], epsilon_local) &&
      !lines_intersection(geom.edge_v(e, 0), geom.edge_v(e, 1), Vec3d(0, 0, 0),
                          axes[ax], epsilon_local)
           .is_set()) {
    double ht = vdot(geom.edge_v(e, 0).unit(), axes[ax]);
    map<double, Color, ht_less>::iterator mi;
    if ((mi = heights[ax].find(ht)) != heights[ax].end()) {
      set_edge_color(geom, e, mi->second);
    }
  }
}

void ncon_face_coloring_by_compound(Geometry &geom,
                                    const Geometry &polar_polygon,
                                    const ncon_opts &opts)
{
  // for method 3 and digons, faces have trouble getting colored
  // take colors from attached edge
  if ((opts.build_method == 3) && opts.digons) {
    if (strchr("QF", opts.edge_coloring_method))
      opts.warning("method 3 and digons: face colorings are taken from\nedge "
                   "colorings. -e is not set",
                   'e');
    else {
      // need to filter out invisible edges
      // Coloring clrng(&geom);
      // clrng.f_from_adjacent(EDGES);
      for (unsigned int i = 0; i < geom.faces().size(); i++) {
        vector<int> face = geom.faces(i);
        unsigned int sz = face.size();
        for (unsigned int j = 0; j < sz; j++) {
          vector<int> edge = make_edge(face[j], face[(j + 1) % sz]);
          int edge_no = find_edge_in_edge_list(geom.edges(), edge);
          Color c = geom.colors(EDGES).get(edge_no);
          if ((edge_no != -1) && !c.is_invisible()) {
            geom.colors(FACES).set(i, c);
            break;
          }
        }
      }
    }

    // no further code needed
    return;
  }

  vector<Vec3d> axes(2);
  vector<map<double, Color, ht_less>> heights(2);

  // note: N and D are not pre-doubled here for build_method 2
  int n = opts.ncon_order;
  int twist = opts.twist;
  bool hyb = opts.hybrid;

  if (opts.build_method == 2) {
    if (opts.radius_set) {
      if (double_ne(opts.inner_radius, opts.outer_radius, opts.eps)) {
        if (!opts.d_of_one) {
          if (hyb) {
            hyb = true;
          }
        }
      }
      // if D == 1 or N-D == 1
      if (opts.d_of_one) {
        if (hyb) {
          hyb = false;
        }
      }
    }
  }

  // for negative twists
  double half_twist = 0.5 * ((twist > 0) ? 1 : -1);

  axes[0] = Vec3d::Y;
  axes[1] =
      Trans3d::rotate(Vec3d::Z, -2 * M_PI * (twist - half_twist * hyb) / n) *
      axes[0];

  for (unsigned int i = 0; i < polar_polygon.edges().size(); i++) {
    for (int ax = 0; ax < 2; ax++) {
      Vec3d near_pt = nearest_point(Vec3d(0, 0, 0), polar_polygon.edge_v(i, 0),
                                    polar_polygon.edge_v(i, 1));
      double ht = vdot(near_pt.unit(), axes[ax]);
      Color c = polar_polygon.colors(EDGES).get(i);
      if (opts.opacity[FACES] > -1) {
        int opq = opts.face_pattern[i % opts.face_pattern.size()] == '1'
                      ? opts.opacity[FACES]
                      : 255;
        c = set_opacity(c, opq);
      }
      heights[ax][ht] = c;
    }
  }

  bool found = true;
  for (unsigned int f = 0; f < geom.faces().size(); f++) {
    if (geom.faces(f).size() > 2) {
      // if, because negative radii, the face center is shifted onto the wrong
      // axis then no color will be found for look up, try the other axis
      Color c = geom.colors(FACES).get(f);
      if (c.is_invisible() && opts.lon_invisible)
        continue;
      if ((c == opts.face_default_color) && (opts.build_method != 3))
        continue;
      lookup_face_color(geom, f, axes, heights, false, opts);
      c = geom.colors(FACES).get(f);
      if (!c.is_set()) {
        lookup_face_color(geom, f, axes, heights, true, opts);
        c = geom.colors(FACES).get(f);
        if (!c.is_set())
          found = false;
      }
    }
  }

  if (!found && !opts.digons) {
    opts.warning("color by compound: some faces could not be colored", 'f');
    opts.warning("color by compound: try a different method", "z");
  }
}

// opts is not const since some options are calculated
int process_hybrid(Geometry &geom, ncon_opts &opts)
{
  int ret = 0;

  // hybrids which are really point cuts with radii swapped on opposite side
  bool special_hybrids =
      ((opts.build_method == 2) && (opts.d_of_one) && opts.radius_set);
  // was !std::isnan(opts.inner_radius));

  // retain longitudes settings
  int longitudes_back = opts.longitudes.back();

  // attributes of elements
  vector<coordList *> coordinates;
  vector<faceList *> face_list;
  vector<edgeList *> edge_list;

  // create memory for poles 0 - North Pole 1 - South Pole
  vector<poleList *> pole;
  pole.push_back(new poleList);
  pole.push_back(new poleList);

  // keep track of caps
  vector<int> caps;

  Geometry geom_d;

  // inner and outer radius calculated if not set
  // double sweep is set in build_globe()

  // build side cut half first
  double inner_radius_save = opts.inner_radius;
  double outer_radius_save = opts.outer_radius;
  bool point_cut_save = opts.point_cut;
  if (opts.build_method == 1)
    opts.point_cut = false;
  else if (opts.build_method == 2)
    opts.point_cut = (special_hybrids) ? true : false;
  else if (opts.build_method == 3)
    opts.point_cut = (opts.angle_is_side_cut) ? true : false;

  opts.longitudes.back() = opts.longitudes.front() / 2;
  bool second_half = false;
  build_globe(geom_d, coordinates, face_list, edge_list, pole, caps,
              opts.inner_radius, opts.outer_radius, opts.radius_inversion,
              opts.double_sweep, second_half, opts);
  bool radius_inversion_save = opts.radius_inversion;

  if (opts.face_coloring_method == 'c')
    opts.polar_polygons.push_back(find_polar_polygon(geom_d, face_list, opts));

  // delete half the model
  if (opts.build_method == 3)
    delete_unused_longitudes(geom_d, face_list, edge_list, caps, false, true,
                             opts);

  // backup face longitudes that are needed for flood fill
  if (opts.face_coloring_method == 'f')
    backup_flood_longitude_faces(geom_d, face_list, opts);

  // backup edge longitudes that are needed for flood fill
  if (opts.edge_coloring_method == 'f')
    backup_flood_longitude_edges(geom_d, edge_list, opts);

  // start over. build base part second, then rotate it
  clear_coord(coordinates);
  clear_faces(face_list);
  clear_edges(edge_list);

  caps.clear();

  opts.inner_radius = inner_radius_save;
  opts.outer_radius = outer_radius_save;
  opts.point_cut = !opts.point_cut;
  if (special_hybrids) {
    // don't let point cut go to side cut
    opts.point_cut = true;
    swap(opts.inner_radius, opts.outer_radius);
  }
  opts.longitudes.back() = opts.longitudes.front() / 2;
  second_half = true;
  build_globe(geom, coordinates, face_list, edge_list, pole, caps,
              opts.inner_radius, opts.outer_radius, opts.radius_inversion,
              opts.double_sweep, second_half, opts);
  opts.point_cut = point_cut_save;

  if (opts.face_coloring_method == 'c')
    opts.polar_polygons.push_back(find_polar_polygon(geom, face_list, opts));

  // delete half the model
  if (opts.build_method == 3)
    delete_unused_longitudes(geom, face_list, edge_list, caps, true, true,
                             opts);
  caps.clear();

  // we can do the twist by transforming just one part.
  // negative angle because z reflection
  opts.twist_angle = hybrid_twist_angle(opts);
  if (special_hybrids) {
    opts.twist_angle = (360.0 / opts.ncon_order) * opts.twist;
    // swap these back for later use
    swap(opts.inner_radius, opts.outer_radius);
    opts.radius_inversion = radius_inversion_save;
  }
  geom.transform(Trans3d::rotate(0, 0, deg2rad(-opts.twist_angle)));
  // methods 1 and 2 need reflection
  if (opts.build_method < 3)
    geom.transform(Trans3d::reflection(Vec3d(0, 0, 1)));

  // merge the two halves and merge the vertices
  geom.append(geom_d);

  // merge by using polar orbit coordinates. this keeps the face_list pointing
  // to the right faces
  vector<polarOrb *> polar_orbit;
  find_polar_orbit(geom, polar_orbit, opts);
  merge_halves(geom, polar_orbit, opts);
  polar_orbit.clear();

  // for build method 3 there are some unmatched edges
  // make it a valid polyhedron, and be able to flood fill across divide
  // digons cause trouble with hybrids so bypass. prefer method 2 for digons
  vector<int> added_triangles;
  if ((opts.build_method == 3) && !opts.digons)
    add_triangles_to_close(geom, added_triangles, opts);

  opts.longitudes.back() =
      (opts.lon_invisible) ? opts.longitudes_save : longitudes_back;

  if (opts.face_coloring_method == 'f') {
    // restore face longitudes needed for flood fill
    restore_flood_longitude_faces(geom, face_list, opts);
    ret = ncon_face_coloring_by_adjacent_face(geom, face_list, opts);
  }
  else if (opts.face_coloring_method == 'k')
    ret = ncon_face_coloring_by_compound_flood(geom, face_list, caps, opts);

  if (opts.edge_coloring_method == 'f') {
    // restore edge longitudes needed for flood fill
    restore_flood_longitude_edges(geom, edge_list, opts);
    ncon_edge_coloring_by_adjacent_edge(geom, edge_list, pole, opts);
  }

  // if not a full model, added triangles no longer needed
  if (added_triangles.size() && !full_model(opts.longitudes) &&
      !opts.lon_invisible)
    geom.del(FACES, added_triangles);

  // allow for partial open model in hybrids
  delete_unused_longitudes(geom, face_list, edge_list, caps, false, false,
                           opts);

  caps.clear();

  // clean up
  clear_coord(coordinates);
  clear_faces(face_list);
  clear_edges(edge_list);

  return ret;
}

// opts is not const since some options are calculated
int process_normal(Geometry &geom, ncon_opts &opts)
{
  int ret = 0;

  vector<coordList *> coordinates;
  vector<faceList *> face_list;
  vector<edgeList *> edge_list;

  // create memory for poles 0 - North Pole 1 - South Pole
  vector<poleList *> pole;
  pole.push_back(new poleList);
  pole.push_back(new poleList);

  // keep track of caps
  vector<int> caps;

  // inner and outer radius calculated if not set
  // double sweep is set in build_globe()

  bool second_half = false;
  build_globe(geom, coordinates, face_list, edge_list, pole, caps,
              opts.inner_radius, opts.outer_radius, opts.radius_inversion,
              opts.double_sweep, second_half, opts);

  if (opts.face_coloring_method == 'c')
    opts.polar_polygons.push_back(find_polar_polygon(geom, face_list, opts));

  // now we do the twisting
  // twist plane is now determined by points landing on z-plane
  vector<polarOrb *> polar_orbit;
  find_polar_orbit(geom, polar_orbit, opts);

  // method 1: can't twist when half or less of model is showing
  // method 2 and 3: whole model exists even though some longitudes will later
  // be deleted
  if ((opts.build_method > 1) ||
      (opts.build_method == 1 &&
       (2 * opts.longitudes.back() > opts.longitudes.front()))) {
    // in case of build_method 3, polygon may have been doubled. treat like 2N
    int adjust = (opts.double_sweep) ? 2 : 1;
    ncon_twist(geom, polar_orbit, coordinates, face_list, edge_list,
               opts.ncon_order * adjust, opts.twist * adjust, opts);
  }

  // for build method 3 there are some duplicate vertices and unmatched edges
  vector<int> added_triangles;
  if (opts.build_method == 3) {
    polar_orbit.clear();
    find_polar_orbit(geom, polar_orbit, opts);
    merge_halves(geom, polar_orbit, opts);
    polar_orbit.clear();

    // for build method 3 there are some unmatched edges
    // make it a valid polyhedron, and be able to flood fill across divide
    add_triangles_to_close(geom, added_triangles, opts);
  }

  if (opts.build_method > 1 && opts.face_coloring_method == 'f')
    ret = ncon_face_coloring_by_adjacent_face(geom, face_list, opts);
  else if (opts.face_coloring_method == 'k')
    ret = ncon_face_coloring_by_compound_flood(geom, face_list, caps, opts);

  if (opts.build_method > 1 && opts.edge_coloring_method == 'f')
    ncon_edge_coloring_by_adjacent_edge(geom, edge_list, pole, opts);

  // if not a full model, added triangles no longer needed
  if (added_triangles.size() && !full_model(opts.longitudes) &&
      !opts.lon_invisible)
    geom.del(FACES, added_triangles);

  delete_unused_longitudes(geom, face_list, edge_list, caps, false, false,
                           opts);

  // horizontal closure only works for build method 1 and 2
  if (opts.build_method < 3)
    if (strchr(opts.closure.c_str(), 'v'))
      close_latitudinal(geom, face_list, pole, opts);

  // color closures
  if ((opts.build_method != 3) && (opts.closure.length())) {
    if (strchr("SC", opts.face_coloring_method)) {
      for (auto i : face_list) {
        int j = i->face_no;
        if (i->lat < 0)
          set_face_color(geom, j, opts.face_default_color, opts.opacity[FACES]);
      }
    }

    if (strchr("SC", opts.face_coloring_method)) {
      for (auto i : edge_list) {
        int j = i->edge_no;
        if (i->lat < 0)
          set_edge_color(geom, j, Color::invisible, 255);
      }
    }
  }

  caps.clear();

  // clean up
  clear_coord(coordinates);
  clear_faces(face_list);
  clear_edges(edge_list);

  return ret;
}

void ncon_make_inv_edges_of_inv_faces(Geometry &geom)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<vector<int>> &edges = geom.edges();

  for (unsigned int i = 0; i < edges.size(); i++) {
    if ((geom.colors(EDGES).get(i)).is_invisible())
      continue;
    vector<int> face_idx = find_faces_with_edge(faces, edges[i]);
    vector<Color> cols;
    for (int j : face_idx)
      cols.push_back(geom.colors(FACES).get(j));
    Color c = ncon_average_edge_color(cols);
    if (c.is_invisible())
      set_edge_color(geom, i, Color::invisible, 255);
  }
}

void filter(Geometry &geom, const char *elems)
{
  for (const char *p = elems; *p; p++) {
    switch (*p) {
    case 'v':
      geom.colors(VERTS).clear();
      break;
    case 'e':
      geom.colors(EDGES).clear();
      break;
    case 'E':
      ncon_make_inv_edges_of_inv_faces(geom);
      break;
    case 'f':
      geom.clear(FACES);
      break;
    }
  }
}

Geometry build_gear_polygon(const int N, const int D, const double o_radius,
                            const double i_radius)
{
  Geometry gear;

  int N2 = N;
  double arc = 180.0 / N2;
  double angle = 0.0;

  if ((D != 1) && ((N2 - D) != 1))
    N2 *= 2;
  else
    arc *= 2.0;

  vector<int> face;
  for (int i = 0; i < N2; i++) {
    double radius = (is_even(i)) ? o_radius : i_radius;
    gear.add_vert(
        Vec3d(cos(deg2rad(angle)) * radius, sin(deg2rad(angle)) * radius, 0));
    gear.add_edge(make_edge(i, (i + 1) % N2));
    face.push_back(i);
    angle += arc;
  }

  return gear;
}

// transfer colors of the regular polygon to the gear polygon
void transfer_colors(Geometry &gear, const Geometry &pgon,
                     const ncon_opts &opts)
{
  /* maximum vertex may not be vertex 0
    double radius_pgon = GeometryInfo(pgon).vert_dist_lims().max;
    double radius_gear = GeometryInfo(gear).vert_dist_lims().max;
    gear.transform(Trans3d::scale(radius_pgon/radius_gear));
  */

  // line up polygons
  double radius_pgon = pgon.verts()[0].len();
  double radius_gear = gear.verts()[0].len();
  gear.transform(Trans3d::scale(radius_pgon / radius_gear));

  map<int, int> v_map;
  for (unsigned int i = 0; i < pgon.verts().size(); i++) {
    Vec3d pv = pgon.verts(i);
    v_map[i] = -1;
    for (unsigned int j = 0; j < gear.verts().size(); j++) {
      Vec3d gv = gear.verts(j);
      if (!compare(pv, gv, opts.eps)) {
        v_map[i] = j;
        break;
      }
    }
  }

  // reset gear radius
  gear.transform(Trans3d::scale(radius_gear / radius_pgon));

  // in case polygons don't line up
  if (!v_map.size()) {
    opts.warning("transfer_colors(): polygons do not align", 'f');
    return;
  }

  // assign vertex colors
  for (unsigned int i = 0; i < v_map.size(); i++) {
    int gear_idx = v_map[i];
    if (opts.hide_indent)
      gear.colors(VERTS).set(gear_idx, pgon.colors(VERTS).get(i));
  }

  if (opts.hide_indent && opts.n_doubled) {
    unsigned int start = (opts.radius_inversion) ? 0 : 1;
    for (unsigned int i = start; i < gear.verts().size(); i += 2) {
      gear.colors(VERTS).set(i, Color(Color::invisible));
    }
  }

  // digon colors are taken from edges
  if (opts.digons) {
    // if edges are not set, take colors from vertices
    Color ec = gear.colors(EDGES).get(0);
    if (!ec.is_set()) {
      // extract invisible elements so they don't interfere with coloring
      vector<int> v_idx;
      for (unsigned int i = 0; i < gear.verts().size(); i++) {
        Color c = gear.colors(VERTS).get(i);
        if (c.is_invisible()) {
          v_idx.push_back(i);
          gear.colors(VERTS).set(i, Color());
        }
      }

      Coloring clrng(&gear);
      clrng.e_from_adjacent(VERTS);

      // restore invisible elements
      for (unsigned int i = 0; i < v_idx.size(); i++) {
        gear.colors(VERTS).set(v_idx[i], Color::invisible);
      }
    }
    return;
  }

  // get left and right edge color at pgon vertices
  map<int, Color> rcol;
  map<int, Color> lcol;
  map<int, int>::iterator vm;
  // cycle through map
  for (vm = v_map.begin(); vm != v_map.end(); vm++) {
    if (vm->second == -1)
      continue;
    unsigned int i = vm->first;
    vector<int> es = find_edges_with_vertex(pgon.edges(), i);
    unsigned int v0_idx = pgon.edges(es[0])[0];
    if (v0_idx == i)
      v0_idx = pgon.edges(es[0])[1];
    Vec3d v0 = pgon.verts()[v0_idx];
    unsigned int v1_idx = pgon.edges(es[1])[0];
    if (v1_idx == i)
      v1_idx = pgon.edges(es[1])[1];
    Vec3d v1 = pgon.verts()[v1_idx];

    Vec3d ax = vcross(pgon.verts()[i], Vec3d::zero).unit();
    Vec3d vec0 = pgon.verts()[i] - v0;
    Vec3d vec1 = pgon.verts()[i] - v1;
    double angle0 = angle_around_axis(vec1, vec0, ax);
    double angle1 = angle_around_axis(vec0, vec1, ax);

    // save left and right colors
    lcol[i] = pgon.colors(EDGES).get(es[(angle0 < angle1) ? 0 : 1]);
    rcol[i] = pgon.colors(EDGES).get(es[(angle0 > angle1) ? 0 : 1]);
  }

  // set left and right edge color at gear vertices
  for (vm = v_map.begin(); vm != v_map.end(); vm++) {
    if (vm->second == -1)
      continue;
    unsigned int i = vm->first;
    unsigned int j = vm->second;
    vector<int> es = find_edges_with_vertex(gear.edges(), j);
    unsigned int v0_idx = gear.edges(es[0])[0];
    if (v0_idx == j)
      v0_idx = gear.edges(es[0])[1];
    Vec3d v0 = gear.verts()[v0_idx];
    unsigned int v1_idx = gear.edges(es[1])[0];
    if (v1_idx == j)
      v1_idx = gear.edges(es[1])[1];
    Vec3d v1 = gear.verts()[v1_idx];

    Vec3d ax = vcross(gear.verts()[j], Vec3d::zero).unit();
    Vec3d vec0 = pgon.verts()[i] - v0;
    Vec3d vec1 = pgon.verts()[i] - v1;
    double angle0 = angle_around_axis(vec1, vec0, ax);
    double angle1 = angle_around_axis(vec0, vec1, ax);

    // set left and right colors of gear edges
    gear.colors(EDGES).set(es[(angle0 < angle1) ? 0 : 1], lcol[i]);
    gear.colors(EDGES).set(es[(angle0 > angle1) ? 0 : 1], rcol[i]);
  }
}

void pgon_post_process(Geometry &pgon, vector<Vec3d> &axes, const int N,
                       const int twist, const bool hybrid,
                       const ncon_opts &opts)
{
  int t_mult = opts.symmetric_coloring ? 1 : 2;

  // for even n, twist 0, symmetric coloring does't happen, so temporarily
  // twist half way. Use absolute value of twist
  int t = std::abs(twist);
  if (opts.symmetric_coloring && is_even(N) && t == 0) {
    int mod_twist = t % N;
    if (!mod_twist)
      t = N / 2;
  }

  int Dih_num = N / gcd(t_mult * t - hybrid, N);
  vector<vector<set<int>>> sym_equivs;
  get_equiv_elems(pgon, Symmetry(Symmetry::D, Dih_num).get_trans(),
                  &sym_equivs);

  if (opts.info)
    fprintf(stderr, "symmetry polygon (%s) colored as D%d\n",
            GeometryInfo(pgon).get_symmetry_type_name().c_str(), Dih_num);

  Coloring e_clrng(&pgon);
  ColorMap *f_map = opts.clrngs[FACES].clone();
  e_clrng.add_cmap(f_map);
  e_clrng.e_sets(sym_equivs[1]);

  Coloring v_clrng(&pgon);
  ColorMap *e_map = opts.clrngs[EDGES].clone();
  v_clrng.add_cmap(e_map);
  v_clrng.v_sets(sym_equivs[0]);

  // pgon.transform(Trans3d::rotate(Vec3d::Z, (1-2*(D>1 &&
  // is_even(D%2)))*M_PI/2));
  pgon.transform(Trans3d::rotate(Vec3d::Z, M_PI / 2));

  // for negative twists. get sign from opts.twist
  t = t * ((opts.twist < 0) ? -1 : 1);
  double half_twist = 0.5 * ((t > 0) ? 1 : -1);

  axes[0] = Vec3d::Y;
  axes[1] =
      Trans3d::rotate(Vec3d::Z, -2 * M_PI * (t - half_twist * hybrid) / N) *
      axes[0];
}

void rotate_polygon(Geometry &pgon, const int N, const bool point_cut,
                    const bool hybrid, const ncon_opts &opts)
{
  // rotate polygons
  double rot_angle = 0;
  if ((opts.build_method == 3) && double_ne(opts.angle, 0, opts.eps)) {
    rot_angle = deg2rad(opts.angle);
    if (hybrid && point_cut)
      rot_angle += M_PI / N;
  }
  else if (!is_even(N) || !point_cut || hybrid)
    rot_angle = M_PI / N;

  pgon.transform(Trans3d::rotate(Vec3d::Z, rot_angle));

  // odds are flipped under these rules
  bool is_flipped = false;
  if (!is_even(N)) {
    // method 3 these angles will cause an upward flip
    if ((opts.build_method == 3) && double_ne(opts.angle, 0, opts.eps) &&
        (angle_on_aligned_polygon(opts.angle, N, opts.eps)))
      is_flipped = true;
  }
  else {
    // special hybrids and their kin when N/2 is odd
    if ((opts.build_method == 2) && !is_even(N / 2) && (opts.d_of_one) &&
        !std::isnan(opts.inner_radius))
      is_flipped = true;
  }

  // note: the polygon is still on its side
  if (is_flipped)
    pgon.transform(Trans3d::reflection(Vec3d(1, 0, 0)));
}

// another coloring method by Adrian Rossiter
void color_by_symmetry(Geometry &geom, Geometry &pgon, const ncon_opts &opts)
{
  // note: N and D are not pre-doubled here for build_method 2
  int N = opts.ncon_order;
  int D = opts.d;
  int twist = std::abs(opts.twist);
  bool pc = opts.point_cut;
  bool hyb = opts.hybrid;

  vector<Vec3d> axes(2);
  vector<map<double, Color, ht_less>> heights(2);

  // check for change in mode
  bool continue_after_rotation = false;
  if (opts.build_method == 2) {
    if (opts.radius_set) {
      if (double_eq(opts.inner_radius, opts.outer_radius, opts.eps)) {
        if (!opts.d_of_one) {
          N *= 2;
          D = 1;
          twist *= 2;
          pc = true;
          if (hyb) {
            hyb = false;
            twist--;
          }
        }
      }
      // radii not equal
      else {
        if (!opts.d_of_one) {
          if (hyb) {
            pc = false;
            hyb = true;
          }
        }
      }
      // if radii set but differ and D == 1
      if (opts.d_of_one) {
        if (hyb) {
          hyb = false;
        }
      }
    }
  }
  else if (opts.build_method == 3) {
    if (opts.angle_is_side_cut)
      pc = false;
    else if (opts.double_sweep)
      continue_after_rotation = true;
  }

  // create polygon
  Polygon dih(N, D, Polygon::dihedron);
  dih.set_radius(0, GeometryInfo(geom).vert_dist_lims().max);
  dih.make_poly(pgon);
  pgon.add_missing_impl_edges();
  pgon.clear(FACES);

  rotate_polygon(pgon, N, pc, hyb, opts);

  // method 3: reflect on Y axis when angled
  // done after rotation
  if (continue_after_rotation) {
    // if it is formed by double sweeping, mirror on Y
    if (opts.double_sweep) {
      Geometry pgon_refl;
      pgon_refl = pgon;
      pgon_refl.transform(Trans3d::reflection(Vec3d(0, 1, 0)));
      pgon.append(pgon_refl);

      // this flip is needed
      if (!is_even(N))
        pgon.transform(Trans3d::reflection(Vec3d(1, 0, 0)));

      // when angle is used
      // a normal becomes a normal side cut of 2N/2D with a twist of 2T
      // a hybrid becomes a normal side cut of 2N/2D with a twist of 2T-1
      N *= 2;
      D *= 2;
      twist *= 2;
      if (hyb) {
        hyb = false;
        twist--;
      }
      pc = false;
    }
  }

  pgon_post_process(pgon, axes, N, twist, hyb, opts);

  // if build method 2 and radii were set, build gear polygon and replace pgon
  bool rad_set = (opts.radius_set &&
                  (double_ne(opts.outer_radius, opts.inner_radius, opts.eps)));
  bool gear_polygon_used = false;
  if ((opts.build_method == 2) && (rad_set || opts.digons)) {
    gear_polygon_used = true;

    // build gear polygon
    Geometry gear =
        build_gear_polygon(N, D, opts.outer_radius, opts.inner_radius);

    rotate_polygon(gear, N, pc, hyb, opts);

    pgon_post_process(gear, axes, N, twist, hyb, opts);

    // transfer colors from pgon to the gear polygon
    transfer_colors(gear, pgon, opts);

    pgon = gear;
  }

  // special case, if twisted half way, turn upside down (post coloring)
  // use original N as it may have been doubled
  // catch hybrid case if special hybrids
  if (is_even(opts.ncon_order) && (opts.posi_twist != 0) && (N / twist == 2) &&
      !opts.hybrid)
    pgon.transform(Trans3d::reflection(Vec3d(0, 1, 0)));

  // color faces. if method 3 and digons, bypass for code below
  if ((opts.face_coloring_method == 's') &&
      !((opts.build_method == 3) && opts.digons)) {
    // Find nearpoints of polygon edge lines, make unit, get height on each
    // axis
    for (unsigned int i = 0; i < pgon.edges().size(); i++) {
      for (int ax = 0; ax < 2; ax++) {
        Vec3d near_pt =
            nearest_point(Vec3d(0, 0, 0), pgon.edge_v(i, 0), pgon.edge_v(i, 1));
        double ht = vdot(near_pt.unit(), axes[ax]);
        Color c = pgon.colors(EDGES).get(i);
        if (opts.opacity[FACES] > -1) {
          int opq = opts.face_pattern[i % opts.face_pattern.size()] == '1'
                        ? opts.opacity[FACES]
                        : 255;
          c = set_opacity(c, opq);
        }
        heights[ax][ht] = c;
      }
    }

    bool found = true;
    for (unsigned int f = 0; f < geom.faces().size(); f++) {
      if (geom.faces(f).size() > 2) {
        // if, because negative radii, the face center is shifted onto the
        // wrong axis then no color will be found for look up, try the other
        // axis
        Color c = geom.colors(FACES).get(f);
        if (c.is_invisible() && opts.lon_invisible)
          continue;
        if ((c == opts.face_default_color) && (opts.build_method != 3))
          continue;
        lookup_face_color(geom, f, axes, heights, false, opts);
        c = geom.colors(FACES).get(f);
        if (!c.is_set()) {
          lookup_face_color(geom, f, axes, heights, true, opts);
          c = geom.colors(FACES).get(f);
          if (!c.is_set())
            found = false;
        }
      }
    }

    if (!found && !opts.digons)
      opts.warning("color by symmetry: some faces could not be colored", 'f');
  }

  // color edges
  if (opts.edge_coloring_method == 's') {
    if (opts.build_method == 2 && !opts.hide_indent && !gear_polygon_used) {
      // build gear polygon for indented edges, if not already built
      pgon = build_gear_polygon(N, D, opts.outer_radius, opts.inner_radius);

      rotate_polygon(pgon, N, pc, hyb, opts);

      pgon_post_process(pgon, axes, N, twist, hyb, opts);

      // no need to transfer colors. only using vertices

      // special case, if twisted half way, turn upside down (post coloring)
      // use original N as it may have been doubled
      // catch hybrid case if special hybrids
      if (is_even(opts.ncon_order) && (opts.posi_twist != 0) &&
          (N / twist == 2) && !opts.hybrid)
        pgon.transform(Trans3d::reflection(Vec3d(0, 1, 0)));
    }

    // Find nearpoints of polygon vertices, make unit, get height on each axis
    for (int ax = 0; ax < 2; ax++)
      heights[ax].clear();
    for (unsigned int i = 0; i < pgon.verts().size(); i++) {
      for (int ax = 0; ax < 2; ax++) {
        double ht = vdot(pgon.verts(i).unit(), axes[ax]);
        Color c = pgon.colors(VERTS).get(i);
        if (opts.opacity[EDGES] > -1) {
          int opq = opts.edge_pattern[i % opts.edge_pattern.size()] == '1'
                        ? opts.opacity[EDGES]
                        : 255;
          c = set_opacity(c, opq);
        }
        heights[ax][ht] = c;
      }
    }

    // color marked edges
    bool found = true;
    for (unsigned int e = 0; e < geom.edges().size(); e++) {
      Color c = geom.colors(EDGES).get(e);
      if (c.is_maximum_index()) {
        lookup_edge_color(geom, e, axes, heights, false, opts);
        c = geom.colors(EDGES).get(e);
        // if, because negative radii, the edge center is shifted onto the
        // wrong axis no color will be found for look up, try the other axis
        if (c.is_maximum_index()) {
          // not found? try again
          lookup_edge_color(geom, e, axes, heights, true, opts);
          c = geom.colors(EDGES).get(e);
          // if still not found, unset color, set for warning
          if (c.is_maximum_index()) {
            set_edge_color(geom, e, opts.edge_default_color, 255);
            found = false;
          }
        }
      }
    }

    if (!found)
      opts.warning("color by symmetry: some edges could not be colored", 'e');

    // occasionally some vertices can be missed
    for (unsigned int i = 0; i < geom.verts().size(); i++) {
      Color c = geom.colors(VERTS).get(i);
      if (c.is_maximum_index())
        geom.colors(VERTS).set(i, Color::invisible);
    }

    // try to color poles. in method 2 a model can be forced upside down
    if (opts.add_poles) {
      int north = -1;
      int south = -1;
      find_poles(pgon, north, south, opts);

      // coincident points on model take those colors
      if (north != -1) {
        for (unsigned int i = 0; i < geom.verts().size(); i++) {
          if ((double_eq(geom.verts(i)[0], pgon.verts(north)[0])) &&
              (double_eq(geom.verts(i)[2], pgon.verts(north)[2])) &&
              (geom.verts(i)[1] > 0)) {
            // if (!compare(pgon.verts(north), pgon.verts(north), opts.eps)) {
            geom.colors(VERTS).set(i, pgon.colors(VERTS).get(north));
            break;
          }
        }
      }
      if (south != -1) {
        for (unsigned int i = 0; i < geom.verts().size(); i++) {
          if ((double_eq(geom.verts(i)[0], pgon.verts(south)[0])) &&
              (double_eq(geom.verts(i)[2], pgon.verts(south)[2])) &&
              (geom.verts(i)[1] < 0)) {
            geom.colors(VERTS).set(i, pgon.colors(VERTS).get(south));
            break;
          }
        }
      }
    }
  }

  // for method 3 and digons, faces have trouble getting colored
  // take colors from attached edge
  if ((opts.face_coloring_method == 's') && (opts.build_method == 3) &&
      opts.digons) {
    if (strchr("QF", opts.edge_coloring_method))
      opts.warning("method 3 and digons: face colorings are taken from\nedge "
                   "colorings. -e is not set",
                   'e');
    else {
      // need to filter out invisible edges
      // Coloring clrng(&geom);
      // clrng.f_from_adjacent(EDGES);
      for (unsigned int i = 0; i < geom.faces().size(); i++) {
        vector<int> face = geom.faces(i);
        unsigned int sz = face.size();
        for (unsigned int j = 0; j < sz; j++) {
          vector<int> edge = make_edge(face[j], face[(j + 1) % sz]);
          int edge_no = find_edge_in_edge_list(geom.edges(), edge);
          Color c = geom.colors(EDGES).get(edge_no);
          if ((edge_no != -1) && !c.is_invisible()) {
            geom.colors(FACES).set(i, c);
            break;
          }
        }
      }
    }
  }
}

int ncon_subsystem(Geometry &geom, ncon_opts &opts)
{
  int ret = 0;

  // initialize
  opts.twist_angle = 0;
  opts.posi_twist = std::abs(opts.twist % opts.ncon_order);
  opts.digons = (opts.ncon_order == 2 * opts.d) ? true : false;

  // for build method 2, multiply n and twist by 2
  // if d == 1 or n-d == 1, radii will equal
  opts.d_of_one = (opts.d == 1 || (opts.ncon_order - opts.d) == 1);
  if ((opts.build_method == 2) && !opts.d_of_one) {
    opts.ncon_order *= 2;
    opts.twist *= 2;
    opts.n_doubled = true;
  }

  opts.radius_set =
      ((opts.build_method == 2) &&
       (!std::isnan(opts.outer_radius) || !std::isnan(opts.inner_radius)))
          ? true
          : false;

  // the best way to do side cut with method 3 is angle
  if (opts.build_method == 3 && opts.hybrid && !opts.point_cut) {
    opts.angle = 180.0 / opts.ncon_order;
    opts.angle_is_side_cut = true;
  }

  bool point_cut_save = opts.point_cut;

  if (opts.info)
    fprintf(stderr, "\nmeasures: ==============================\n");

  if (opts.hybrid)
    ret = process_hybrid(geom, opts);
  else
    ret = process_normal(geom, opts);

  opts.point_cut = point_cut_save;

  // restore for possible reporting
  if ((opts.build_method == 2) && opts.n_doubled) {
    opts.ncon_order /= 2;
    opts.twist /= 2;
  }

  // build compound polygons first
  if (opts.face_coloring_method == 'c')
    build_compound_polygon(opts.polar_polygons, opts);

  if (opts.face_coloring_method == 's' || opts.edge_coloring_method == 's') {
    // borrow polygon 0
    if (opts.polar_polygons.size())
      (opts.polar_polygons[0]).clear_all();
    else
      opts.polar_polygons.push_back(Geometry());
    color_by_symmetry(geom, opts.polar_polygons[0], opts);
  }
  if (opts.face_coloring_method == 'c') {
    // polygon 1 (twin) is still intact
    ncon_face_coloring_by_compound(geom, opts.polar_polygons[1], opts);
  }

  // in the case of hybrid and side cut
  // model needs to be rotated into position at side_cut angles
  // done after color_by_symmetry for accuracy
  if (opts.build_method == 3 && opts.angle_is_side_cut && opts.hybrid) {
    geom.transform(Trans3d::rotate(0, deg2rad(180.0), 0) *
                   Trans3d::rotate(0, 0, deg2rad(opts.twist_angle)));
  }

  if (opts.info)
    model_info(geom, opts);

  // append here so it doesn't interfere with edge counts
  if (opts.add_symmetry_polygon && !opts.circuit_coloring) {
    int pol_num = 0;
    if (opts.edge_coloring_method == 's')
      pol_num = 0;
    else if (opts.edge_coloring_method == 'c')
      pol_num = 1;
    geom.append(opts.polar_polygons[pol_num]);
  }

  if (opts.edge_coloring_method == 'F')
    ncon_edge_coloring_from_faces(geom, opts);

  // Color post-processing
  // process the uninvolved edges of the n_icon
  if (opts.unused_edge_color.is_set())
    color_unused_edges(geom, opts.unused_edge_color);

  if (opts.edge_set_no_color)
    // now is the point that edges which are supposed to be unset can be done
    unset_marked_elements(geom);
  else
    // when partial model, some vertices can't be rotated, so out of sync with
    // their edges
    // when method 2 or 3, front vertices can get back color because of order
    // of edges in list faster to just recolor all vertices
    reassert_colored_verts(geom, opts.edge_default_color,
                           opts.unused_edge_color);

  // some edges can remain uncolored
  if (!opts.edge_set_no_color)
    color_uncolored_edges(geom, opts);

  // some faces can remain uncolored
  if (opts.face_coloring_method != '0')
    color_uncolored_faces(geom, opts);

  geom.orient(1); // positive orientation

  // elements can be chosen to be eliminated completely
  filter(geom, opts.hide_elems.c_str());

  // find compound part count
  if ((strchr("cC", opts.face_coloring_method)) && opts.list_compounds) {
    // find compound parts count for testing
    vector<Color> cols;
    for (unsigned int i = 0; i < geom.faces().size(); i++) {
      Color c = geom.colors(FACES).get(i);
      if ((c != opts.face_default_color) && !c.is_invisible()) {
        cols.push_back(c);
      }
    }

    sort(cols.begin(), cols.end());
    auto li = unique(cols.begin(), cols.end());
    cols.erase(li, cols.end());

    ret = (int)cols.size();
  }

  // clear for listing loop
  opts.twist_angle = 0;
  opts.posi_twist = 0;
  opts.digons = false;

  // clear polygons memory
  for (unsigned int i = 0; i < opts.polar_polygons.size(); i++)
    opts.polar_polygons[i].clear_all();
  opts.polar_polygons.clear();

  GeometryInfo info(geom);
  if (!info.is_closed())
    opts.warning("the model is not closed");

  return ret;
}

void surface_subsystem(ncon_opts &opts)
{
  int d = opts.list_d;

  // variables set for d mod 4 hybrid compound counts
  if (opts.list_compounds) {
    opts.build_method = 3;
    opts.d = opts.list_d;

    // epsilon lowered for large models
    opts.eps = 1e-11;

    // don't let these set
    opts.info = false;
    opts.split_bypass = false;
    opts.hide_elems = "";
    opts.face_default_color = Color(192, 192, 192, 255);
    opts.edge_default_color = Color(192, 192, 192, 255);

    // symmetry polygon coloring is faster than 'k'
    opts.face_coloring_method = 'c';
    opts.edge_coloring_method = 's';

    // generated only small model for counting compound parts
    opts.longitudes.clear();
    opts.longitudes.push_back(4);
    opts.longitudes.push_back(4);
    opts.longitudes_save = opts.longitudes.back();

    // need color map for counting colors
    opts.print_status_or_exit(read_colorings(opts.clrngs, "colorful"));
  }

  fprintf(stdout, "\n");

  char form = opts.ncon_surf[0];
  if (form == 'p')
    fprintf(stdout, "Only Even Order Point Cut models are listed\n");
  else if (form == 's')
    fprintf(stdout, "Only Even Order Side Cut models are listed\n");
  else if (form == 'o')
    fprintf(stdout, "Only Odd Order models are listed\n");
  else if (form == 'h')
    fprintf(stdout, "Only Hybrid models are listed\n");
  else if (form == 'i')
    fprintf(stdout, "Only Hybrids such that N/2 is even are listed\n");
  else if (form == 'j')
    fprintf(stdout, "Only Hybrids such that N/2 is odd are listed\n");
  else if (form == 'k')
    fprintf(stdout, "Only Hybrids such that N/4 is even are listed\n");
  else if (form == 'l')
    fprintf(stdout, "Only Hybrids such that N/4 is odd are listed\n");

  fprintf(stdout, "listing twists one %s way round to avoid repeats\n",
          (form == 'o') ? "half" : "quarter");

  fprintf(stdout, "listing only %s of %d to %d (option -K)\n",
          (!opts.list_compounds ? "surfaces" : "compound parts"),
          opts.filter_surfaces.front(), opts.filter_surfaces.back());

  if (opts.list_compounds && d == 1)
    fprintf(stdout, "when d = 1 no n_icons are compounds (option -D)\n");

  if (!opts.filter_case2)
    fprintf(stdout, "case 2 n-icons are depicted with {curly brackets}\n");

  if ((form != 'o') && (form != 'i'))
    fprintf(stdout, "non-chiral n-icons are depicted with [square brackets]\n");
  else if (form == 's')
    fprintf(stdout,
            "all even order side cut n-icons have at least two surfaces\n");

  string buffer;
  if (opts.list_compounds) {
    if ((form == 'p') && (is_even(d))) {
      fprintf(stdout, "when d is even, all even order point cut n-icons\n");
      fprintf(stdout, "will have at least two compound parts\n");
    }
    else if (d == 2) {
      if (form == 's')
        buffer = "even order side cut";
      else if (form == 'o')
        buffer = "odd order";
      else
        buffer = "hybrid";
      fprintf(stdout, "when d=2, no %s n-icons will have compound parts\n",
              buffer.c_str());
    }
  }

  fprintf(stdout, "\nd = %d (option -D) (only n/d are measured)\n", d);

  fprintf(stdout, "\n");

  vector<int> ncon_range = opts.ncon_range;

  int inc = 2;
  if (form == 'o') {
    if (is_even(ncon_range.front()))
      ncon_range.front()++;
  }
  else {
    if (!is_even(ncon_range.front()))
      ncon_range.front()++;
  }

  if ((form == 'i') || (form == 'j')) {
    if (((form == 'i') && !is_even(ncon_range.front() / 2)) ||
        ((form == 'j') && is_even(ncon_range.front() / 2)))
      ncon_range.front() += 2;

    inc += 2;
    form = 'h';
  }
  else if ((form == 'k') || (form == 'l')) {
    if (((form == 'k') && !is_even(ncon_range.front() / 4)) ||
        ((form == 'l') && is_even(ncon_range.front() / 4)))
      ncon_range.front() += 4;

    inc += 6;
    form = 'h';
  }

  if (opts.long_form) {
    if (!opts.list_compounds) {
      fprintf(stdout,
              "                       Surfaces                       Edges\n");
      fprintf(stdout, "                       ------------------------------ "
                      "------------------------\n");
      fprintf(stdout, "%s  %s          %s %s %s %s %s\n\n", "Order", "n-icon",
              "Total", "Continuous", "Discontinuous", "Continuous",
              "Discontinuous");
    }
    else {
      fprintf(stdout,
              "                       Compound Parts   Total Surfaces\n");
      fprintf(stdout,
              "                       -------------------------------\n");
      fprintf(stdout, "%s  %s          %s            %s\n\n", "Order", "n-icon",
              "Parts", "Surfaces");
    }
  }

  int hit_count = 0;
  int model_count = 0;

  int last = 0;
  for (int ncon_order = ncon_range.front(); ncon_order <= ncon_range.back();
       ncon_order += inc) {
    // fprintf(stderr,"n = %d\n", ncon_order);

    bool point_cut = false;
    bool hybrid = false;
    bool info = false;

    if (form == 'p' || form == 'o')
      point_cut = true;
    else if (form == 's')
      point_cut = false;
    else
      hybrid = true;

    // last twist
    last = (int)floor((double)ncon_order / ((!is_even(ncon_order)) ? 2 : 4));
    if (hybrid)
      last++;

    bool none = true;

    if (opts.long_form)
      fprintf(stdout, "%-5d: ", ncon_order);
    else
      fprintf(stdout, "%d: ", ncon_order);

    surfaceData sd;

    int twist = (hybrid) ? 1 : 0;
    for (; twist <= last; twist++) {
      // fprintf(stderr,"twist = %d\n", twist);
      //  need list entry but...
      //  now that d>1 is allowed must not allow n/0
      if (!(d % ncon_order))
        continue;
      // don't allow d>n
      if (d > ncon_order)
        continue;

      ncon_info(ncon_order, d, point_cut, twist, hybrid, info, sd);

      if (opts.list_compounds) {
        // bypass digon cases
        if (ncon_order == 2 * d)
          continue;

        // compounds have to have more than one surface
        // compounds can't have more parts than surfaces
        if (sd.total_surfaces < opts.filter_surfaces.front())
          continue;

        opts.ncon_order = ncon_order;
        opts.twist = twist;
        opts.point_cut = point_cut;
        opts.hybrid = hybrid;

        Geometry geom;
        // hybrids of d mod 4 still has errors (e.g. n/d >= 144/48)
        // use direct measures
        if (hybrid && (d % 4 == 0))
          sd.compound_parts = ncon_subsystem(geom, opts);
        model_count++;
      }

      string d_part = ((d > 1) ? "/" : "") + ((d > 1) ? std::to_string(d) : "");
      if (!opts.list_compounds) {
        if ((sd.total_surfaces >= opts.filter_surfaces.front()) &&
            (sd.total_surfaces <= opts.filter_surfaces.back())) {
          if (!sd.case2 || (sd.case2 && !opts.filter_case2)) {
            if (!none) {
              if (opts.long_form)
                fprintf(stdout, "%-5d: ", ncon_order);
              else
                fprintf(stdout, ", ");
            }
            if (sd.nonchiral)
              buffer = "[" + std::to_string(ncon_order) + d_part + "+" +
                       std::to_string(twist) + "]";
            else if (sd.case2)
              buffer = "{" + std::to_string(ncon_order) + d_part + "+" +
                       std::to_string(twist) + "}";
            else
              buffer = "(" + std::to_string(ncon_order) + d_part + "+" +
                       std::to_string(twist) + ")";
            if (opts.long_form)
              fprintf(stdout, "%-15s %5d %10d %13d %10d %13d\n", buffer.c_str(),
                      sd.total_surfaces, sd.c_surfaces, sd.d_surfaces,
                      sd.c_edges, sd.d_edges);
            else
              fprintf(stdout, "%s", buffer.c_str());
            none = false;
            hit_count++;
          }
        }
      }
      else {
        if ((sd.compound_parts >= opts.filter_surfaces.front()) &&
            (sd.compound_parts <= opts.filter_surfaces.back())) {
          if (!none) {
            if (opts.long_form)
              fprintf(stdout, "%-5d: ", ncon_order);
            else
              fprintf(stdout, ", ");
          }
          if (sd.nonchiral)
            buffer = "[" + std::to_string(ncon_order) + d_part + "+" +
                     std::to_string(twist) + "]";
          else if (sd.case2)
            buffer = "{" + std::to_string(ncon_order) + d_part + "+" +
                     std::to_string(twist) + "}";
          else
            buffer = "(" + std::to_string(ncon_order) + d_part + "+" +
                     std::to_string(twist) + ")";
          if (opts.long_form) {
            fprintf(stdout, "%-15s %5d %19d\n", buffer.c_str(),
                    sd.compound_parts, sd.total_surfaces);
          }
          else
            fprintf(stdout, "%s", buffer.c_str());
          none = false;
          hit_count++;
        }
      }
    }

    if (none) {
      fprintf(stdout, "none");
      if (opts.long_form)
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
  }

  fprintf(
      stdout, "found %d %s\n", hit_count,
      (opts.list_compounds ? "compounds" : "models with multiple surfaces"));
  if (opts.list_compounds)
    fprintf(stdout, "processed %d models\n", model_count);
  fprintf(stdout, "\n");
}

int main(int argc, char *argv[])
{
  int ret = 0;

  ncon_opts opts;
  opts.process_command_line(argc, argv);

  if (opts.ncon_surf.length())
    surface_subsystem(opts);
  else {
    Geometry geom;
    ret = ncon_subsystem(geom, opts);

    opts.write_or_error(geom, opts.ofile);
  }

  return ret;
}
