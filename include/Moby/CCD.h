/****************************************************************************
 * Copyright 2009 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifndef _CCD_H
#define _CCD_H

#include <list>
#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include <Moby/sorted_pair>
#include <Moby/Log.h>
#include <Moby/SpherePrimitive.h>
#include <Moby/BoxPrimitive.h>
#include <Moby/BV.h>

namespace Moby {

class RigidBody;
class ArticulatedBody;
class CollisionGeometry;  

/// Implements the CollisionDetection abstract class to perform exact contact finding using abstract shapes 
class CCD
{
  public:
    CCD();
    virtual ~CCD() {}
    virtual void load_from_xml(boost::shared_ptr<const XMLTree> node, std::map<std::string, BasePtr>& id_map);
    virtual void save_to_xml(XMLTreePtr node, std::list<boost::shared_ptr<const Base> >& shared_objects) const;
    double find_next_contact_time(CollisionGeometryPtr cgA, CollisionGeometryPtr cgB);
    void broad_phase(double dt, const std::vector<DynamicBodyPtr>& bodies, std::vector<std::pair<CollisionGeometryPtr, CollisionGeometryPtr> >& to_check);
    double calc_CA_step(CollisionGeometryPtr cgA, CollisionGeometryPtr cgB);

    template <class OutputIterator>
    OutputIterator find_contacts(CollisionGeometryPtr cgA, CollisionGeometryPtr cgB, OutputIterator output_begin);

    /// Pairs of collision geometries that aren't checked for contact/collision
    /**
     * \note collisions between geometries for two disabled bodies and 
     *       collisions between geometries for a single body are automatically
     *       not checked and do not need to be added to this set.
     */
    std::set<sorted_pair<CollisionGeometryPtr> > disabled_pairs;

  private:
    // the 3 axes
    enum AxisType { eXAxis, eYAxis, eZAxis };

    // structure for doing broad phase collision detection
    struct BoundsStruct
    {
      bool end;                   // bounds is for start or end
      CollisionGeometryPtr geom;  // the geometry
      BVPtr bv;                   // the unexpanded bounding volume
      bool operator<(const BoundsStruct& bs) const { return (!end && bs.end); } 
    };

    // gets the distance on farthest points
    std::map<CollisionGeometryPtr, double> _rmax;

    // see whether the bounds vectors need to be rebuilt
    bool _rebuild_bounds_vecs;

    // the bounding spheres 
    std::map<CollisionGeometryPtr, BVPtr> _bounding_spheres; 

    /// AABB bounds (x-axis)
    std::vector<std::pair<double, BoundsStruct> > _x_bounds;

    /// AABB bounds (y-axis)
    std::vector<std::pair<double, BoundsStruct> > _y_bounds;

    /// AABB bounds (z-axis)
    std::vector<std::pair<double, BoundsStruct> > _z_bounds;

    /// Swept BVs computed during last call to is_contact/update_contacts()
    std::map<CollisionGeometryPtr, BVPtr> _swept_BVs;

    static BVPtr construct_bounding_sphere(CollisionGeometryPtr cg);
    void sort_AABBs(const std::vector<RigidBodyPtr>& rigid_bodies, double dt);
    void update_bounds_vector(std::vector<std::pair<double, BoundsStruct> >& bounds, AxisType axis, double dt);
    void build_bv_vector(const std::vector<RigidBodyPtr>& rigid_bodies, std::vector<std::pair<double, BoundsStruct> >& bounds);
    BVPtr get_swept_BV(CollisionGeometryPtr geom, BVPtr bv, double dt);

    double calc_max_dist_per_t(RigidBodyPtr rb, const Ravelin::Vector3d& n, double rmax);
    static double calc_max_velocity(RigidBodyPtr rb, const Ravelin::Vector3d& n, double rmax);
    bool intersect_BV_trees(boost::shared_ptr<BV> a, boost::shared_ptr<BV> b, const Ravelin::Transform3d& aTb, CollisionGeometryPtr geom_a, CollisionGeometryPtr geom_b);
    static Event create_contact(CollisionGeometryPtr a, CollisionGeometryPtr b, const Point3d& point, const Ravelin::Vector3d& normal);

    template <class OutputIterator>
    OutputIterator intersect_BV_leafs(BVPtr a, BVPtr b, const Ravelin::Transform3d& aTb, CollisionGeometryPtr geom_a, CollisionGeometryPtr geom_b, OutputIterator output_begin) const;

    template <class OutputIterator>
    OutputIterator find_contacts_sphere_sphere(CollisionGeometryPtr cgA, CollisionGeometryPtr cgB, OutputIterator output_begin);

    template <class OutputIterator>
    OutputIterator find_contacts_not_separated(CollisionGeometryPtr cgA, CollisionGeometryPtr cgB, OutputIterator output_begin);

    template <class OutputIterator>
    OutputIterator find_contacts_separated(CollisionGeometryPtr cgA, CollisionGeometryPtr cgB, double min_dist, OutputIterator output_begin);

    template <class RandomAccessIterator>
    void insertion_sort(RandomAccessIterator begin, RandomAccessIterator end);
}; // end class

#include "CCD.inl"

} // end namespace

#endif

