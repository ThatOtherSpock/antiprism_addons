/*
   Copyright (c) 2006-2024, Roger Kaufman

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
   Name: vrml2off.cc
   Description: convert vrml to an off file
   Project: Antiprism - http://www.antiprism.com
*/

#include "vrml2off.h"
#include "../base/antiprism.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using std::string;

using namespace anti;

class vrml2off_opts : public ProgramOpts {
public:
  string ifile;
  string ofile;
  string tfile; // hedron text file

  bool unitize = false;
  bool merge_nets = false;

  // hedron
  bool estimate_colors = false;
  bool detect_rhombi = false;
  bool detect_star_polygons = false;
  bool exclude_coordinates = false;
  bool force_transparent = false;

  int sig_digits = DEF_SIG_DGTS; // significant digits output (system default)

  double eps = anti::epsilon;

  vrml2off_opts() : ProgramOpts("vrml2off") {}

  void process_command_line(int argc, char **argv);
  void usage();
};

void vrml2off_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options] [input_file]

vrml2off will convert a Hedron or Stella generated vrml file to off format.
It does not work with Antiprism vrml.

Options
%s
  -s        adjusts average edge length to 1 (unless -n is used)
  -m        in case of multiple nets, merge them into one
  -d <dgts> number of significant digits (default %d) or if negative
            then the number of digits after the decimal point
  -o <file> file name for output (otherwise prints to stdout)
  
Hedron Options
  -f <file> outputs Hedron text format to file
  -c        estimates colors from OFF file
  -r        detect rhombi. D parameter added if found
  -p        detect star polygons
  -x        exclude coordinates
  -t        force all faces transparent

)",
          prog_name(), help_ver_text, DEF_SIG_DGTS);
}

void vrml2off_opts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;

  Split parts;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hsmf:crpxto:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 's':
      unitize = true;
      break;

    case 'm':
      merge_nets = true;
      break;

    case 'f':
      tfile = optarg;
      break;

    case 'c':
      estimate_colors = true;
      break;

    case 'r':
      detect_rhombi = true;
      break;

    case 'p':
      detect_star_polygons = true;
      break;

    case 'x':
      exclude_coordinates = true;
      break;

    case 't':
      force_transparent = true;
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
}

