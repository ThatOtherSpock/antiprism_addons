/*
   Copyright (c) 2003, Adrian Rossiter

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
   Name: rep_print.h
   Description: information from OFF file - print functions 
   Project: Antiprism - http://www.antiprism.com
*/

#ifndef REP_PRINT_H
#define REP_PRINT_H

#include <stdio.h>
#include "../base/antiprism.h"

class rep_printer: public geom_info
{
   private:
      int sig_dgts;
      char *d2s(char *buf, double d) { return dtostr(buf, d, sig_dgts); }
      char *v2s(char *buf, vec3d v) { return vtostr(buf, v, " ", sig_dgts); }
      char *vidx2s(char *buf, int idx)
         { return idx2s(buf, idx, num_verts()-extra_v_sz); }
      char *eidx2s(char *buf, int idx)
         { return idx2s(buf, idx, num_edges()-extra_e_sz); }
      char *fidx2s(char *buf, int idx)
         { return idx2s(buf, idx, num_faces()-extra_f_sz); }
     
      string sub_sym_str;

      char *idx2s(char *buf, int idx, int extra_sz);
      int extra_v_sz;
      int extra_e_sz;
      int extra_f_sz;
      FILE *ofile;

   public:
      rep_printer(geom_if &geom, FILE *outfile=stdout) :
         geom_info(geom), extra_v_sz(0), extra_e_sz(0), extra_f_sz(0), 
         ofile(outfile)
         { set_sig_dgts(); }
      
      void set_sig_dgts(int dgts=8) { sig_dgts=dgts; }
      bool set_sub_symmetry(const string &sub_sym, char *errmsg);

      void extra_elems_added(int v_sz, int e_sz, int f_sz)
         { extra_v_sz += v_sz; extra_e_sz += e_sz; extra_f_sz += f_sz; }
      
      void general_sec();
      void faces_sec();
      void edges_sec();
      void angles_sec();
      void solid_angles_sec();
      void distances_sec();
      void symmetry();

      void face_sides_cnts();
      void vert_order_cnts();
      void vert_heights_cnts();
      void solid_angles_cnts();
      void face_angles_cnts();
      void edge_lengths_cnts();
      void dihedral_angles_cnts();
      void sym_orbit_cnts();

      void v_index(int v_idx);
      void v_coords(int v_idx);
      void v_neighbours(int v_idx);
      void v_face_idxs(int v_idx);
      void v_solid_angle(int v_idx);
      void v_order(int v_idx);
      void v_distance(int v_idx);
      void v_angles(int v_idx);

      void e_index(int e_idx);
      void e_vert_idxs(int e_idx);
      void e_face_idxs(int e_idx);
      void e_dihedral_angle(int e_idx);
      void e_central_angle(int e_idx);
      void e_distance(int e_idx);
      void e_centroid(int e_idx);
      void e_direction(int e_idx);
      void e_length(int e_idx);
      
      void f_index(int f_idx);
      void f_vert_idxs(int v_idx);
      void f_neighbours(int v_idx);
      void f_normal(int f_idx);
      void f_angles(int f_idx);
      void f_sides(int f_idx);
      void f_distance(int f_idx);
      void f_area(int f_idx);
      void f_perimeter(int f_idx);
      void f_max_nonplanar(int f_idx);
      void f_centroid(int f_idx);
      void f_lengths(int f_idx);
};




#endif // REP_PRINT_H

