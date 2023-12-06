/*
   Copyright (c) 2007,2023, Roger Kaufman

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
   Name: vr1tovr2.cc
   Description: Wrapper to give vrml1tovrml2.exe stdin and stdout support
   Project: Antiprism - http://www.antiprism.com
*/

#include "../base/antiprism.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace anti;

#define COMMAND_SIZE 512

class vr1tovr2_opts : public ProgramOpts {
public:
  string ifile;
  string ofile;

  bool pass = false;

  char command[COMMAND_SIZE];

  vr1tovr2_opts() : ProgramOpts("vr1tovr2") {}

  void process_command_line(int argc, char **argv);
  void usage();
};

void vr1tovr2_opts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options]

Convert vrml 1.0 to vrml 2.0 via vrml1tovrml2.exe (external program). If
input_file is not given the program reads from standard input.

Options
%s
  -p        pass any non-vrml 1.0 file to output unchanged
  -o <file> file name for output (otherwise prints to stdout)

)",
          prog_name(), help_ver_text);
}

void vr1tovr2_opts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  char c;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hpo:")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {

    case 'p':
      pass = true;
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

vector<string> read_file_to_mem(string file_name, const vr1tovr2_opts &opts)
{
  string str;
  vector<string> buf;

  FILE *ifile;
  if (file_name == "" || file_name == "-") {
    ifile = stdin;
    file_name = "stdin";
  }
  else {
    ifile = fopen(file_name.c_str(), "r");
    if (!ifile) {
      perror("read_file_to_mem fopen() error");
      opts.error(msg_str("read_file_to_mem could not open file \'%s\'",
                         file_name.c_str()));
    }
  }

  char *line = 0;
  while (read_line(ifile, &line) == 0) {
    buf.push_back(line);
    free(line);
  }

  if (file_name != "stdin")
    fclose(ifile);

  return buf;
}

void write_mem_to_file(string file_name, vector<string> *memfile,
                       vr1tovr2_opts &opts)
{
  FILE *ofile;
  if (file_name == "" || file_name == "-") {
    ofile = stdout;
    file_name = "stdout";
  }
  else {
    ofile = fopen(file_name.c_str(), "w");
    if (!ofile) {
      perror("write_mem_to_file fopen() error");
      opts.error(msg_str("write_mem_to_file could not open file \'%s\'",
                         file_name.c_str()));
    }
  }

  // create temporary input file
  for (unsigned int i = 0; i < memfile->size(); i++)
    fprintf(ofile, "%s\n", (*memfile)[i].c_str());

  if (file_name != "stdout")
    fclose(ofile);
}

int main(int argc, char *argv[])
{
  vr1tovr2_opts opts;
  opts.process_command_line(argc, argv);

  // read vrml text into array
  vector<string> vrmltxt;
  vrmltxt = read_file_to_mem(opts.ifile, opts);

  // check ahead of time to see if file is valid
  string vr1_str = "#VRML V1.0 ascii";
  if (vrmltxt[0].compare(0, vr1_str.size(), vr1_str)) {
    if (!opts.pass) {
      opts.error(
          msg_str("first line of input file is not '%s'", vr1_str.c_str()));
    }
    else {
      // pass the input to output
      write_mem_to_file(opts.ofile, &vrmltxt, opts);
      exit(0);
    }
  }

  // write out tmp file for vrml1tovrml2
  char *tmpname1;
  if ((tmpname1 = tempnam(NULL, "wrl1-")) == NULL)
    opts.error("Cannot create a unique filename");
  else
    write_mem_to_file(tmpname1, &vrmltxt, opts);

  vrmltxt.clear();

  char *tmpname2;
  if ((tmpname2 = tempnam(NULL, "wrl2-")) == NULL)
    opts.error("Cannot create a unique filename");

  snprintf(opts.command, COMMAND_SIZE, "vrml1tovrml2.exe %s %s", tmpname1,
           tmpname2);
  int ret = 0;
  ret = system(opts.command);

  remove(tmpname1);

  // read temporary result into array
  vrmltxt = read_file_to_mem(tmpname2, opts);
  if (!vrmltxt.size())
    opts.error("can vrml1tovrml2.exe be found in your system path?");
  else {
    // The attribute of output file becomes read only within vrml1tovrml2. undo
    // that.
    snprintf(opts.command, COMMAND_SIZE, "attrib -r %s", tmpname2);
    ret = system(opts.command);

    remove(tmpname2);
  }

  // So the the output file can always be written
  if (opts.ofile != "") {
    // if the output file is read only from previous run of vrml1tovrml2,
    // then undo that.
    snprintf(opts.command, COMMAND_SIZE, "attrib -r %s", opts.ofile.c_str());
    ret = system(opts.command);
  }

  // write out result from vrml1tovrml2
  write_mem_to_file(opts.ofile, &vrmltxt, opts);

  vrmltxt.clear();

  return ret;
}
