/****************************************************************************
 * Copyright 2005 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifndef _PRIMITIVE_H
#define _PRIMITIVE_H

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <Ravelin/Vector3d.h>
#include <Ravelin/Matrix3d.h>
#include <Ravelin/Pose3d.h>
#include <Moby/Base.h>
#include <Moby/Triangle.h>
#include <Moby/ThickTriangle.h>
#include <Moby/Constants.h>
#include <Moby/IndexedTriArray.h>

namespace osg {
  class MatrixTransform;
  class Material;
  class Node;
  class Matrixd;
}

namespace Moby {

/// Defines a triangle-mesh-based primitive type used for inertial property calculation and geometry provisions
/**
 * The center-of-mass of derived types may be at the origin of the world,
 * or not.  Additionally, Primitive can take a transformation matrix in its
 * constructor, with which the primitive data (com, inertia matrix, and geometry)
 * can be transformed.
 */
class Primitive : public virtual Base
{
  friend class CSG;

  public:
    Primitive();
    Primitive(const Ravelin::Pose3d& T);
    virtual ~Primitive();
    virtual void load_from_xml(boost::shared_ptr<const XMLTree> node, std::map<std::string, BasePtr>& id_map);
    virtual void save_to_xml(XMLTreePtr node, std::list<boost::shared_ptr<const Base> >& shared_objects) const;
    void update_visualization();
    void set_mass(double mass);
    void set_density(double density);
    virtual void set_pose(const Ravelin::Pose3d& T);
    virtual Point3d get_supporting_point(const Ravelin::Vector3d& d);
    virtual double calc_signed_dist(const Point3d& p);

    /// Computes the distance between a point and this primitive
    virtual double calc_dist_and_normal(const Point3d& p, Ravelin::Vector3d& normal) const = 0;

    /// Computes the signed distance between this and another primitive
    virtual double calc_signed_dist(boost::shared_ptr<const Primitive> p, boost::shared_ptr<const Ravelin::Pose3d> pose_this, boost::shared_ptr<const Ravelin::Pose3d> pose_p, Point3d& pthis, Point3d& pp) const = 0;

     /// Gets the visualization for this primitive
    virtual osg::Node* get_visualization();
    virtual osg::Node* create_visualization() = 0;

    /// Sets whether this primitive is used for a deformable body
    virtual void set_deformable(bool flag) { _deformable = flag; }

    /// Gets the root bounding volume for this primitive
    virtual BVPtr get_BVH_root(CollisionGeometryPtr geom) = 0; 

    /// Returns whether this primitive is deformable
    bool is_deformable() const { return _deformable; }

    /// Get vertices corresponding to this primitive
    virtual void get_vertices(std::vector<Point3d>& vertices) = 0;

    /// Gets vertices corresponding to the bounding volume
//    virtual void get_vertices(BVPtr bv, std::vector<const Point3d*>& vertices) = 0; 

    /// Determines whether a line segment and the shape intersect
    /**
     * \param bv a bounding volume (to speed intersection testing)
     * \param seg the line segment
     * \param t the parameter of the intersection (seg.first + seg.second*t)
     *        (if intersection)
     * \param isect the point of intersection, on return (if any)
     * \param normal the normal to the shape at the point of intersection, 
     *          on return (if any)
     * \return <b>true</b> if intersection, <b>false</b> otherwise 
     */
//    virtual bool intersect_seg(BVPtr bv, const LineSeg3& seg, double& t, Point3d& isect, Ravelin::Vector3d& normal) const = 0;

    /// Gets mesh data for the geometry with the specified bounding volume
    /**
     * \param bv the bounding data from which the corresponding mesh data will
     *           be taken
     * \return a pair consisting of an IndexedTriArray and a list of triangle
     *         indices encapsulated by bv
     */
    virtual const std::pair<boost::shared_ptr<const IndexedTriArray>, std::list<unsigned> >& get_sub_mesh(BVPtr bv) = 0;

    /// Gets the inertial frame of this primitive
    boost::shared_ptr<const Ravelin::Pose3d> get_inertial_pose() const { return _jF; }

    /// Gets the pose of this primitive 
    boost::shared_ptr<const Ravelin::Pose3d> get_pose() const { return _F; } 

    /// Gets the underlying triangle mesh for this primitive 
    virtual boost::shared_ptr<const IndexedTriArray> get_mesh() = 0;

    /// Gets the inertia for this primitive 
    const Ravelin::SpatialRBInertiad& get_inertia() const { return _J; }

  protected:
    virtual void calc_mass_properties() = 0;

    /// The pose of this primitive
    boost::shared_ptr<Ravelin::Pose3d> _F;

    /// The inertial pose of this primitive
    boost::shared_ptr<Ravelin::Pose3d> _jF;

    /// The density of this primitive
    boost::shared_ptr<double> _density;

    /// The inertia of the primitive
    Ravelin::SpatialRBInertiad _J;

    /// Indicates whether the primitive's mesh or vertices have changed
    bool _invalidated;

  private:

    /// Whether the geometry is deformable or not
    bool _deformable;

    /// The visualization transform (possibly NULL)
    osg::MatrixTransform* _vtransform;

    /// The material for the visualization (possibly NULL)
    osg::Material* _mat;
}; // end class

} // end namespace

#endif
