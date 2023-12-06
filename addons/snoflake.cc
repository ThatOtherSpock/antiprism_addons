/*
   Copyright (c) 2007-2021, Roger Kaufman

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
   Name: snoflake.cc
   Description: Makes Snowflake like patterns
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

class sn_opts : public ProgramOpts {
public:
  string ofile;
  string cfile;

  int spokes = 6;
  double ratio = (1 + sqrt(5)) / 2 - 1;
  double branch_angle = 60.0;
  double stopping_segment = 0;
  int recursion_levels_max = 5;

  OffColor off_color = OffColor("colorful");

  int opacity[3] = {-1, -1, -1}; // transparency from 0 to 255, for v,e,f

  sn_opts() : ProgramOpts("snoflake") {}

  void process_command_line(int argc, char **argv);
  void usage();
};

void sn_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options]

Generates Snow Flake like models

Options
%s
  -s <int>  spokes (default: 6)
  -b <ang>  branch angle (default: 60.0)
  -r <rat>  ratio (default: Phi (~0.6180339887))
  -g <seg>  stopping segment size (default 0)
              -g 0 give all control to -R recursion limit
  -R <int>  maximum allowable recurion levels (default: 5)
  -o <file> file name for output (otherwise prints to stdout)

Coloring Options
  -E <col>  color the edges according to: (default: white)
               u - unique color
               s - symmetric coloring [,sub_group,conj_type]
  -V <col>  color the vertices: (default: e)
               e - color with average adjacent edge color
               u - unique color
               s - symmetric coloring [,sub_group,conj_type]
  -T <t,e>  transparency. from 0 (invisible) to 255 (opaque). element is any
            or all of, v - vertices, e - edges, a - all (default: a)
  -m <maps> a comma separated list of color maps used to transform color
            indexes (default: colorful), a part consisting of letters from
            v, e, selects the element types to apply the map list to
            (default 've'). use map name of 'index' to output index numbers
               colorful:   red,darkorange1,yellow,darkgreen,cyan,blue,magenta,
                           white,gray50,black

)",
          prog_name(), help_ver_text);
}

void sn_opts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  char c;
  int num;

  Split parts;
  Color col;

  off_color.set_e_col(Color(255, 255, 255)); // white
  off_color.set_v_col_op('e');

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hs:b:r:g:R:V:E:T:m:o:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {

    case 's':
      print_status_or_exit(read_int(optarg, &spokes), c);
      if (spokes < 1)
        error("spokes must be greater than 0", c);
      break;

    case 'b':
      print_status_or_exit(read_double(optarg, &branch_angle), c);
      break;

    case 'r':
      print_status_or_exit(read_double(optarg, &ratio), c);
      if (ratio <= 0)
        error("ratio must be greater than 0", c);
      break;

    case 'g':
      print_status_or_exit(read_double(optarg, &stopping_segment), c);
      if (stopping_segment < 0)
        error("stopping_segment must be greater than or equal to 0", c);
      break;

    case 'R':
      print_status_or_exit(read_int(optarg, &recursion_levels_max), c);
      if (recursion_levels_max < 0)
        error("maximum recursion levels must be greater than or equal to 0", c);
      break;

    case 'V':
      if (col.read(optarg)) {
        off_color.set_v_col(col);
        break;
      }
      parts.init(optarg, ",");
      if (off_color.v_op_check((char *)parts[0], "eus"))
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
      if (off_color.e_op_check((char *)parts[0], "us"))
        off_color.set_e_col_op(*parts[0]);
      else
        error("invalid coloring", c);

      if (!((strchr("sS", off_color.get_e_col_op()) && parts.size() < 4) ||
            parts.size() < 2))
        error("too many comma separated parts", c);

      if (strchr("sS", off_color.get_e_col_op()))
        off_color.set_e_sub_sym(strlen(optarg) > 2 ? optarg + 2 : "");
      break;

    case 'T': {

      int parts_sz = parts.init(optarg, ",");
      if (parts_sz > 2)
        error("the argument has more than 2 parts", c);

      print_status_or_exit(read_int(parts[0], &num), c);
      if (num < 0 || num > 255)
        error("face transparency must be between 0 and 255", c);

      // if only one part, apply to edges and verts as default
      if (parts_sz == 1) {
        opacity[EDGES] = num;
        opacity[VERTS] = num;
      }
      else if (parts_sz > 1) {
        if (strspn(parts[1], "vea") != strlen(parts[1]))
          error(msg_str("transparency elements are '%s' must be any or all "
                        "from  v, e, a",
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
      print_status_or_exit(read_colorings(off_color.clrngs, optarg), c);
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
}

void tree(Geometry &geom, const sn_opts &opts, int base_idx, double angle,
          double radius, int recursion_level)
{
  const vector<Vec3d> &verts = geom.verts();

  // fprintf(stderr,"in tree level %d  base = %d, recursion_level, base_idx);

  recursion_level++;
  if (recursion_level > opts.recursion_levels_max)
    return;

  if (opts.stopping_segment != 0)
    if ((opts.stopping_segment < 1.0 && radius < opts.stopping_segment) ||
        (opts.stopping_segment > 1.0 && radius > opts.stopping_segment))
      return;

  geom.add_vert(
      Vec3d(cos(deg2rad(angle)) * radius, sin(deg2rad(angle)) * radius, 0) +
      verts[base_idx]);
  int stem_idx = verts.size() - 1;

  vector<int> edge = make_edge(base_idx, stem_idx);
  // fprintf(stderr,"edge %d %d written length %lf, base_idx, stem_idx, radius);
  geom.add_edge(edge);

  radius *= opts.ratio;

  // stem
  tree(geom, opts, stem_idx, angle, radius, recursion_level);

  // branches
  tree(geom, opts, stem_idx, angle + opts.branch_angle, radius,
       recursion_level);
  tree(geom, opts, stem_idx, angle - opts.branch_angle, radius,
       recursion_level);
}

void snoflake(Geometry &geom, const sn_opts &opts)
{
  const vector<Vec3d> &verts = geom.verts();

  geom.add_vert(Vec3d(0, 0, 0));
  int hub_idx = verts.size() - 1;

  double arc = 360.0 / opts.spokes;
  double radius = 1.0;

  int recursion_level = -1;
  double angle = 90.0;
  for (int i = 0; i < opts.spokes; i++) {
    tree(geom, opts, hub_idx, angle, radius, recursion_level);
    angle += arc;
  }
}

int main(int argc, char *argv[])
{
  sn_opts opts;
  opts.process_command_line(argc, argv);

  Geometry geom;

  snoflake(geom, opts);

  // colors done by class
  Status stat;
  if (!(stat = opts.off_color.off_color_main(geom)))
    opts.error(stat.msg());

  // apply all element transparencies
  apply_transparencies(geom, opts.opacity);

  opts.write_or_error(geom, opts.ofile);

  return 0;
}
