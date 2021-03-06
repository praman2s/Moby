#ifndef RESTINGCONTACTHANDLER_H
#define RESTINGCONTACTHANDLER_H

#include <list>
#include <vector>
#include <map>
#include <Ravelin/LinAlgd.h>
#include <Moby/Base.h>
#include <Moby/Types.h>
#include <Moby/LCP.h>
#include <Moby/Event.h>
#include <Moby/ContactProblemData.h>

namespace Moby {

/// Defines the mechanism for handling impact contacts
class RestingContactHandler
{
  public:
    RestingContactHandler();
    void process_events(const std::vector<Event>& contacts);

  private:
    static DynamicBodyPtr get_super_body(SingleBodyPtr sb);
    void apply_model(const std::vector<Event>& contacts);
    void apply_model_to_connected_contacts(const std::list<Event*>& contacts);
    static void compute_problem_data(ContactProblemData& epd);
    void solve_lcp(ContactProblemData& epd, Ravelin::VectorNd& z);
    double calc_ke(ContactProblemData& epd, const Ravelin::VectorNd& z);
    void apply_forces(const ContactProblemData& epd) const;
    static void contact_select(const std::vector<int>& alpha_c_indices, const std::vector<int>& beta_nbeta_c_indices, const Ravelin::VectorNd& x, Ravelin::VectorNd& alpha_c, Ravelin::VectorNd& beta_c);
    static void contact_select(const std::vector<int>& alpha_c_indices, const std::vector<int>& beta_nbeta_c_indices, const Ravelin::MatrixNd& m, Ravelin::MatrixNd& alpha_c_rows, Ravelin::MatrixNd& beta_c_rows);
    static double sqr(double x) { return x*x; }
    bool solve_lcp(const Ravelin::MatrixNd& M, const Ravelin::VectorNd& q, Ravelin::VectorNd& z);

    Ravelin::LinAlgd _LA;
    LCP _lcp;
}; // end class

/// Exception thrown when trying to initialize a fixed size vector/matrix with the wrong size
class RestingContactFailException : public std::runtime_error
{
  public:
    RestingContactFailException(const std::list<Event*>& contact_events) : std::runtime_error("post-event Kinetic Energy exceeds pre-event Kinetic Energy!") { events = contact_events; type = EnergyTolerance;}
    RestingContactFailException(const Ravelin::VectorNd& lcpv, const Ravelin::MatrixNd& lcpM) : std::runtime_error("Unable to solve resting contact LCP!") { v = lcpv; M = lcpM; type = LCPFail;}

    virtual ~RestingContactFailException() throw() { }

    enum FailType{
      LCPFail,
      EnergyTolerance
    };

    FailType type;
    std::list<Event*> events;
    Ravelin::VectorNd v;
    Ravelin::MatrixNd M;
}; // end class

} // end namespace

#endif // RESTINGCONTACTHANDLER_H
