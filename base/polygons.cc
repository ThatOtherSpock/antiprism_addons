/*
   Copyright (c) 2003-2008, Adrian Rossiter

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

/*!\file polygons.h
   \brief Generate polyhedra based on polygons.
*/

#include <stdlib.h>
#include <float.h>
#include <algorithm>

#include "polygons.h"
#include "std_polys.h"
#include "symmetry.h"
#include "utils.h"


polygon::polygon(int sides, int fract) :
   fraction(fract), radius(1.0), radius2(NAN), height(NAN), height2(NAN),
   twist_angle(NAN), subtype(0), max_subtype(0)
{
   parts = gcd(sides, fract);
   num_sides = sides/parts;
   fraction = fract/parts;
}
   
bool polygon::set_subtype(int typ, char *msg)
{  
   if(typ<0 || typ>max_subtype) {
      if(msg) {
         if(max_subtype==0)
            strcpy(msg, "this type of polyhedron has no subtypes");
         else
            sprintf(msg,
                  "polyhedron subtype outside of range 0 - %d", max_subtype);
      }
      return false;
   }
   
   subtype=typ;
   return true;
}



void polygon::add_polygon(geom_if &geom, double ht)
{
   vector<int> face(num_sides);
   int offset = geom.verts().size();
   for(int i=0; i<num_sides; i++) {
      geom.add_vert(radius*cos(i*angle())*vec3d::x + ht*vec3d::z +
               -radius*sin(i*angle())*vec3d::y);
      face[i] = offset+i;
   }
   geom.add_face(face);
}

void polygon::make_poly(geom_if &geom)
{
   geom_v poly_unit;
   make_poly_part(poly_unit);
   poly_unit.orient();
   geom.append(poly_unit);
   for(int i=1; i<parts; i++) {
      geom_v rep = poly_unit;
      rep.transform(mat3d::rot(vec3d::z, 2*M_PI*i/parts/num_sides));
      geom.append(rep);
   }
}

void polygon::dump()
{
   fprintf(stderr, "\npolygon %d x {%d/%d}\n", parts, num_sides, fraction);
   fprintf(stderr, "subtype %d out of %d\n", subtype, max_subtype);
   fprintf(stderr, "r=%g, r2=%g, h=%g, h2=%g, a=%g\n\n",
         radius, radius2, height, height2, twist_angle);
}


void dihedron::make_poly_part(geom_if &geom)
{
   add_polygon(geom);
   if(subtype==subtype_default) {
      geom.add_face(geom.faces(0));
      reverse(geom.raw_faces()[1].begin(), geom.raw_faces()[1].end());
   }
}


void prism::make_poly_part(geom_if &geom)
{
   if(!isnan(twist_angle) || subtype==subtype_trapezohedron ||
                             subtype==subtype_antiprism) {
      antiprism ant = *this;
      if(!isnan(twist_angle) || subtype==subtype_antiprism)
         ant.set_subtype(0);
      if(subtype==subtype_trapezohedron)
         ant.set_subtype(antiprism::subtype_trapezohedron);

      double twist_ang = isnan(twist_angle) ? 0.0 : twist_angle;
      ant.set_twist_angle(twist_ang-angle()/2);
      ant.make_poly(geom);
      return;
   }

   double ht = (!isnan(height)) ? height : radius;
   vector<vec3d> verts;
   vector<vector<int> > faces;
   verts.resize(2*num_sides);
   faces.resize(2+num_sides);
   geom_v pgon;
   add_polygon(pgon);
   for(int i=0; i<num_sides; i++) {
      verts[i] = pgon.verts(i) + (ht/2)*vec3d::z;
      verts[i+num_sides] = pgon.verts(i) - (ht/2)*vec3d::z;
      faces[0].push_back(i);
      faces[1].push_back(i+num_sides);
      faces[2+i].push_back(i);
      faces[2+i].push_back((i+1)%num_sides);
      faces[2+i].push_back((i+1)%num_sides + num_sides);
      faces[2+i].push_back(i + num_sides);
   }
   reverse(faces[0].begin(), faces[0].end());
   geom.add_verts(verts);
   geom.add_faces(faces);
}


