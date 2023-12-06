/*
   Copyright (c) 2014-2020, Roger Kaufman

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
   Name: alt_color.cc
   Description: extract coordinates from an OFF file
   Project: Antiprism - http://www.antiprism.com
*/

#include "../base/antiprism.h"
#include "../src/color_common.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace anti;

class alt_color_opts : public ProgramOpts {
public:
  string ifile;
  string ofile;

  // maps are managed, set no map
  OffColor off_color = OffColor("");

  int opacity[3] = {-1, -1, -1}; // transparency from 0 to 255, for v,e,f

  double eps = anti::epsilon;

  alt_color_opts() : ProgramOpts("alt_color") {}

  void process_command_line(int argc, char **argv);
  void usage();
};

void alt_color_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options] [input_file]

Alternate Color Methods.

Options
%s
  -l <lim>  minimum distance for unique vertex locations as negative exponent
               (default: %d giving %.0e)
  -o <file> write output to file (default: write to standard output)

Coloring Options (run 'off_util -H color' for help on color formats)
(run 'off_color -h' for help on other color options). Lowercase letters color
using index numbers and uppercase letters color using color values.
keyword: none - sets no color
  -F <col>  color the faces according to: (default: none)
               h - face/face connection count (model is altered with kis)
               b - color faces by convexity using all dihedral angles
  -E <col>  color the edges according to: (default: none)
               b - color edges by convexity of dihedral angle
  -V <col>  color the vertices: (default: none)
  -T <t,e>  transparency. from 0 (invisible) to 255 (opaque). element is any
            or all of, v - vertices, e - edges, f - faces, a - all (default: f)
  -m <maps> a comma separated list of color maps used to transform color
            indexes (default: colorful), a part consisting of letters from
            v, e, f, selects the element types to apply the map list to
            (default 'vef'). use map name of 'index' to output index numbers
               colorful:   red,darkorange1,yellow,darkgreen,cyan,blue,magenta,
                           white,gray50,black
               convexity:  white,gray50,gray25 (for -F b, -E b)
)",
          prog_name(), help_ver_text, int(-log(::epsilon) / log(10) + 0.5),
          ::epsilon);
}

void alt_color_opts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;
  int num;

  Split parts;
  Color col;
  vector<string> map_files;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hV:E:F:T:m:l:o:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'V':
      if (col.read(optarg)) {
        off_color.set_v_col(col);
        break;
      }
      parts.init(optarg, ",");
      if (off_color.v_op_check((char *)parts[0]))
        off_color.set_v_col_op(*parts[0]);
      else
        error("invalid coloring", c);

      if (!((strchr("sS", off_color.get_v_col_op()) && parts.size() < 4) ||
            parts.size() < 2))
        error("too many comma separated parts", c);

      if (strchr("sS", off_color.get_v_col_op()))
        off_color.set_v_sub_sym(strlen(optarg) > 2 ? optarg + 2 : "");
      break;

    case 'E':
      if (col.read(optarg)) {
        off_color.set_e_col(col);
        break;
      }
      parts.init(optarg, ",");
      if (off_color.e_op_check((char *)parts[0], "b"))
        off_color.set_e_col_op(*parts[0]);
      else
        error("invalid coloring", c);

      if (!((strchr("sS", off_color.get_e_col_op()) && parts.size() < 4) ||
            parts.size() < 2))
        error("too many comma separated parts", c);

      if (strchr("sS", off_color.get_e_col_op()))
        off_color.set_e_sub_sym(strlen(optarg) > 2 ? optarg + 2 : "");
      break;

    case 'F':
      if (col.read(optarg)) {
        off_color.set_f_col(col);
        break;
      }
      parts.init(optarg, ",");
      if (off_color.f_op_check((char *)parts[0], "bh"))
        off_color.set_f_col_op(*parts[0]);
      else
        error("invalid coloring", c);

      if (!((strchr("sS", off_color.get_f_col_op()) && parts.size() < 4) ||
            parts.size() < 2))
        error("too many comma separated parts", c);

      if (strchr("sS", off_color.get_f_col_op()))
        off_color.set_f_sub_sym(strlen(optarg) > 2 ? optarg + 2 : "");
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

  if (argc - optind > 1)
    error("too many arguments");

  if (argc - optind == 1)
    ifile = argv[optind];

  // set all maps in list
  for (unsigned int i = 0; i < map_files.size(); i++)
    print_status_or_exit(read_colorings(off_color.clrngs, map_files[i].c_str()),
                         'm');

  // fill in missing maps
  string default_map_name = "colorful";
  for (unsigned int i = 0; i < 3; i++) {
    string map_name = default_map_name;
    // if map is already set, skip
    if (off_color.clrngs[i].get_cmaps().size())
      continue;
    if (i == EDGES) {
      char op = off_color.get_e_col_op();
      if (op && strchr("bB", op))
        map_name = "convexity";
    }
    // faces
    else if (i == FACES) {
      char op = off_color.get_f_col_op();
      if (op && strchr("bB", op))
        map_name = "convexity";
    }

    off_color.clrngs[i].add_cmap(colormap_from_name(map_name.c_str()));
  }
}

void alt_color(Geometry &geom, alt_color_opts &opts)
{
  // color edges first in case faces_by_connection is called
  char op = opts.off_color.get_e_col_op();
  if (op && strchr("bB", op))
    color_edges_by_dihedral(geom, opts.off_color.clrngs[1], (op == 'B'),
                            opts.eps);

  op = opts.off_color.get_f_col_op();
  if (op && strchr("hH", op))
    // does off_color_main()
    color_faces_by_connection_vef(geom, opts.off_color);
  else {
    // process other options as normal
    if (op && strchr("bB", op))
      color_faces_by_convexity(geom, opts.off_color.clrngs[2], (op == 'B'),
                               opts.eps);

    Status stat;
    if (!(stat = opts.off_color.off_color_main(geom)))
      opts.error(stat.msg());
  }

  // apply all element transparencies
  apply_transparencies(geom, opts.opacity);
}

int main(int argc, char *argv[])
{
  alt_color_opts opts;
  opts.process_command_line(argc, argv);

  Geometry geom;
  opts.read_or_error(geom, opts.ifile);

  alt_color(geom, opts);

  opts.write_or_error(geom, opts.ofile);

  return 0;
}