bool process_vrml(string &file_name, Geometry &geom, const vrml2off_opts &opts,
                  string *error_msg = nullptr)
{
  FILE *ifile;
  if (file_name == "" || file_name == "-") {
    ifile = stdin;
    file_name = "stdin";
  }
  else {
    ifile = fopen(file_name.c_str(), "r");
    if (!ifile) {
      if (error_msg)
        *error_msg =
            msg_str("could not open input file '%s'", file_name.c_str());
      return false;
    }
  }

  // Stage 1 - find color table
  const int max_width = 50000;
  char text_line[max_width];
  char *pch, *pch2, *ptok;
  char parse_key[] = " ,.";
  char parse_key_double[] = " ,";

  vector<Color> col_table;
  int col_table_count = 0;

  pch = NULL;
  while (fgets(text_line, max_width, ifile) != NULL && pch == NULL)
    pch = strstr(text_line, "color [");

  if (pch != NULL) {
    pch = NULL;
    col_table_count = 0;
    while (pch == NULL) {
      pch = strstr(text_line, "]");
      if (pch == NULL) {
        // nullify any comments within the data
        ptok = strchr(text_line, '#');
        if (ptok != NULL)
          *ptok = '\0';

        int c[3] = {0};
        int j = 0;
        ptok = strtok(text_line, parse_key_double);
        while (ptok != NULL) {
          double tmp_double = atof(ptok);
          c[j] = (int)floor((tmp_double * 255.0) + 0.5);
          j++;
          if (j > 2) {
            col_table.push_back(Color(c[0], c[1], c[2]));
            j = 0;
            col_table_count++;
          }
          ptok = strtok(NULL, parse_key_double);
        }
        pch2 = fgets(text_line, max_width, ifile);
        pch2 = pch2; // handle warning of unused variable
      }
    }
  }

  // Stage 2 - find net(s)
  int num_edges = 0;
  int num_vertices = 0;
  int num_faces = 0;
  int num_face[max_net_instances];
  int net_instance = 0;

  bool ignore_next_net = false;
  bool ignore_next_col = false;
  int col_table_count_diffuse = 0;

  fseek(ifile, 0, SEEK_SET);
  pch = NULL;
  net_instance = -1;
  num_vertices = 0;
  // In Hedron, ignore next IndexedFaceSet after FRAME
  while (fgets(text_line, max_width, ifile) != NULL) {
    pch = strstr(text_line, "DEF FRAME");
    if (pch != NULL)
      ignore_next_net = true;

    pch = strstr(text_line, "IndexedLineSet");
    if (pch != NULL)
      ignore_next_col = true;

    pch = strstr(text_line, "IndexedFaceSet");
    if (pch != NULL)
      ignore_next_col = false;

    if (pch != NULL && ignore_next_net) {
      pch = NULL;
      ignore_next_net = false;
    }

    int net_count_tmp = 0;
    if (pch != NULL) {
      pch = NULL;
      while (fgets(text_line, max_width, ifile) != NULL && pch == NULL)
        pch = strstr(text_line, "coordIndex");

      net_count_tmp = 0;
      net_instance++;

      if (net_instance > max_net_instances)
        opts.error(
            msg_str("maximum number of faces sets is %d", max_net_instances));

      pch = NULL;
      while (pch == NULL) {
        pch = strstr(text_line, "]");
        if (pch == NULL) {
          // nullify last comma
          ptok = strrchr(text_line, ',');
          if (ptok != NULL)
            *ptok = '\0';

          ptok = strtok(text_line, parse_key);
          int j = 1;
          while (ptok != NULL) {
            net[net_instance][net_count_tmp].element[j] = atoi(ptok);
            if (num_vertices < net[net_instance][net_count_tmp].element[j])
              num_vertices = net[net_instance][net_count_tmp].element[j];
            ptok = strtok(NULL, parse_key);
            j++;
          }
          net[net_instance][net_count_tmp].element[0] = j - 2;
          net_count_tmp++;
        }
        pch2 = fgets(text_line, max_width, ifile);
      }

      num_face[net_instance] = net_count_tmp;
      num_faces += num_face[net_instance];

      if (num_face[net_instance] > max_net_size)
        opts.error(msg_str("maximum number of faces is %d", max_net_size));

      // find color index for stella
      // while( fgets(text_line, max_width, ifile) != NULL && pch == NULL )
      pch = strstr(text_line, "colorIndex");
      if (pch != NULL) {
        pch2 = fgets(text_line, max_width, ifile);
        ptok = strtok(text_line, parse_key);
        int color_index = 0;
        while (ptok != NULL) {
          net[net_instance][color_index++].color = atoi(ptok);
          ptok = strtok(NULL, parse_key);
        }
      }
    }

    // Build Color Table for Compounds
    pch = strstr(text_line, "diffuseColor");
    if (pch != NULL && !ignore_next_col) {
      // nullify any comments within the data
      ptok = strchr(text_line, '#');
      if (ptok != NULL)
        *ptok = '\0';

      int c[3] = {0};
      col_table_count_diffuse = 0;
      int j = 0;
      ptok = strtok(text_line, parse_key_double);
      ptok = strtok(NULL, parse_key_double);
      while (ptok != NULL) {
        double tmp_double = atof(ptok);
        c[j] = (int)floor((tmp_double * 255.0) + 0.5);
        j++;
        if (j > 2) {
          col_table.push_back(Color(c[0], c[1], c[2]));
          j = 0;
          col_table_count++;
          col_table_count_diffuse++;
        }
        ptok = strtok(NULL, parse_key_double);
      }

      // Guard against all Black solid (usually edge lines)
      j = col_table_count - 1;
      if (col_table_count_diffuse == 1 && (c[0] + c[1] + c[2] == 0))
        col_table_count--;

      for (int i = 0; i < net_count_tmp; i++)
        net[net_instance][i].color = col_table_count - 1;
    }
  }

  if (num_vertices == 0)
    opts.error("number of Vertices is Zero");

  num_vertices += 1;
  num_edges = num_vertices + num_faces - 2;

  // Stage 3 - find coordinate sections
  int max_precision = 0;
  bool found_minus = false;
  bool ignore_next_coord = false;

  double coords[3] = {0};
  fseek(ifile, 0, SEEK_SET);
  pch = NULL;
  int i = 0;
  while (fgets(text_line, max_width, ifile) != NULL) {
    pch = strstr(text_line, "IndexedLineSet");
    if (pch != NULL)
      ignore_next_coord = true;

    pch = strstr(text_line, "Coordinate");
    if (pch != NULL && ignore_next_coord) {
      pch = NULL;
      ignore_next_coord = false;
    }

    if (pch != NULL) {
      // The next two lines are non-numeric
      pch2 = fgets(text_line, max_width, ifile);
      pch2 = fgets(text_line, max_width, ifile);

      pch = NULL;
      while (pch == NULL) {
        pch = strstr(text_line, "]");
        if (pch == NULL) {
          int j = 0;
          ptok = strtok(text_line, parse_key_double);
          while (ptok != NULL) {
            double tmp_double = atof(ptok);
            pch = strchr(ptok, '-');
            if (pch != NULL)
              found_minus = true;
            if (max_precision < (int)strlen(ptok))
              max_precision = (int)strlen(ptok);
            coords[j] = tmp_double;
            j++;
            if (j > 2) {
              // fprintf(stderr, "%d = %lf %lf %lf\n", i, coords[i].c[0],
              // coords[1], coords[2]);
              geom.add_vert(Vec3d(coords[0], coords[1], coords[2]));
              j = 0;
              i++;
            }
            ptok = strtok(NULL, parse_key_double);
          }

          pch2 = fgets(text_line, max_width, ifile);
        }
      }
    }
  }

  max_precision -= 2;
  if (found_minus)
    max_precision -= 1;

  if (i > num_vertices) {
    num_vertices = i;
    num_edges = num_vertices + num_faces - 2;
  }

  // Stage 4 - write out off file
  // fprintf( fp2, "#\n# File %s converted to OFF format by vrml2txt %s\n#\n",
  // in_file, version );

  // faces
  int off_vertex_adj = 0;
  int off_max_vertex = 0;
  for (int k = 0; k <= net_instance; k++) {
    for (int i = 0; i < num_face[k]; i++) {
      vector<int> face;
      off_max_vertex = 0;
      for (int j = 0; j <= net[k][i].element[0]; j++) {
        // no longer need face size
        if (j == 0)
          continue;
        face.push_back(net[k][i].element[j] + off_vertex_adj);

        if (off_max_vertex < net[k][i].element[j])
          off_max_vertex = net[k][i].element[j];
      }

      Color c = Color();
      if (col_table_count != 0) {
        int l = net[k][i].color;
        c = col_table[l];
      }

      geom.add_face(face, c);
    }
    off_vertex_adj += off_max_vertex + 1;
  }

  if ((net_instance > 0) && !opts.merge_nets)
    opts.warning("MULTIPLE NETS FOUND. Use -m option to merge them if desired");

  fprintf(stderr, "vertices: %d edges: %d faces: %d\n", num_vertices, num_edges,
          num_faces);

  if (file_name != "stdin")
    fclose(ifile);

  return true;
}