bool antiprism::set_edge2(double e2, char *msg)
{
   double dist = 2*radius*sin(angle()/4);
   double ht = (e2>dist) ? sqrt(e2*e2-dist*dist) : 0;
   if(e2-dist<-epsilon) {
      if(msg)
         strcpy(msg, "too short to reach between vertices");
      return false;
   }
   return set_height(ht, msg);
}

bool antiprism::set_height(double ht, char *msg)
{ 
   if(subtype==subtype_subdivided_scalenohedron) {
      double E = sqrt(ht*ht+pow(2*radius*sin(angle()/4), 2));
      if(fabs(radius) > E-epsilon) {
         if(msg)
            strcpy(msg, "antprism slant height to short to close polyhedron at apex");
         return false;
      }
   }

   height = ht;
   return true;
}

void antiprism::make_scal_part(geom_if &geom)
{
   antiprism ant(*this);
   ant.set_subtype(0);
   ant.set_twist_angle(0);
   ant.make_poly_part(geom);
   geom.clear_faces();
   
   double ht = (!isnan(height)) ? height : radius;
   double apex_ht = isnan(height2) ? 2*ht : height2;
   int apex_idx1 = geom.add_vert(vec3d(0, 0, apex_ht));
   int apex_idx2 = geom.add_vert(vec3d(0, 0,-apex_ht));

   for(int i=0; i<num_sides; i++) {
      geom.add_face(i, i+num_sides, apex_idx1, -1);
      geom.add_face(i+num_sides, i, apex_idx2, -1);
      geom.add_face(i+num_sides, (i+1)%num_sides, apex_idx1, -1);
      geom.add_face((i+1)%num_sides, i+num_sides, apex_idx2, -1);
   }
}

static void add_scal_faces(geom_if &geom, int v0, int v1, int v2, int v3,
      double ht2)
{
   vec3d cent = 0.25 * (geom.verts(v0)+geom.verts(v1)+
                                    geom.verts(v2)+geom.verts(v3));
   vec3d norm = vcross(geom.verts(v0)-geom.verts(v2),
                             geom.verts(v3)-geom.verts(v1)).unit();
   int ap = geom.add_vert(cent+norm*ht2);
   geom.add_face(v0, v1, ap, -1);
   geom.add_face(v1, v2, ap, -1);
   geom.add_face(v2, v3, ap, -1);
   geom.add_face(v3, v0, ap, -1);
}

void antiprism::make_subscal_part(geom_if &geom)
{
   double ht = (!isnan(height)) ? height : radius;
   double ht2 = (!isnan(height2)) ? height2 : 0.0;
   
   double E = sqrt(ht*ht+pow(2*radius*sin(angle()/4), 2));
   double apex_ht = ht/2; // default to "flat" on failure
   if(radius<E)
      apex_ht += sqrt(E*E-radius*radius);
   
   antiprism ant(*this);
   ant.set_subtype(0);
   ant.set_twist_angle(0);
   ant.make_poly_part(geom);
   geom.clear_faces();
   
   int apex_idx1 = geom.add_vert(vec3d(0, 0, apex_ht));
   int apex_idx2 = geom.add_vert(vec3d(0, 0,-apex_ht));

   for(int i=0; i<num_sides; i++) {
      add_scal_faces(geom, i, i+num_sides, (i+1)%num_sides, apex_idx1, ht2);
      add_scal_faces(geom, (i+1)%num_sides+num_sides, (i+1)%num_sides,
            i+num_sides, apex_idx2, ht2);
   }
}





