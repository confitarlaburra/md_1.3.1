/**
 * @file quartic_bond_interaction.cc
 * template methods of Quartic_bond_interaction.
 */

#include "../../stdheader.h"

#include "../../algorithm/algorithm.h"
#include "../../topology/topology.h"
#include "../../simulation/simulation.h"
#include "../../configuration/configuration.h"
#include "../../interaction/interaction.h"

#include "../../math/periodicity.h"

// interactions
#include "../../interaction/interaction_types.h"
#include "quartic_bond_interaction.h"

#include "../../util/template_split.h"
#include "../../util/debug.h"

#undef MODULE
#undef SUBMODULE
#define MODULE interaction
#define SUBMODULE bonded

/**
 * calculate quartic bond forces and energies.
 */
template<math::boundary_enum B, math::virial_enum V>
static int _calculate_quartic_bond_interactions(topology::Topology &topo,
						configuration::Configuration &conf,
						simulation::Simulation &sim,
						std::vector<interaction::bond_type_struct> const & param)
{
  
  math::Periodicity<B> periodicity(conf.current().box);

  // loop over the bonds
  std::vector<topology::two_body_term_struct>::iterator b_it =
    topo.solute().bonds().begin(),
    b_to = topo.solute().bonds().end();

  math::VArray &pos   = conf.current().pos;
  math::VArray &force = conf.current().force;
  math::Vec v, f;

  double e;

  for( ; b_it != b_to; ++b_it){
    periodicity.nearest_image(pos(b_it->i), pos(b_it->j), v);

    double dist2 = abs2(v);
    
    assert(unsigned(b_it->type) < param.size());
    const double r02 = param[b_it->type].r0 *
      param[b_it->type].r0;

    DEBUG(7, "bond " << b_it->i << "-" << b_it->j
	  << " type " << b_it->type);
    DEBUG(10, "K " << param[b_it->type].K
	  << " r02 " << r02);
    DEBUG(10, "DF " << (-param[b_it->type].K *
			(dist2 - r02)) << "\n" << math::v2s(v));

    f = v * (-param[b_it->type].K *
	     (dist2 - r02));
    
    force(b_it->i) += f;
    force(b_it->j) -= f;

    // if (V == math::atomic_virial){
      for(int a=0; a<3; ++a)
	for(int c=0; c<3; ++c)
	  conf.current().virial_tensor(a, c) += 
	    v(a) * f(c);
      
      DEBUG(7, "\tatomic virial done");
      // }

    e = 0.25 * param[b_it->type].K *
      (dist2 -r02) * (dist2 - r02);

    DEBUG(10, "energy: " << e);
    DEBUG(10, "bond energy size: " << unsigned(conf.current().energies.bond_energy.size()));
    DEBUG(10, "energy group size: " << unsigned(topo.atom_energy_group().size()));

    assert(conf.current().energies.bond_energy.size() >
	   topo.atom_energy_group()[b_it->i]);
    
    conf.current().energies.
      bond_energy[topo.atom_energy_group()
		  [b_it->i]] += e;
  }
 
  return 0;
}

int interaction::Quartic_Bond_Interaction
::calculate_interactions(topology::Topology &topo,
			 configuration::Configuration &conf,
			 simulation::Simulation &sim)
{
  m_timer.start();

  SPLIT_VIRIAL_BOUNDARY(_calculate_quartic_bond_interactions, topo, conf, sim, m_parameter);
  
  m_timer.stop();
  
  return 0;
}

