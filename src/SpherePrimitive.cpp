/****************************************************************************
 * Copyright 2005 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifdef USE_OSG
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#endif
#include <Moby/Constants.h>
#include <Moby/CompGeom.h>
#include <Moby/sorted_pair>
#include <Moby/XMLTree.h>
#include <Moby/BoundingSphere.h>
#include <Moby/CollisionGeometry.h>
#include <Moby/BoxPrimitive.h>
#include <Moby/SpherePrimitive.h>

using namespace Ravelin;
using namespace Moby;
using boost::shared_ptr; 
using std::map;
using std::vector;
using std::list;
using std::pair;
using boost::dynamic_pointer_cast;
using std::make_pair;

/// Creates a sphere with radius 1.0 and 100 points 
SpherePrimitive::SpherePrimitive()
{
  _radius = 1.0;
  _npoints = 100;
  calc_mass_properties();
}

/// Creates a sphere with radius 1.0 and 100 points at the given transform
SpherePrimitive::SpherePrimitive(const Pose3d& T) : Primitive(T)
{
  _radius = 1.0;
  _npoints = 100;
  calc_mass_properties();
}

/// Creates a sphere with the specified radius and 100 points 
SpherePrimitive::SpherePrimitive(double radius)
{
  _radius = radius;
  _npoints = 100;
  calc_mass_properties();
}

/// Creates a sphere with the specified radius and number of points
SpherePrimitive::SpherePrimitive(double radius, unsigned n)
{
  _radius = radius;
  _npoints = n;
  calc_mass_properties();
}

/// Creates a sphere with the specified radius and transform
/**
 * The sphere is composed of 100 points.
 */
SpherePrimitive::SpherePrimitive(double radius, const Pose3d& T) : Primitive(T)
{
  _radius = radius;
  _npoints = 100;
  calc_mass_properties();
}

/// Creates a sphere with the specified radius, transform, and number of points 
SpherePrimitive::SpherePrimitive(double radius, unsigned n, const Pose3d& T) : Primitive(T)
{
  _radius = radius;
  _npoints = n;  
  calc_mass_properties();
}

/// Gets the supporting point
Point3d SpherePrimitive::get_supporting_point(const Vector3d& d) 
{
  return Vector3d::normalize(d)*_radius;
}

/// Computes the signed distance of the given point from this primitive
double SpherePrimitive::calc_signed_dist(const Point3d& p)
{
  assert(p.pose == get_pose());

  return p.norm() - _radius;
}

/// Computes the distance from another sphere primitive
double SpherePrimitive::calc_signed_dist(shared_ptr<const SpherePrimitive> s, shared_ptr<const Pose3d> pose_this, shared_ptr<const Pose3d> pose_s, Point3d& pthis, Point3d& ps) const
{
  // get the transform from s to this
  Transform3d T = Pose3d::calc_relative_pose(pose_s, pose_this);

  // compute the distance
  double d = T.x.norm() - _radius - s->_radius;

  // setup poses
  pthis.pose = get_pose();
  ps.pose = s->get_pose();

  // setup sphere centers in alternate frames
  Point3d ps_c(0.0, 0.0, 0.0, pose_s);
  Point3d pthis_c(0.0, 0.0, 0.0, pose_this);

  // setup closest points
  pthis = Pose3d::transform_point(pose_this, ps_c);
  pthis.normalize();
  ps = Pose3d::transform_point(pose_s, pthis_c);
  ps.normalize();

  // scale closest points appropriately
  if (d > 0.0)
  {
    pthis *= _radius;
    ps *= s->_radius;
  }
  else
  {
    pthis *= _radius+d;
    ps *= s->_radius+d;
  }

  return d;
}

/// Calculates mass properties for the sphere 
void SpherePrimitive::calc_mass_properties() 
{
  // get the current transform
  shared_ptr<const Pose3d> T = get_pose();

  // compute the mass if density is given
  if (_density)
  {
    const double volume = M_PI * _radius * _radius * _radius * 4.0/3.0; 
    _J.m = *_density * volume;
  }

  // compute the diagonal element of the untransformed inertia matrix
  const double diag = _radius * _radius * _J.m * 2.0/5.0;

  // form the inertia matrix (untransformed)
  _J.J = Matrix3d(diag, 0, 0, 0, diag, 0, 0, 0, diag);
}