void antiprism::make_poly_part(geom_if &geom)
{
   if(subtype==subtype_scalenohedron) {
      make_scal_part(geom);
      return;
   }
   else if(subtype==subtype_subdivided_scalenohedron) {
      make_subscal_part(geom);
      return;
   }

   double ht = (!isnan(height)) ? height : radius;
   int extra_verts = 2*(subtype==subtype_trapezohedron) +
                     1*(subtype==subtype_antihermaphrodite);
   int extra_faces = 2-extra_verts;

   vector<vec3d> verts;
   vector<vector<int> > faces;
   vector<vector<int> > caps(2);
   verts.resize(2*num_sides+extra_verts);
   faces.resize(2*num_sides);
   
   geom_v pgon, pgon2;
   add_polygon(pgon);
   double twist_ang = isnan(twist_angle) ? 0.0 : twist_angle;
   pgon.transform(mat3d::rot(vec3d::z,  angle()/4-twist_ang/2));
   add_polygon(pgon2);
   pgon2.transform(mat3d::rot(vec3d::z,-angle()/4+twist_ang/2));
   if(extra_verts) {
      double apex_ht = 0.5*ht*(cos(angle()/2)+cos(twist_ang)) /
            (cos(twist_ang)-cos(angle()/2));
      if(apex_ht>FLT_MAX)
         apex_ht=FLT_MAX;
      else if(apex_ht<-FLT_MAX)
         apex_ht=-FLT_MAX;

      verts[2*num_sides] = vec3d(0,0,-apex_ht);
      if(extra_verts==2)
         verts[2*num_sides+1] = vec3d(0,0,apex_ht);
   }
   
   for(int i=0; i<num_sides; i++) {
      verts[i] = pgon.verts(i) + (ht/2)*vec3d::z;
      verts[i+num_sides] = pgon2.verts(i) - (ht/2)*vec3d::z;
      if(extra_faces) {
         caps[0].push_back(i);
         if(extra_faces>1)
            caps[1].push_back(i+num_sides);
      }
      faces[i].push_back(i);
      if(extra_verts>1)
         faces[i].push_back(2*num_sides+1);
      faces[i].push_back((i+1)%num_sides);
      faces[i].push_back(i + num_sides);
      faces[i+num_sides].push_back((i+1)%num_sides + num_sides);
      if(extra_verts)
         faces[i+num_sides].push_back(2*num_sides);
      faces[i+num_sides].push_back(i%num_sides + num_sides);
      faces[i+num_sides].push_back((i+1)%num_sides);
   }
   
   if(extra_faces) {
      reverse(caps[0].begin(), caps[0].end());
      geom.add_face(caps[0]);
      if(extra_faces>1) {
         geom.add_face(caps[1]);
      }
   }
   geom.add_verts(verts);
   geom.add_faces(faces);
}

bool pyramid::set_edge2(double e2, char *msg) {
   height = (e2>radius) ? sqrt(e2*e2-radius*radius) : 0;
   if(msg && e2-radius < -epsilon)
      strcpy(msg, "too short to reach apex");
   return (e2-radius > -epsilon);
}

void pyramid::make_poly_part(geom_if &geom)
{
   double ht = (!isnan(height)) ? height : radius;
   if( (subtype==0 && !isnan(twist_angle)) ||
            subtype==subtype_antihermaphrodite) {
      antiprism ant = *this;
      ant.set_subtype(antiprism::subtype_antihermaphrodite);

      double twist_ang = isnan(twist_angle) ? 0.0 : twist_angle - angle()/2;
      ant.set_twist_angle(twist_ang);
      double ant_ht = ht * (cos(twist_ang)/cos(angle()/2)-1);
      ant.set_height(ant_ht);
      ant.make_poly(geom);
      geom.raw_verts()[geom.verts().size()-1]=vec3d(0,0,-(0.5*ant_ht+ht));
      return;
   }

   
   geom_v pyr;
   add_polygon(pyr);
   for(int i=0; i<num_sides; i++) {
      vector<int> face(3);
      face[0] = i;
      face[1] = (i+1)%num_sides;
      face[2] = num_sides;
      pyr.add_face(face);
   }
   pyr.add_vert(ht*vec3d::z);

   if(subtype==0) {
      geom.append(pyr);
   }
   else if(subtype==subtype_elongated) {
      prism pri(*this); 
      pri.set_subtype(0);
      pri.set_height((isnan(height2)) ? get_edge() : height2);
      pri.make_poly_part(geom);
      if(num_sides>2)
         face_bond(geom, pyr);
      else {
         geom.delete_faces(vector<int>(1, 0));
         int apex = geom.add_vert(vec3d(0, 0, pri.get_height()/2+ht));
         geom.add_face(0, 1, apex, -1);
         geom.add_face(1, 0, apex, -1);
      }

   }
   else if(subtype==subtype_gyroelongated) {
      antiprism ant(*this);
      ant.set_subtype(0);
      if(isnan(height2))
         ant.set_edge2(get_edge());
      else
         ant.set_height(height2);
      ant.make_poly_part(geom);
      if(num_sides>2)
         face_bond(geom, pyr);
      else {
         geom.delete_faces(vector<int>(1, 0));
         int apex = geom.add_vert(vec3d(0, 0, get_height()/2+ht));
         geom.add_face(0, 1, apex, -1);
         geom.add_face(1, 0, apex, -1);
      }
   }
}


