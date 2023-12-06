/*
   Copyright (c) 2023, Roger Kaufman, Adrian Rossiter

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
   Name: off_sym.cc
   Description: Add visual symmetry elements to an OFF file as seen in antiview
   Project: Antiprism - http://www.antiprism.com
*/

#include "../base/antiprism.h"
#include "../src/help.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using std::pair;
using std::set;
using std::string;
using std::vector;

using namespace anti;

class off_sym_opts : public ProgramOpts {
public:
  string ofile;

  Trans3d trans_m;
  Geometry mod_geom;

  string hide_elems;              // elements to hide
  string sym_str;                 // for sub symmetry
  bool sym_list = false;          // list sub symmetries
  double sym_scale = 1.1;         // scale symmetry elements for sub-symmetries
  bool restore_alignment = false; // restore original alignment

  bool show_axes = false;
  bool show_mirrors = false;
  bool show_rotrefls = false;

  off_sym_opts() : ProgramOpts("off_sym") {}
  void process_command_line(int argc, char **argv);
  void usage();
};

void off_sym_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options] [input_files]

Add visual symmetry elements to an OFF file as seen in antiview.
Note that any off file viewer must be able to handle invisible elements
invisible is defined as rgba = (0,0,0,0). Elements should be hidden with
off_sys rather than the viewer because the symmetry elements are made of faces

Options
%s
  -H        Symmetry help and Symmetry Model help (from off_util -H help)
  -x <elms> hide model elements. The element string can include v, e, f or a
            to hide vertices, edges, faces, or a to remove all elements
  -s <syms> show symmetry elements. The element string can include
               x - rotation axes
               m - mirror planes
               r - rotation-reflection planes
               a - all elements (same as xmr)
  -y        align geometry with the standard alignment for a symmetry type,
            up to three comma separated parts: symmetry subgroup (Schoenflies
            notation) or 'full', conjugation type (integer), realignment
            (colon separated list of an integer then decimal numbers)
  -z        restore starting alignment (can undo conjugation alignment)
  -S        scale of symmetry elements for sub-symmetries (default: 1.1)
  -L        list symmetries, with conjugations and realignments
  -o <file> write output to file (default: write to standard output)

)",
          prog_name(), help_ver_text);
}

void off_sym_opts::process_command_line(int argc, char **argv)
{
  Status stat;
  opterr = 0;
  int c;
  vector<pair<char, char *>> args;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hHx:s:y:zS:Lo:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) { // Keep switch for consistency/maintainability
    case 'H':
      fprintf(stdout, "\n%s\n\n", help_symmetry);
      fprintf(stdout, "\n%s\n\n", help_sym_models);
      exit(0);

    default:
      args.push_back(pair<char, char *>(c, optarg));
    }
  }

  vector<string> ifiles;
  while (argc - optind)
    ifiles.push_back(string(argv[optind++]));

  if (!ifiles.size())
    ifiles.push_back("");

  // Append all input files
  for (auto &ifile : ifiles) {
    Geometry geom_arg;
    if (ifile != "null")
      read_or_error(geom_arg, ifile);
    mod_geom.append(geom_arg);
  }

  vector<double> nums;

  for (auto &arg : args) {
    c = arg.first;
    char *optarg = arg.second;
    switch (c) {
    case 'x':
      if (strspn(optarg, "vefa") != strlen(optarg))
        error(msg_str("elements to hide are '%s' must be from v, e, f, a",
                      optarg));

      hide_elems = optarg;
      break;

    case 's':
      if (strspn(optarg, "axmr") != strlen(optarg))
        error(msg_str(
            "symmetry elements to show are '%s' must be from a (all), x, m, r",
            optarg));
      else {
        show_axes = (strchr(optarg, 'x') || strchr(optarg, 'a'));
        show_mirrors = (strchr(optarg, 'm') || strchr(optarg, 'a'));
        show_rotrefls = (strchr(optarg, 'r') || strchr(optarg, 'a'));
      }
      break;

    case 'y': {
      Symmetry full_sym(mod_geom);

      Split parts(optarg, ",");
      if (parts.size() == 0 || parts.size() > 3)
        error("argument should have 1-3 comma separated parts", c);

      Symmetry sub_sym;
      if (strncmp(parts[0], "full", strlen(optarg)) == 0)
        sub_sym = full_sym;
      else if (!(stat = sub_sym.init(parts[0], Trans3d())))
        error(msg_str("sub-symmetry type: %s", stat.c_msg()), c);

      sym_str = parts[0];

      int sub_sym_conj = 0;
      if (parts.size() > 1 && !(stat = read_int(parts[1], &sub_sym_conj)))
        error(msg_str("sub-symmetry conjugation number: %s", stat.c_msg()), c);

      Symmetry sym;
      if (!(stat = full_sym.get_sub_sym(sub_sym, &sym, sub_sym_conj)))
        error(msg_str("sub-symmetry: %s", stat.c_msg()), c);

      if (parts.size() > 2 &&
          !(stat = sym.get_autos().set_realignment(parts[2])))
        error(msg_str("sub-symmetry realignment: %s", stat.c_msg()), c);

      trans_m = sym.get_autos().get_realignment() * sym.get_to_std() * trans_m;
      break;
    }

    case 'z':
      restore_alignment = true;
      break;

    case 'S':
      print_status_or_exit(read_double(optarg, &sym_scale), c);
      if (sym_scale <= 0)
        error("scale cannot be negative or zero", "s", c);
      break;

    case 'L':
      sym_list = true;
      break;

    case 'o':
      ofile = optarg;
      break;

    default:
      error("unknown command line error");
    }
  }
}

