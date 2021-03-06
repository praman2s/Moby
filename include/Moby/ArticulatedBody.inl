/****************************************************************************
 * Copyright 2011 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

/// Gets joint limit events 
template <class OutputIterator>
OutputIterator ArticulatedBody::find_limit_events(OutputIterator output_begin) const 
{
  for (unsigned i=0; i< _joints.size(); i++)
    for (unsigned j=0; j< _joints[i]->num_dof(); j++)
    {
      // get the current joint position and velocity
      double q = _joints[i]->q[j];

      // setup an event for this joint/dof in case we need it
      Event e;
      e.event_type = Event::eLimit;
      e.limit_joint = _joints[i];
      e.limit_dof = j;
      e.limit_epsilon = _joints[i]->limit_restitution;

      // check whether we are already at a limit
      if (q >= _joints[i]->hilimit[j])
      {
        // add event for upper limit
        e.limit_upper = true;
        *output_begin++ = e;
      }
      if (q <= _joints[i]->lolimit[j])
      {
        e.limit_upper = false;
        *output_begin++ = e;
      }
    }

  return output_begin;
}