/*
void dipyramid::make_scal_part(geom_if &geom)
{
   double twist_ang = isnan(twist_angle) ? 0.0 : twist_angle;

   double edge = get_edge();
   double ring_ht = 0.0;
   if(subtype==subtype_scalenohedron)
      ring_ht = sin(twist_ang) * edge/2;

   double R1 = 0.0;
   double R2 = 0.0;
   if(subtype==subtype_scalenohedron)
      R1 = R2 = radius * cos(twist_ang);
   else if(subtype==subtype_dip_scalenohedron) {
      double ang = angle();
      R1 = edge*cos(ang/2 + twist_ang)/sin(ang);
      R2 = edge*cos(ang/2 - twist_ang)/sin(ang);
   }

   dipyramid dip(num_sides, fraction);
   dip.set_height(height);
   dip.set_twist_angle();
   dip.set_subtype(0);
   dip.make_poly_part(geom);
   vector<vec3d> &verts = geom.raw_verts();
   for(int i=0; i<num_sides/2; i++) {
      verts[2*i]   = verts[2*i] * R1 + vec3d(0,0, ring_ht);
      verts[2*i+1] = verts[2*i+1]*R2 + vec3d(0,0,-ring_ht);
   }
}
*/


void dipyramid::make_scal_part(geom_if &geom)
{
   double ht = (!isnan(height)) ? height : radius;
   double inrad = radius*cos(angle()/2);
   double rad2 = isnan(radius2) ? inrad : radius2;

   polygon pgon(num_sides, fraction);
   pgon.set_radius(radius);
   geom_v pg;
   pgon.add_polygon(pg);
   for(int i=0; i<num_sides; i++) {
      geom.add_vert(pg.verts(i));
      geom.add_vert((rad2/inrad)*0.5*(pg.verts(i)+pg.verts((i+1)%num_sides)));
   }
   int apex_idx1 = geom.add_vert(vec3d(0, 0, ht));
   int apex_idx2 = geom.add_vert(vec3d(0, 0,-ht));

   for(int i=0; i<2*num_sides; i++) {
      geom.add_face(i, (i+1)%(2*num_sides), apex_idx1, -1);
      geom.add_face((i+1)%(2*num_sides), i, apex_idx2, -1);
   }
}



