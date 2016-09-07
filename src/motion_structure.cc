/**
 @file    motion_structure.h
 @author  Alexander W. Winkler (winklera@ethz.ch)
 @date    Jun 6, 2016
 @brief   Defines the MotionStructure class.
 */

// motion_ref these should both be removed (see UML, only uni-association)
#include <xpp/zmp/motion_structure.h>
#include <xpp/zmp/com_spline.h> // spline times

#include <xpp/hyq/support_polygon_container.h>
#include <xpp/zmp/phase_info.h>

namespace xpp {
namespace zmp {

MotionStructure::MotionStructure ()
{
  // TODO Auto-generated constructor stub
}

MotionStructure::MotionStructure (const LegIDVec& start_legs,
                                  const LegIDVec& step_legs,
                                  const PhaseVec& phases,
                                  double dt)
{
  Init(start_legs, step_legs, phases, dt);
}

MotionStructure::~MotionStructure ()
{
  // TODO Auto-generated destructor stub
}

void
MotionStructure::Init (const StartStance& start_stance,
                       const LegIDVec& step_legs,
                       const SplineTimes& times,
                       bool insert_initial_stance,
                       bool insert_final_stance)
{
  // motion_ref add IDs and even footholds here
  phases_.clear();

  int id = 0;
  int n_completed_steps = 0;

  if (insert_initial_stance) {
    PhaseInfo phase(PhaseInfo::kStancePhase, n_completed_steps, id++, times.t_stance_initial_);
    phases_.push_back(phase);
  }

  // steps
  for (int step=0; step<step_legs.size(); ++step) {
    PhaseInfo phase(PhaseInfo::kStepPhase, n_completed_steps++, id++, times.t_swing_);
    phases_.push_back(phase);
  }

  if (insert_final_stance) {
    PhaseInfo phase(PhaseInfo::kStancePhase, n_completed_steps, id++, 0.55);
    phases_.push_back(phase);
  }

}


void
MotionStructure::Init (const LegIDVec& start_legs, const LegIDVec& step_legs,
                       const PhaseVec& phases, double dt)
{
  start_stance_   = start_legs;
  steps_          = step_legs;
  phases_         = phases;
  dt_             = dt;
  cache_needs_updating_ = true;
}

MotionStructure::MotionInfoVec
MotionStructure::GetContactInfoVec () const
{
  if (cache_needs_updating_) {
    cached_motion_vector_ = CalcContactInfoVec();
    cache_needs_updating_ = false;
  }

  return cached_motion_vector_;
}

MotionStructure::MotionInfoVec
MotionStructure::CalcContactInfoVec () const
{
  xpp::hyq::SupportPolygonContainer foothold_container;
  foothold_container.Init(start_stance_, steps_);
  auto supp = foothold_container.AssignSupportPolygonsToPhases(phases_);

  MotionInfoVec info;

  double t_global = 0;
  for (auto phase : phases_) {

    auto stance_feet = supp.at(phase.id_).GetFootholds();

    int nodes_in_phase = std::floor(phase.duration_/dt_);

    for (int k=0; k<nodes_in_phase; ++k ) {

      MotionInfo contact_info;
      contact_info.time_ = t_global+k*dt_;

      for (const auto& f : stance_feet)
        contact_info.contacts.push_back(MotionInfo::Contact(f.id, f.leg));

      info.push_back(contact_info);
    }

    t_global += phase.duration_;
  }

  // even though the last footstep doesn't create a support polygon, still include
  // this last time instance with contact configuration
  MotionInfo final_contacts;
  final_contacts.time_ = t_global;
  for (const auto& f : foothold_container.GetFinalFootholds())
    final_contacts.contacts.push_back(MotionInfo::Contact(f.id, f.leg));

  info.push_back(final_contacts);

  return info;
}

int
MotionStructure::GetTotalNumberOfDiscreteContacts () const
{
  auto contact_info_vec = GetContactInfoVec();

  int i = 0;
  for (auto node : contact_info_vec)
    i += node.contacts.size();

  return i;
}

MotionStructure::PhaseVec
MotionStructure::GetPhases () const
{
  return phases_;
}

} /* namespace zmp */
} /* namespace xpp */