/// Sets the radius for this sphere (forces redetermination of the mesh)
void SpherePrimitive::set_radius(double radius)
{
  _radius = radius;
  if (_radius < 0.0)
    throw std::runtime_error("Attempting to set negative radius in call to SpherePrimitive::set_radius()");

  // mesh, vertices are no longer valid
  _mesh = shared_ptr<IndexedTriArray>();
  _vertices.clear();
  _smesh = pair<shared_ptr<IndexedTriArray>, list<unsigned> >();
  _invalidated = true;

  // recalculate mass properties
  calc_mass_properties();

  // need to update the visualization
  update_visualization();

  // set radius on each bounding sphere
  for (map<CollisionGeometryPtr, shared_ptr<BoundingSphere> >::iterator i = _bsphs.begin(); i != _bsphs.end(); i++)
    i->second->radius = _radius;
}

/// Sets the number of points used in this sphere 
/**
 * \param n the number of points
 * \note forces redetermination of the mesh
 */
void SpherePrimitive::set_num_points(unsigned n)
{
  _npoints = n;

  // vertices are no longer valid
  _vertices.clear();
  _invalidated = true;
}

/// Transforms the primitive
void SpherePrimitive::set_pose(const Pose3d& p)
{
  // convert p to a shared pointer
  shared_ptr<Pose3d> x(new Pose3d(p));

  // determine the transformation from the old pose to the new one 
  Transform3d T = Pose3d::calc_relative_pose(_F, x);

  // go ahead and set the new transform
  Primitive::set_pose(p);

  // clear the mesh and vertices
  _mesh.reset(); 
  _smesh.first.reset();
  _smesh.second.clear();
  _vertices.clear();

  // invalidate this primitive
  _invalidated = true;

  // recalculate the mass properties
  calc_mass_properties();

  // fix poses on bounding spheres
  for (map<CollisionGeometryPtr, shared_ptr<BoundingSphere> >::iterator i = _bsphs.begin(); i != _bsphs.end(); i++)
  {
    // get the pose for the geometry
    shared_ptr<const Pose3d> gpose = i->first->get_pose();

    // verify that this pose is defined w.r.t. the global frame
    shared_ptr<const Pose3d> P = get_pose();
    assert(!P->rpose);

    // setup the bounding sphere center; we're assuming that the primitive
    // pose is defined relative to the geometry frame
    i->second->center = Point3d(P->x, gpose);
  }
}

/// Gets the mesh, computing it if necessary
shared_ptr<const IndexedTriArray> SpherePrimitive::get_mesh()
{
  if (!_mesh)
  {
    // if the radius is zero, create an empty mesh 
    if (_radius == 0.0)
    {
      _mesh = shared_ptr<IndexedTriArray>(new IndexedTriArray());
      _smesh = make_pair(_mesh, list<unsigned>());
      return _mesh;
    }

    // get the translation for the transform
    shared_ptr<const Pose3d> T = get_pose();

    // determine the vertices in the mesh
    // NOTE: they will all be defined in the global frame
    list<Point3d> points;
    const double INC = (double) M_PI * ((double) 3.0 - std::sqrt((double) 5.0));
    const double OFF = (double) 2.0 / _npoints;
    for (unsigned k=0; k< _npoints; k++)
    {
      const double Y = k * OFF - (double) 1.0 + (OFF * (double) 0.5);
      const double R = std::sqrt((double) 1.0 - Y*Y);
      const double PHI = k * INC;
      Vector3d unit(std::cos(PHI)*R, Y, std::sin(PHI)*R);
      points.push_back(T->transform_point(unit*_radius));
    }

    // add points at the extents
    points.push_back(T->transform_point(Vector3d(+1*_radius,0,0)));
    points.push_back(T->transform_point(Vector3d(-1*_radius,0,0)));
    points.push_back(T->transform_point(Vector3d(0,+1*_radius,0)));
    points.push_back(T->transform_point(Vector3d(0,-1*_radius,0)));
    points.push_back(T->transform_point(Vector3d(0,0,+1*_radius)));
    points.push_back(T->transform_point(Vector3d(0,0,-1*_radius)));

    // compute the convex hull
    PolyhedronPtr hull = CompGeom::calc_convex_hull(points.begin(), points.end());

    // set the mesh
    const vector<Origin3d>& v = hull->get_vertices();
    const vector<IndexedTri>& f = hull->get_facets();
    _mesh = boost::shared_ptr<IndexedTriArray>(new IndexedTriArray(v.begin(), v.end(), f.begin(), f.end()));

    // setup sub mesh (it will be just the standard mesh)
    list<unsigned> all_tris;
    for (unsigned i=0; i< _mesh->num_tris(); i++)
      all_tris.push_back(i);
    _smesh = make_pair(_mesh, all_tris);
  }

  return _mesh;
}