// the online viewer can't make faces invisible
// delete faces for now
// vertices must remain (unless a) so they must not be deleted
void filter(Geometry &geom, const char *elems)
{
  for (const char *p = elems; *p; p++) {
    switch (*p) {
    case 'v':
      Coloring(&geom).v_one_col(Color::invisible);
      break;
    case 'e':
      Coloring(&geom).e_one_col(Color::invisible);
      break;
    case 'f':
      Coloring(&geom).f_one_col(Color::invisible);
      break;
    case 'a':
      geom.clear_all();
      break;
    }
  }
}

void print_symmetry(const Geometry &geom)
{
  // from off_report
  GeometryInfo info(geom);

  fprintf(stderr, "[symmetry]\n");
  fprintf(stderr, "type = %s\n", info.get_symmetry_type_name().c_str());
  fprintf(stderr, "\n");

  fprintf(stderr, "[symmetry_subgroups]\n");
  const set<Symmetry> &subs = info.get_symmetry_subgroups();
  set<Symmetry>::const_iterator si;
  map<string, int> subsym_cnts;
  // added per poly_kscope
  map<string, int> subsym_free_rots;
  map<string, int> subsym_free_transls;
  for (si = subs.begin(); si != subs.end(); ++si) {
    subsym_cnts[si->get_symbol()]++;

    // added per poly_kscope
    Symmetry sub = (Symmetry)(*si);
    subsym_free_rots[si->get_symbol()] = sub.get_autos().num_free_rots();
    subsym_free_transls[si->get_symbol()] = sub.get_autos().num_free_transls();
  }

  map<string, int>::const_iterator mi;
  for (mi = subsym_cnts.begin(); mi != subsym_cnts.end(); ++mi) {
    fprintf(stderr, "%s,%d\t", mi->first.c_str(), mi->second);

    // added per poly_kscope
    int free_rots = subsym_free_rots[mi->first.c_str()];
    if (free_rots == 1)
      fprintf(stderr, " x axial rotation   ");
    else if (free_rots == 3)
      fprintf(stderr, " x full  rotation   ");
    int free_transls = subsym_free_transls[mi->first.c_str()];
    if (free_transls == 1)
      fprintf(stderr, " x axial translation");
    else if (free_transls == 2)
      fprintf(stderr, " x plane translation");
    else if (free_transls == 3)
      fprintf(stderr, " x space translation");
    fprintf(stderr, "\n");
  }

  fprintf(stderr, "\n");
}

Geometry make_sym_elems_geom(const Geometry &geom, const off_sym_opts &opts)
{
  DisplaySymmetry disp_sym;
  disp_sym.set_show_axes(opts.show_axes);
  disp_sym.set_show_mirrors(opts.show_mirrors);
  disp_sym.set_show_rotrefls(opts.show_rotrefls);

  Scene scen;
  SceneGeometry sc_geom;
  sc_geom.set_scene(&scen);
  sc_geom.set_sym(disp_sym);

  // if sub-symmetry, use sym_xxx model to build symmetry figure
  if (opts.sym_str.size() && opts.sym_str != "full") {
    Geometry geom_sub;
    geom_sub.read_resource("sym_" + opts.sym_str);
    sc_geom.set_geom(geom_sub);
  }
  else
    sc_geom.set_geom(geom);

  scen.add_geom(sc_geom);

  auto *sym = dynamic_cast<DisplaySymmetry *>(scen.get_geoms()[0].get_sym());
  return sym->get_disp_geom();
}

int main(int argc, char *argv[])
{
  off_sym_opts opts;
  opts.process_command_line(argc, argv);

  if (opts.sym_list) {
    print_symmetry(opts.mod_geom);
    exit(0);
  }

  // transforms from opts
  opts.mod_geom.transform(opts.trans_m);

  // generate symmetry figure
  Geometry sym_geom = make_sym_elems_geom(opts.mod_geom, opts);
  sym_geom.add_missing_impl_edges();
  Coloring(&sym_geom).v_one_col(Color::invisible);
  Coloring(&sym_geom).e_one_col(Color::invisible);

  // adjust symmetry figure when displaying sub-symmetry elements
  if (opts.sym_str.size()) {
    // remember center of model
    Vec3d cent_geom = opts.mod_geom.centroid();

    // resize for sub-symmetry, center geoms on origin
    // radius of model
    opts.mod_geom.transform(Trans3d::translate(-cent_geom));
    double radius_geom = GeometryInfo(opts.mod_geom).vert_dist_lims().max;

    // radius of symmetry elements
    sym_geom.transform(Trans3d::translate(-sym_geom.centroid()));
    double radius_sym = GeometryInfo(sym_geom).vert_dist_lims().max;

    // scale symmetry elements larger than model
    sym_geom.transform(
        Trans3d::scale(radius_geom / radius_sym * opts.sym_scale));

    // center geoms on original model centroid
    opts.mod_geom.transform(Trans3d::translate(cent_geom));
    sym_geom.transform(Trans3d::translate(cent_geom));
  }

  Geometry geom;

  // append model
  opts.mod_geom.add_missing_impl_edges();
  filter(opts.mod_geom, opts.hide_elems.c_str());
  geom.append(opts.mod_geom);

  // append symmetry figure
  geom.append(sym_geom);

  // if original alignment of input model is desired
  if (opts.restore_alignment)
    geom.transform(opts.trans_m.inverse());

  opts.write_or_error(geom, opts.ofile);

  return 0;
}