string estimated_color(Color col)
{
  string color;
  int v[3];

  for (unsigned int i = 0; i < 3; i++)
    v[i] = (col[i] <= 128) ? 0 : 255;

  // Special case for orange
  if (v[0] == 255 && (col[1] >= 64 && col[1] <= 192) && v[2] == 0)
    color = "a";
  else if (v[0] == 0 && v[1] == 0 && v[2] == 255)
    color = "b";
  else
    // Special case for green. "0,128,0" is exact green would become 0 0 0
    // Make more dark greens be g instead of using 0 255 0.
    if (v[0] == 0 && col[1] >= 64 && v[2] == 0)
      color = "g";
    else
      // Hedron has no black so specify cyan
      if (v[0] == 0 && v[1] == 0 && v[2] == 0)
        color = "c";
      else if (v[0] == 0 && v[1] == 255 && v[2] == 255)
        color = "c";
      else if (v[0] == 255 && v[1] == 0 && v[2] == 0)
        color = "r";
      else if (v[0] == 255 && v[1] == 0 && v[2] == 255)
        color = "m";
      else if (v[0] == 255 && v[1] == 255 && v[2] == 0)
        color = "y";
      else if (v[0] == 255 && v[1] == 255 && v[2] == 255)
        color = "w";
      else
        color = "\0";

  return (color);
}