/// Gets a sub-mesh for the primitive
const std::pair<boost::shared_ptr<const IndexedTriArray>, std::list<unsigned> >& SpherePrimitive::get_sub_mesh(BVPtr bv)
{
  if (!_smesh.first)
    get_mesh(); 
  return _smesh;
}

/// Gets vertices for the primitive
void SpherePrimitive::get_vertices(std::vector<Point3d>& vertices)
{
  // clear the vertices at first 
  vertices.clear(); 

  // look for no vertices 
  if (_radius == 0.0)
    return;

  // resize number of points
  vertices.resize(_npoints+6);

  // determine the vertices in the mesh
  // NOTE: they will all be defined in the global frame
  const double INC = (double) M_PI * ((double) 3.0 - std::sqrt((double) 5.0));
  const double OFF = (double) 2.0 / _npoints;
  for (unsigned k=0; k< _npoints; k++)
  {
    const double Y = k * OFF - (double) 1.0 + (OFF * (double) 0.5);
    const double R = std::sqrt((double) 1.0 - Y*Y);
    const double PHI = k * INC;
    vertices[k] = Vector3d(std::cos(PHI)*R, Y, std::sin(PHI)*R, get_pose())*_radius;
  }

  // setup vertices at extents of each axis
  vertices[_npoints+0] = Vector3d(1.0*_radius, 0.0, 0.0, get_pose());
  vertices[_npoints+1] = Vector3d(-1.0*_radius, 0.0, 0.0, get_pose());
  vertices[_npoints+2] = Vector3d(0.0, 1.0*_radius, 0.0, get_pose());
  vertices[_npoints+3] = Vector3d(0.0, -1.0*_radius, 0.0, get_pose());
  vertices[_npoints+4] = Vector3d(0.0, 0.0, 1.0*_radius, get_pose());
  vertices[_npoints+5] = Vector3d(0.0, 0.0, -1.0*_radius, get_pose());
}

/// Finds the signed distance between the sphere and another primitive
double SpherePrimitive::calc_signed_dist(shared_ptr<const Primitive> p, shared_ptr<const Pose3d> pose_this, shared_ptr<const Pose3d> pose_p, Point3d& pthis, Point3d& pp) const
{
  // first try box/sphere
  shared_ptr<const BoxPrimitive> boxp = dynamic_pointer_cast<const BoxPrimitive>(p);
  if (boxp)
  {
    shared_ptr<const SpherePrimitive> thisp = dynamic_pointer_cast<const SpherePrimitive>(shared_from_this());
    return boxp->calc_signed_dist(thisp, pose_p, pose_this, pp, pthis);
  }

  // now try sphere/sphere
  shared_ptr<const SpherePrimitive> spherep = dynamic_pointer_cast<const SpherePrimitive>(p);
  if (spherep)
    return calc_signed_dist(spherep, pose_this, pose_p, pthis, pp);

  assert(false);
  return 0.0;
}

/// Finds the signed distance betwen the sphere and a point
double SpherePrimitive::calc_dist_and_normal(const Point3d& p, Vector3d& normal) const
{
  // setup the normal
  normal = p;
  double pnorm = p.norm();
  normal /= pnorm;

  // compute the distance
  return pnorm - _radius;
}