void dipyramid::make_poly_part(geom_if &geom)
{
   double ht = (!isnan(height)) ? height : radius;
   if( (subtype==0 && !isnan(twist_angle)) ||    // make trapezohedron
            subtype==subtype_trapezohedron) {
      antiprism ant = *this;
      ant.set_subtype(antiprism::subtype_trapezohedron);
      double twist_ang = isnan(twist_angle) ? 0.0 : twist_angle - angle()/2;
      ant.set_twist_angle(twist_ang);
      double ant_ht = ht * (cos(twist_ang)/cos(angle()/2)-1);
      ant.set_height(ant_ht);
      ant.make_poly(geom);
      for(int i=0; i<2; i++)       // make sure apex heights are correct
         geom.raw_verts()[geom.verts().size()-1-i] =
            vec3d(0, 0, (1-2*i)*(0.5*ant_ht+ht));
   }
   else if(subtype==subtype_dip_scalenohedron)
      make_scal_part(geom);
   else {                                           // make dipyramid types
      // Make the top part, possibly elongated or gyroelongated
      pyramid::make_poly_part(geom);

      // Add the bottom pyramid
      pyramid pyr(*this);
      pyr.set_subtype(0);
      pyr.set_twist_angle();
      geom_v pyr_geom;
      pyr.make_poly_part(pyr_geom);
      if(num_sides>2)
         face_bond(geom, pyr_geom);
      else {
         geom.delete_faces(vector<int>(1, 0));
         int off = (subtype==0) ? 0 : 2;
         int apex = geom.add_vert(vec3d(0, 0, geom.verts(off)[2]-ht));
         geom.add_face(0+off, 1+off, apex, -1);
         geom.add_face(1+off, 0+off, apex, -1);
      }
   }
}

 
bool cupola::set_edge2(double e2, char *msg)
{
   double inrad1 = radius*cos(angle()/2);
   double inrad2 = 0.5*get_edge()/tan(angle()/4);  // larger polygon
   double diff2 = pow(inrad2 - inrad1, 2);
   height = (e2*e2>diff2) ? sqrt(e2*e2-diff2) : 0;
   if(msg && e2-radius < -epsilon)
      strcpy(msg, "too short to reach between polygons");
   return (e2*e2-diff2 > -epsilon);
}

static void prism_wrap(geom_if &geom, int num_wraps)
{
   vector<vector<int> > &faces = geom.raw_faces();
   int sz = faces.size();
   for(int i=0; i<sz; i++) {
      int f_sz = faces[i].size();
      for(int w=0; w<num_wraps; w++) {
         if(i<2)  // double wind the caps
            faces[i].insert(faces[i].end(),
                  faces[i].begin(), faces[i].begin()+f_sz);
         else     // repeat the side faces
            geom.add_face(faces[i]);
      }
   }
}
   

void cupola::make_poly_part(geom_if &geom)
{
   geom_v cup_geom;
   bool semi = is_even(fraction);
   vector<vec3d> verts;
   vector<vector<int> > faces;
   int n2 = (2-semi)*num_sides;
   polygon large(n2, fraction/(1+semi));
   large.set_edge(get_edge());
   large.add_polygon(cup_geom);
   if(semi) {
      for(int i=0; i<num_sides; i++)
         cup_geom.raw_faces()[0].push_back(i);
   }
   if(subtype==subtype_semicupola)
      cup_geom.clear_faces();
   
   vec3d r = (-angle()/4)*vec3d::z;
   cup_geom.transform(mat3d::rot(r[0], r[1], r[2]));
   double ht = (!isnan(height)) ? height : radius;
   add_polygon(cup_geom, ht);
   for(int i=0; i<2*num_sides; i++) {
      vector<int> face;
      int v = i%n2;
      face.push_back(v);
      face.push_back(n2 + ((i+1)/2)%num_sides);
      if(is_even(i))
         face.push_back(n2 + ((i/2+1)%num_sides));
      face.push_back((v+1)%n2);
      cup_geom.add_face(face);
   }

   if(subtype==subtype_default || subtype==subtype_semicupola) {
      geom.append(cup_geom);
      if(subtype==subtype_semicupola)
         cup_geom.clear_faces();
   }
   else if(subtype==subtype_elongated) {
      prism pri(large);
      pri.set_twist_angle(twist_angle);
      pri.set_height((isnan(height2)) ? get_edge() : height2);
      pri.make_poly_part(geom);
      if(semi)
         prism_wrap(geom, 2);
      face_bond(geom, cup_geom);
   }
   else if(subtype==subtype_gyroelongated) {
      antiprism ant(large);
      ant.set_twist_angle(twist_angle);
      if(isnan(height2))
         ant.set_edge2(get_edge());
      else
         ant.set_height(height2);
      ant.make_poly_part(geom);
      if(semi)
         prism_wrap(geom, 2);
      face_bond(geom, cup_geom);
   }
}

