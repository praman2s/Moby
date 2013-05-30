/****************************************************************************
 * Copyright 2005 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#ifndef _FS_AB_ALGORITHM_H
#define _FS_AB_ALGORITHM_H

#include <queue>
#include <Ravelin/SpatialABInertiad.h>

namespace Moby {

/// Implements Featherstone's algorithm for forward dynamics
/**
 * Implements Featherstone's algorithm for articulated bodies.  Featherstone's 
 * algorithm runs in O(n) time [n = # of joints].  This implementation is based
 * on Brian Mirtich's Ph. D. thesis, and remains pretty consistent with it. 
 * There are a couple of changes, to produce a nice implementation.  The user 
 * need not be concerned with these issues, but they are useful to know for 
 * debugging.
 * <ol>
 * <li>Mirtich labels his links from 1..n, and considers the base to be link 0; 
 * the total number of links is considered to be n, rather than n+1.  I make 
 * the total number of links n+1 and treat the links the same as the base.  I 
 * do this so that the user thinks of the base as a link for purposes of link 
 * connectivity.</li>
 * <li>Mirtich labels his joints from 0..n-1</li>.  When labeling the link in 
 * Mirtich's style, link i and joint i match up (joint i is link i's inner 
 * joint).  When labeling the link in my style, joint i-1 is the corresponding 
 * joint for link i.</li>
 * </ol>
 * Note that one critical note for manipulator setup is that the base is the 
 * first link in the list of links.
 */
class FSABAlgorithm 
{
  friend class RCArticulatedBody;

  public:
    FSABAlgorithm();
    ~FSABAlgorithm() {}
    RCArticulatedBodyPtr get_body() const { return RCArticulatedBodyPtr(_body); }
    void set_body(RCArticulatedBodyPtr body) { _body = body; }
    void calc_fwd_dyn();
    void apply_generalized_impulse(const Ravelin::VectorNd& gj);
    void apply_impulse(const Ravelin::Wrenchd& j, RigidBodyPtr link);

    /// The body that this algorithm operates on
    boost::weak_ptr<RCArticulatedBody> _body;

    /// The spatial velocities
    std::vector<Ravelin::Twistd> _v;

    /// The spatial accelerations
    std::vector<Ravelin::Twistd> _a;

    /// The articulated body inertias
    std::vector<Ravelin::SpatialABInertiad> _I;

    /// The articulated body spatial zero accelerations
    std::vector<Ravelin::Wrenchd> _Z;

    /// Vector of link velocity updates
    std::vector<Ravelin::Twistd> _dv;

    /// The spatial coriolis vectors
    std::vector<Ravelin::Twistd> _c;

    /// The expressions I*s
    std::vector<Ravelin::MatrixNd> _Is;

    /// Cholesky factorizations sIs
    std::vector<Ravelin::MatrixNd> _sIs;

    /// SVDs of sIs
    std::vector<Ravelin::MatrixNd> _usIs, _vsIs;
    std::vector<Ravelin::VectorNd> _ssIs;

    /// Determines whether the equations for a joint are rank deficient 
    std::vector<bool> _rank_deficient;

    /// The temporary expression Q - I*s'*c - s'*Z
    std::vector<Ravelin::VectorNd> _mu;

  private:
    // pointer to the linear algebra routines
    boost::shared_ptr<Ravelin::LinAlgd> _LA;

    /// work variables 
    Ravelin::VectorNd _workv, _workv2, _sTY, _qd_delta, _sIsmu, _Qi, _Q;
    Ravelin::MatrixNd _sIss, _workM;
    std::vector<Ravelin::Wrenchd> _Y;

    /// processed vector
    std::vector<bool> _processed;

    static double sgn(double x);
    static void push_children(RigidBodyPtr link, std::queue<RigidBodyPtr>& q);
    void apply_coulomb_joint_friction(RCArticulatedBodyPtr body);
    void calc_impulse_dyn(RCArticulatedBodyPtr body);
    void apply_generalized_impulse(unsigned index, const std::vector<std::vector<Ravelin::Wrenchd> >& sTI, Ravelin::VectorNd& vgj);
    void set_spatial_velocities(RCArticulatedBodyPtr body);
    void calc_spatial_accelerations(RCArticulatedBodyPtr body);
    void calc_spatial_zero_accelerations(RCArticulatedBodyPtr body);
    void calc_spatial_inertias(RCArticulatedBodyPtr body);
    void calc_spatial_coriolis_vectors(RCArticulatedBodyPtr body);
    Ravelin::VectorNd& solve_sIs(unsigned idx, const Ravelin::VectorNd& v, Ravelin::VectorNd& result) const;
    Ravelin::MatrixNd& solve_sIs(unsigned idx, const Ravelin::MatrixNd& v, Ravelin::MatrixNd& result) const;
    Ravelin::MatrixNd& transpose_solve_sIs(unsigned idx, const std::vector<Ravelin::Twistd>& m, Ravelin::MatrixNd& result) const;
}; // end class
} // end namespace

#endif