/*
void SpherePrimitive::get_vertices(CollisionGeometryPtr geom, std::vector<const Point3d*>& vertices)
{
  // get the vertices for the geometry
  vector<Point3d>& verts = _vertices[geom];

  // create the vector of vertices if necessary
  if (verts.empty())
  {
    if (_radius == 0.0 || _npoints < 6)
    {
      vertices.clear(); 
      return;
    }

    // get the pose for the geometry
    shared_ptr<const Pose3d> gpose = geom->get_pose();

    // verify that this pose is defined w.r.t. the global frame
    shared_ptr<const Pose3d> P = get_pose();
    assert(!P->rpose);

    // setup transform
    Transform3d T;
    T.source = gpose;
    T.target = gpose;
    T.x = P->x;
    T.q = P->q;

    // resize number of points
    verts.resize(_npoints);

    // determine the vertices in the mesh
    // NOTE: they will all be defined in the global frame
    const double INC = (double) M_PI * ((double) 3.0 - std::sqrt((double) 5.0));
    const double OFF = (double) 2.0 / _npoints;
    for (unsigned k=0; k< _npoints; k++)
    {
      const double Y = k * OFF - (double) 1.0 + (OFF * (double) 0.5);
      const double R = std::sqrt((double) 1.0 - Y*Y);
      const double PHI = k * INC;
      Vector3d unit(std::cos(PHI)*R, Y, std::sin(PHI)*R, gpose);
      verts[k] = T.transform_point(unit*(_radius));
    }
  }

  // copy the addresses of the computed vertices into 'vertices' 
  vertices.resize(verts.size());
  for (unsigned i=0; i< verts.size(); i++)
    vertices[i] = &verts[i];
}
*/

/// Creates the visualization for this primitive
osg::Node* SpherePrimitive::create_visualization()
{
  #ifdef USE_OSG
  osg::Sphere* sphere = new osg::Sphere;
  sphere->setRadius((float) _radius);
  osg::Geode* geode = new osg::Geode;
  geode->addDrawable(new osg::ShapeDrawable(sphere));
  return geode;
  #else
  return NULL;
  #endif
}  

/// Implements Base::load_from_xml() for serialization
void SpherePrimitive::load_from_xml(shared_ptr<const XMLTree> node, std::map<std::string, BasePtr>& id_map)
{
  // verify that the node type is sphere
  assert(strcasecmp(node->name.c_str(), "Sphere") == 0);

  // load the parent data
  Primitive::load_from_xml(node, id_map);

  // read in the radius, if specified
  XMLAttrib* radius_attr = node->get_attrib("radius");
  if (radius_attr)
    set_radius(radius_attr->get_real_value());

  // read in the number of points, if specified
  XMLAttrib* npoints_attr = node->get_attrib("num-points");
  if (npoints_attr)
    set_num_points(npoints_attr->get_unsigned_value());

  // recompute mass properties
  calc_mass_properties();
}

/// Implements Base::save_to_xml() for serialization
void SpherePrimitive::save_to_xml(XMLTreePtr node, std::list<shared_ptr<const Base> >& shared_objects) const
{
  // save the parent data
  Primitive::save_to_xml(node, shared_objects);

  // (re)set the node name
  node->name = "Sphere";

  // save the radius
  node->attribs.insert(XMLAttrib("radius", _radius));

  // save the number of points 
  node->attribs.insert(XMLAttrib("num-points", _npoints));
}

/// Gets the root bounding volume
BVPtr SpherePrimitive::get_BVH_root(CollisionGeometryPtr geom) 
{
  // sphere not applicable for deformable bodies 
  if (is_deformable())
    throw std::runtime_error("SpherePrimitive::get_BVH_root(CollisionGeometryPtr geom) - primitive unusable for deformable bodies!");

  // get the pointer to the bounding sphere
  shared_ptr<BoundingSphere>& bsph = _bsphs[geom];

  // if the bounding sphere hasn't been created, create it and initialize it
  if (!bsph)
  {
    // create the sphere
    bsph = shared_ptr<BoundingSphere>(new BoundingSphere);
    bsph->geom = geom;

    // get the pose for the geometry
    shared_ptr<const Pose3d> gpose = geom->get_pose();

    // get the pose for this geometry
    shared_ptr<const Pose3d> P = get_pose(); 
    assert(!P->rpose);

    // setup the bounding sphere center; we're assuming that the primitive
    // pose is defined relative to the geometry frame
    bsph->center = Point3d(P->x, gpose);
    bsph->radius = _radius;
  }

  return bsph;
}