void cupola::make_poly(geom_if &geom)
{
   polygon::make_poly(geom);
   //merge vertices which are coincident in compound
   map<int, int> vmap;
   if(!is_even(fraction) && is_even(parts)) {
      for(int i=0; i<parts/2; i++) {
         int base_part = i;                   // keep base part polygon verts
         int merge_part = base_part+parts/2;  // use base part polygon verts
         int lrg_sz = 2*num_sides;            // num sides of the large polygon
         
         // How many polygon steps to turn -1 vertices on {op_sz/1}
         int off;
         for(off=0; off<lrg_sz; off++)
            if((off*fraction)%lrg_sz==lrg_sz-1)
               break;
         
         for(int j=0; j<lrg_sz; j++)
            vmap[merge_part*num_sides*3 + j] = 
               base_part*num_sides*3 + (j+off)%lrg_sz;
      }
   }
   geom.verts_merge(vmap);
}


void orthobicupola::make_poly_part(geom_if &geom)
{
   // Make the top part, possibly elongated or gyroelongated
   cupola::make_poly_part(geom);
   
   // Add the bottom cupola
   cupola cup(*this);
   cup.set_subtype(0);
   cup.set_twist_angle();
   geom_v cup_geom;
   cup.make_poly_part(cup_geom);
   face_bond(geom, cup_geom, 0, 0, (subtype==0)); // variation in bases
}


void gyrobicupola::make_poly_part(geom_if &geom)
{
   // Make the top part, possibly elongated or gyroelongated
   cupola::make_poly_part(geom);
   
   // Add the bottom cupola
   cupola cup(*this);
   cup.set_subtype(0);
   cup.set_twist_angle(twist_angle);
   geom_v cup_geom;
   cup.make_poly_part(cup_geom);
   face_bond(geom, cup_geom, 0, 0, (subtype!=0)); // variation in bases
}

bool snub_antiprism::set_edge2(double e2, char *msg)
{
   if(e2==1)
      return true;
   else {
      if(msg)
         strcpy(msg, "cannot set height for this polyhedron");
      return false;
   }
}

bool snub_antiprism::set_height(double /*h*/, char *msg)
{
   if(msg)
      strcpy(msg, "cannot set height for this polyhedron");
   return false;
}