bool is_square(Geometry &geom, const int &face_idx, const double &eps)
{
  const vector<Vec3d> &verts = geom.verts();
  const vector<vector<int>> &faces = geom.faces();
  vector<int> face = faces[face_idx];

  Vec3d P1 = verts[face[0]];
  Vec3d P2 = verts[face[2]];
  double diag1 = (P2 - P1).len();

  P1 = verts[face[1]];
  P2 = verts[face[3]];
  double diag2 = (P2 - P1).len();

  return (double_eq(diag1, diag2, eps));
}

int detect_star_polygon(Geometry &geom, const int &face_idx)
{
  const vector<Vec3d> &verts = geom.verts();
  const vector<vector<int>> &faces = geom.faces();
  vector<int> face = faces[face_idx];

  Vec3d v1 = verts[face[1]] - verts[face[0]];
  Vec3d v2 = verts[face[1]] - verts[face[2]];
  double angle = acos(safe_for_trig(vdot(v1, v2) / (v1.len() * v2.len())));
  angle = rad2deg(angle);

  int star = 1;
  double m = face.size();
  for (double n = 2; n < m / 2; n++) {
    if ((int)(m / n) * n != m && gcd((int)m, (int)n) == 1) {
      double sp = 180 * (1 - 2 * n / m);
      // angle allowed plus/minus 2 degrees slop
      if (fabs(sp - angle) <= 2) {
        star = (int)n;
        break;
      }
    }
  }
  return (star);
}

string Vtxt(const Vec3d &v, const int &dgts)
{
  char buf[128];
  if (dgts > 0)
    snprintf(buf, 128, "%.*g, %.*g, %.*g,", dgts, v[0], dgts, v[1], dgts, v[2]);
  else
    snprintf(buf, 128, "%.*f, %.*f, %.*f,", -dgts, v[0], -dgts, v[1], -dgts,
             v[2]);
  return buf;
}

void print_hedron_txt(FILE *ofile, Geometry &geom, const vrml2off_opts &opts)
{
  const vector<vector<int>> &faces = geom.faces();
  const vector<Vec3d> &verts = geom.verts();

  fprintf(ofile, "{\n");

  for (unsigned int i = 0; i < faces.size(); i++) {
    // fprintf(ofile, "");

    if (opts.detect_rhombi && faces[i].size() == 4 &&
        !is_square(geom, (int)i, opts.eps))
      fprintf(ofile, "D");

    if (opts.estimate_colors) {
      Color col = geom.colors(FACES).get((int)i);
      if (col.is_value())
        fprintf(ofile, "%s", estimated_color(col).c_str());
    }

    if (opts.force_transparent) {
      fprintf(ofile, "t");
    }

    for (unsigned int j = 0; j < faces[i].size(); j++)
      fprintf(ofile, "%d,", faces[i][j]);

    int star = 1;
    if (opts.detect_star_polygons && faces[i].size() > 4)
      star = detect_star_polygon(geom, (int)i);
    fprintf(ofile, "-%d,\n", star);
  }

  if (!opts.exclude_coordinates) {
    fprintf(ofile, "(\n");

    for (unsigned int i = 0; i < verts.size(); i++)
      fprintf(ofile, "%s\n", Vtxt(verts[i], opts.sig_digits).c_str());

    fprintf(ofile, ")\n");
  }

  fprintf(ofile, "}\n");
}

void unitize_edges(Geometry &geom)
{
  GeometryInfo info(geom);
  if (info.num_iedges() > 0) {
    double val = info.iedge_length_lims().sum / info.num_iedges();
    geom.transform(Trans3d::scale(1 / val));
  }
}

int main(int argc, char *argv[])
{
  vrml2off_opts opts;
  opts.process_command_line(argc, argv);

  Geometry geom;

  string error_msg;

  // obj is enough like OFF that it can be parsed and converted in line
  if (!process_vrml(opts.ifile, geom, opts, &error_msg))
    if (!error_msg.empty())
      opts.error(error_msg);

  if (opts.unitize)
    unitize_edges(geom);

  if (opts.tfile != "") {
    FILE *hfile = fopen(opts.tfile.c_str(), "w");
    if (hfile == 0)
      opts.error("could not open output file \'" + opts.tfile + "\'");

    print_hedron_txt(hfile, geom, opts);

    fclose(hfile);
  }

  opts.write_or_error(geom, opts.ofile);

  return 0;
}