/**
/// Determines whether there is an intersection between the primitive and a line segment
 * Determines the normal to the primitive if there is an intersection.
 * \note for line segments that are partially or fully inside the sphere, the
 *       method only returns intersection if the second endpoint of the segment
 *       is farther inside than the first
bool SpherePrimitive::intersect_seg(BVPtr bv, const LineSeg3& seg, double& t, Point3d& isect, Vector3d& normal) const
{
  const unsigned X = 0, Y = 1, Z = 2;
  static shared_ptr<Pose3d> P;

  // get the pose for the collision geometry
  shared_ptr<const Pose3d> gpose = bv->geom->get_pose(); 

  // get the pose for this geometry and BV
  shared_ptr<const Pose3d> bpose = get_pose(); 
  assert(!bpose->rpose);

  // setup a new pose
  if (!P)
    P = shared_ptr<Pose3d>(new Pose3d);
  *P = *bpose;
  P->rpose = gpose;

  // transform the segments to sphere space
  Transform3d T = Pose3d::calc_relative_pose(seg.first.pose, P);
  Vector3d p = T.transform_point(seg.first); 
  Vector3d q = T.transform_point(seg.second);

  // get the radius plus the intersection tolerance
  const double R = _radius;

  // determine whether p is already within the sphere
  double pp = p.dot(p);
  if (pp <= R*R)
  {
    // set the intersection
    t = (double) 0.0;
    isect = p;
    double pnorm = std::sqrt(pp);
    normal.pose = get_pose();
    if (pnorm > NEAR_ZERO)
      normal = p/pnorm;
    else
      normal.set_zero();

    // transform the intersection points and normal back to p's frame
    isect = T.inverse_transform_point(isect);
    normal = T.inverse_transform_vector(normal);

    return true; 
  }

  // look for:
  // (seg.first*t + seg.second*(1-t))^2 = R^2

  // use quadratic formula
  const double px = p[X];
  const double py = p[Y];
  const double pz = p[Z];
  const double qx = q[X];
  const double qy = q[Y];
  const double qz = q[Z];
  const double a = px*px + py*py + pz*pz - 2*px*qx + qx*qx - 2*py*qy + qy*qy -
                 2*pz*qz + qz*qz;
  const double b = 2*px*qx - 2*qx*qx + 2*py*qy - 2*qy*qy + 2*pz*qz - 2*qz*qz;
  const double c = qx*qx + qy*qy + qz*qz - R*R;

  // check for no solution
  if (a == 0.0)
    return false;
  double disc = b*b - 4*a*c;
  if (disc < 0.0)
    return false;

  // compute solutions
  disc = std::sqrt(disc);
  double t1 = (-b + disc)/(2*a);
  double t2 = (-b - disc)/(2*a);

  // look for lowest solution in [0, 1]
  if (t1 < 0.0)
    t1 = 2.0;
  if (t2 < 0.0)
    t2 = 2.0;
  if (t2 < t1)
    std::swap(t1, t2);
  if (t1 < 0.0 || t1 > 1.0)
    return false;

  // compute the point of intersection and normal
  isect = seg.first*t1 + seg.second*(1-t1);
  normal = Vector3d::normalize(Vector3d(isect));

  // transform the intersection point and normal back to p's frame
  isect = T.inverse_transform_point(isect); 
  normal = T.inverse_transform_vector(normal);

  t = t1;
  return true;
}
*/

/// Determines whether a point is inside or on the sphere
/*
bool SpherePrimitive::point_inside(CollisionGeometryPtr geom, const Point3d& p, Vector3d& normal) const
{
  static shared_ptr<Pose3d> P;

  // get the pose for the collision geometry
  shared_ptr<const Pose3d> gpose = geom->get_pose(); 

  // get the pose for this geometry and BV
  shared_ptr<const Pose3d> bpose = get_pose(); 
  assert(!bpose->rpose);

  // setup a new pose
  if (!P)
    P = shared_ptr<Pose3d>(new Pose3d);
  *P = *bpose;
  P->rpose = gpose;

  // transform the point to sphere space
  Transform3d T = Pose3d::calc_relative_pose(p.pose, P);
  Point3d query = T.transform_point(p);

  // check whether query outside of radius
  if (query.norm_sq() > _radius * _radius) 
    return false;

  // determine normal
  normal = Vector3d::normalize(query);

  // transform the normal back to p's space
  normal = T.inverse_transform_vector(normal);

  return true;
}
*/