void snub_antiprism::make_poly_part(geom_if &geom)
{
   const double sqrt_epsilon = sqrt(epsilon);
   const double ang_inc = fraction*M_PI/num_sides;
   const double s = sin(ang_inc);
   const double c = cos(ang_inc);
   double coeffs[5];
   coeffs[0] = (2-3*c*c);
   coeffs[1] = 2*(1 + c);
   coeffs[2] = 4*c - 7;
   coeffs[3] = -4;
   coeffs[4] = 4;
   double sol[4];
   int num_roots = quartic(coeffs, sol);
   //fprintf(stderr, "\n%d radius values to test\n", num_roots);
   vector<double> valid;
   for(int i=0; i<num_roots; i++) {
      double r = sol[i]/s;
      //fprintf(stderr, "\nradius (%d)\n   r = %.16f\n", i, r);
      double rt = 1 - pow(r-0.5/s, 2);
      //fprintf(stderr, "   h1^2 = %.16f\n", rt);
      if(rt<-sqrt_epsilon)
         continue;
      double h1 = rt>sqrt_epsilon ? sqrt(rt) : 0;
      rt = 3/4.0 - pow(r-0.5*c/s, 2);
      //fprintf(stderr, "   h2^2 = %.16f\n", rt);
      if(rt<-sqrt_epsilon)
         continue;
      double h2 = rt>sqrt_epsilon ? sqrt(rt) : 0;
      for(int j=0; j<2; j++) {
         h1 *= -1; // flip sign
         //fprintf(stderr, "      h1=%.16f\n", h1);
         //fprintf(stderr, "      h2=%.16f\n", h2);
         //fprintf(stderr, "      edge length = %.16f\n", 2*r*r*(1-c)+(h1-h2)*(h1-h2));
         if(fabs(2*r*r*(1-c)+(h1-h2)*(h1-h2)-1)>epsilon/s)
            continue;
         valid.push_back(r);
         valid.push_back(h1);
         valid.push_back(h2);
         if(fabs(h1)<epsilon || fabs(h2)<epsilon)
            break;
      }
   }

   int num_sols = valid.size()/3;
   if(num_sols>2)
      fprintf(stderr, "\n\n*** unexpected: snub-antiprism has more than 2 solutions!!! (%d solutions)\n\n)", num_sols);
   
   if(subtype>num_sols-1)
      return;
  
   double r = valid[subtype*3];
   double H = (valid[subtype*3+1]+valid[3*subtype+2])/2;  // new top layer ht
   double h = H-valid[subtype*3+1];       // new upper inner layer height

   //fprintf(stderr, "\nFinal values\n   r = %.16f\n   H = %.16f\n   h = %.16f\n",
   //      r, H, h);
   
   set_edge(1);
   add_polygon(geom, H);
   vector<int> bond_face;
   for(int i=0; i<num_sides; i++) {
      int a = num_sides+2*i;
      geom.add_vert(r*cos(2*i*ang_inc)*vec3d::x + h*vec3d::z +
            -r*sin(2*i*ang_inc)*vec3d::y);
      geom.add_vert(r*cos((2*i+1)*ang_inc)*vec3d::x - h*vec3d::z +
            -r*sin((2*i+1)*ang_inc)*vec3d::y);
      vector<int> face(3);
      face[0] = i;
      face[1] = a;
      face[2] = a+1;
      geom.add_face(face);
      face[0] = i;
      face[1] = (i+1)%num_sides;
      face[2] = a+1;
      geom.add_face(face);
      face[2] = (i+1)%num_sides;
      face[0] = a+1;
      face[1] = num_sides +(2*(i+1))%(2*num_sides);
      geom.add_face(face);
      bond_face.push_back(a);
      bond_face.push_back(a+1);
   }
   geom.add_face(bond_face);
   col_geom_v geom2 = geom;
   face_bond(geom, geom2, geom.faces().size()-1, geom2.faces().size()-1, 1);
   //align dihedral axis with x-axis
   geom.transform(mat3d::rot(vec3d::z, M_PI*fraction/(2.0*num_sides)));
}


int make_resource_pgon(geom_if &geom, string pname, char *errmsg)
{
   if(pname.find('.')!=string::npos)
      return -1; // not polygon res name (the "." indicates a likely local file)
                 // so the name is not handled
   char pnam[MSG_SZ];
   strncpy(pnam, pname.c_str(), MSG_SZ);
   int num_sides;
   int fraction=1;
   char *pnum = pnam+3;
   char *p = strchr(pnum, '/');
   if(p!=0) {
      *p++='\0';
      if(!read_int(p, &fraction))
         return -1;
   }
   if(!read_int(pnum, &num_sides))
      return -1;
   if(num_sides<2)
      return -1;
   if(fraction<1)
      return -1;
   if(fraction >= num_sides)
      return -1;
   
   *pnum = '\0';

   polygon pgon(num_sides, fraction);
   polygon *poly;
   if(strcasecmp("pri", pnam)==0)
      poly = new prism(pgon);
   else if(strcasecmp("ant", pnam)==0)
      poly = new antiprism(pgon);
   else if(strcasecmp("pyr", pnam)==0)
      poly = new pyramid(pgon);
   else if(strcasecmp("dip", pnam)==0)
      poly = new dipyramid(pgon);
   else if(strcasecmp("cup", pnam)==0)
      poly = new cupola(pgon);
   else if(strcasecmp("ort", pnam)==0)
      poly = new orthobicupola(pgon);
   else if(strcasecmp("gyr", pnam)==0)
      poly = new gyrobicupola(pgon);
   else if(strcasecmp("snu", pnam)==0)
      poly = new snub_antiprism(pgon);
   else
      return -1;

   // check can have unit edge
   int ret;
   poly->set_edge(1);
   if(!poly->set_edge2(1)) {
      if(errmsg)
         strcpy(errmsg, "polyhedron cannot have unit edges");
      ret = 1;
   }
   else {
      poly->make_poly(geom);
      ret = 0;
   }

   delete poly;
   set_resource_polygon_color(geom);
   return ret;
}




