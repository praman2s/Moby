/****************************************************************************
 * Copyright 2005 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifndef _TRIANGLE_MESH_PRIMITIVE_H
#define _TRIANGLE_MESH_PRIMITIVE_H

#include <queue>
#include <fstream>
#include <iostream>
#include <cmath>
#include <list>
#include <string>
#include <Moby/Types.h>
#include <Moby/Primitive.h>

namespace Moby {

/// Represents a triangle mesh "primitive" for inertia properties, collision detection, and visualization
class TriangleMeshPrimitive : public Primitive
{
  public: 
    TriangleMeshPrimitive();
    TriangleMeshPrimitive(const std::string& filename, bool center = true);
    TriangleMeshPrimitive(const std::string& filename, const Ravelin::Pose3d& T, bool center = true);
    void set_edge_sample_length(double len);
    virtual osg::Node* create_visualization();

    /// Gets the length of an edge in the mesh above which point sub-samples are created
    double get_edge_sample_length() const { return _edge_sample_length; }

    virtual double calc_signed_dist(const Point3d& p);
    virtual void load_from_xml(boost::shared_ptr<const XMLTree> node, std::map<std::string, BasePtr>& id_map);  
    virtual void save_to_xml(XMLTreePtr node, std::list<boost::shared_ptr<const Base> >& shared_objects) const;
    virtual BVPtr get_BVH_root(CollisionGeometryPtr geom);
    virtual void get_vertices(std::vector<Point3d>& vertices);
    virtual double calc_dist_and_normal(const Point3d& p, Ravelin::Vector3d& normal) const;
    virtual const std::pair<boost::shared_ptr<const IndexedTriArray>, std::list<unsigned> >& get_sub_mesh(BVPtr bv);
    virtual boost::shared_ptr<const IndexedTriArray> get_mesh() { return _mesh; }
    virtual void set_deformable(bool flag);
    void set_mesh(boost::shared_ptr<const IndexedTriArray> mesh);
    virtual void set_pose(const Ravelin::Pose3d& T);
    virtual double calc_signed_dist(boost::shared_ptr<const Primitive> p, boost::shared_ptr<const Ravelin::Pose3d> pose_this, boost::shared_ptr<const Ravelin::Pose3d> pose_p, Point3d& pthis, Point3d& pp) const;

  private:
    void center();
    virtual void calc_mass_properties();

    /// Determines whether we convexify the mesh for inertial calculations
    bool _convexify_inertia;

    /// The root bounding volume around the primitive (indexed by geometry); can differ based on whether the geometry is deformable
    std::map<CollisionGeometryPtr, BVPtr> _roots;
    
    /// The underlying mesh
    /**
     * \note the mesh changes when the primitive's transform changes
     */
    boost::shared_ptr<const IndexedTriArray> _mesh;

    /// Edge sample length above which pseudo-vertices are added
    double _edge_sample_length;

    /// Thick triangle structure augmented with mesh data
    class AThickTri : public ThickTriangle
    {
      public:
        AThickTri(const Triangle& tri, double tol) : ThickTriangle(tri, tol) {}
        boost::shared_ptr<const IndexedTriArray> mesh;  // the mesh that this triangle came from
        unsigned tri_idx;             // the index of this triangle
    };

    void construct_mesh_vertices(boost::shared_ptr<const IndexedTriArray> mesh, CollisionGeometryPtr geom, boost::shared_ptr<const Ravelin::Pose3d> P);
    void build_BB_tree(CollisionGeometryPtr geom);
    void split_tris(const Point3d& point, const Ravelin::Vector3d& normal, const IndexedTriArray& orig_mesh, const std::list<unsigned>& ofacets, std::list<unsigned>& pfacets, std::list<unsigned>& nfacets);
    bool split(boost::shared_ptr<const IndexedTriArray> mesh, BVPtr source, BVPtr& tgt1, BVPtr& tgt2, const Ravelin::Vector3d& axis);
    static bool is_degen_point_on_tri(boost::shared_ptr<AThickTri> tri, const Point3d& p);

    template <class InputIterator, class OutputIterator>
    static OutputIterator get_vertices(const IndexedTriArray& tris, InputIterator fselect_begin, InputIterator fselect_end, OutputIterator output, boost::shared_ptr<const Ravelin::Pose3d> P);

    /// Mapping from BVs to triangles contained within (not necessary to index this per geometry)
    std::map<BVPtr, std::list<unsigned> > _mesh_tris;

    /// Map from the geometry to the vector of vertices (w/transform and intersection tolerance applied), if any
    std::vector<Point3d> _vertices;

    /// Mapping from BVs to vertex indices contained within (not necessary to index this per geometry)
    std::map<BVPtr, std::list<unsigned> > _mesh_vertices;

    /// Mapping from BV leafs to thick triangles (not necessary to index this per geometry)
    std::map<BVPtr, std::list<boost::shared_ptr<AThickTri> > > _tris;

    /// List of triangles covered by a bounding volume
    std::pair<boost::shared_ptr<const IndexedTriArray>, std::list<unsigned> > _smesh;
}; // end class

#include "TriangleMeshPrimitive.inl"

} // end namespace 

#endif

