/**
 * @file configuration.cc
 * methods definition
 */

#ifdef XXMPI
#include <mpi.h>
#endif

#include "../stdheader.h"

#include "../configuration/configuration_global.h"

#include "../algorithm/algorithm.h"
#include "../topology/topology.h"
#include "../configuration/configuration.h"
#include "../configuration/mesh.h"
#include "../configuration/influence_function.h"
#include "../simulation/simulation.h"
#include "../simulation/multibath.h"
#include "../simulation/parameter.h"

#include "../math/periodicity.h"
#include "../math/boundary_checks.h"
#include "../util/template_split.h"

#include "configuration.h"

#ifdef OMP
#include <omp.h>
#endif

#undef MODULE
#undef SUBMODULE
#define MODULE configuration
#define SUBMODULE configuration

double configuration_ver = 0.10;

/**
 * Constructor
 */
configuration::Configuration::Configuration() {
  m_current = &m_state1;
  m_old = &m_state2;

  current().virial_tensor = 0.0;
  old().virial_tensor = 0.0;

  current().kinetic_energy_tensor = 0.0;
  old().kinetic_energy_tensor = 0.0;

  current().pressure_tensor = 0.0;
  old().pressure_tensor = 0.0;

  current().sasa_tot = 0.0;
  old().sasa_tot = 0.0;
  current().sasa_buriedvol_tot = 0.0;
  old().sasa_buriedvol_tot = 0.0;

  for (unsigned int k = 0; k < special().eds.virial_tensor_endstates.size(); ++k) {
    special().eds.virial_tensor_endstates[k] = 0.0;
  }
  
  special().shake_failure_occurred = false;
}

/**
 * copy constructor
 */
configuration::Configuration::Configuration
(
 configuration::Configuration const & conf
 )
{
  m_current = &m_state1;
  m_old = &m_state2;

  current().virial_tensor = conf.current().virial_tensor;
  old().virial_tensor = conf.old().virial_tensor;

  current().kinetic_energy_tensor = conf.current().kinetic_energy_tensor;
  old().kinetic_energy_tensor = conf.old().kinetic_energy_tensor;

  current().pressure_tensor = conf.current().pressure_tensor;
  old().pressure_tensor = conf.old().pressure_tensor;
  
  for (unsigned int k = 0; k < special().eds.virial_tensor_endstates.size(); ++k) {
    special().eds.virial_tensor_endstates[k] =
            conf.special().eds.virial_tensor_endstates[k];
  }


  current().pos = conf.current().pos;
  old().pos = conf.old().pos;
  current().posV = conf.current().posV;
  old().posV = conf.old().posV;
  current().vel = conf.current().vel;
  old().vel = conf.old().vel;
  current().force = conf.current().force;
  current().constraint_force = conf.current().constraint_force;
  old().force = conf.old().force;
  old().constraint_force = conf.current().constraint_force;
  current().stochastic_integral = conf.current().stochastic_integral;
  old().stochastic_integral = conf.old().stochastic_integral;
  current().stochastic_seed = conf.current().stochastic_seed;
  old().stochastic_seed = conf.old().stochastic_seed;
  
  current().box = conf.current().box;
  old().box = conf.old().box;
  
  current().energies = conf.current().energies;
  old().energies = conf.old().energies;
  current().averages = conf.current().averages;
  old().averages = conf.old().averages;
  
  current().perturbed_energy_derivatives =
    conf.current().perturbed_energy_derivatives;
  old().perturbed_energy_derivatives =
    conf.old().perturbed_energy_derivatives;

  current().sasa_area = conf.current().sasa_area;
  old().sasa_area = conf.old().sasa_area;
  current().sasa_buriedvol = conf.current().sasa_buriedvol;
  old().sasa_buriedvol = conf.old().sasa_buriedvol;
  current().gvol = conf.current().gvol;
  old().gvol = conf.old().gvol;
  current().dgvol = conf.current().dgvol;
  old().dgvol = conf.old().dgvol;

  current().sasa_tot = conf.current().sasa_tot;
  old().sasa_tot = conf.old().sasa_tot;
  current().sasa_buriedvol_tot = conf.current().sasa_buriedvol_tot;
  old().sasa_buriedvol_tot = conf.old().sasa_buriedvol_tot;

  special().dihangle_trans.dihedral_angle_minimum = conf.special().dihangle_trans.dihedral_angle_minimum;
  special().dihangle_trans.old_minimum = conf.special().dihangle_trans.old_minimum;
  special().dihangle_trans.resid = conf.special().dihangle_trans.resid;
  special().dihangle_trans.i = conf.special().dihangle_trans.i;
  special().dihangle_trans.j = conf.special().dihangle_trans.j;
  special().dihangle_trans.k = conf.special().dihangle_trans.k;
  special().dihangle_trans.l = conf.special().dihangle_trans.l;
  
  special().umbrellas = conf.special().umbrellas;
  special().flexible_constraint = conf.special().flexible_constraint;
  
  special().jvalue_av = conf.special().jvalue_av;
  special().jvalue_curr = conf.special().jvalue_curr;
  special().jvalue_epsilon = conf.special().jvalue_epsilon;
  
  special().distanceres.av = conf.special().distanceres.av;
  special().distanceres.energy = conf.special().distanceres.energy;
  special().distanceres.d = conf.special().distanceres.d;
  
  special().distancefield.ngrid = conf.special().distancefield.ngrid;
  special().distancefield.distance = conf.special().distancefield.distance;
  special().distancefield.dist = conf.special().distancefield.dist;
  special().distancefield.energy = conf.special().distancefield.energy;
  special().distancefield.energy_deriv = conf.special().distancefield.energy_deriv;
  
  special().pscale = conf.special().pscale;

  special().orderparamres.S2_avg = conf.special().orderparamres.S2_avg;
  special().orderparamres.energy = conf.special().orderparamres.energy;
  special().orderparamres.Q_avg = conf.special().orderparamres.Q_avg;
  special().orderparamres.D_avg = conf.special().orderparamres.D_avg;
  special().orderparamres.Q_winavg = conf.special().orderparamres.Q_winavg;
  special().orderparamres.D_winavg = conf.special().orderparamres.D_winavg;

  special().rdc = conf.special().rdc;
 
  special().rottrans_constr = conf.special().rottrans_constr;

  // if this works just like this, why do we need to explicitly copy the virial tensor?
  special().eds = conf.special().eds;
  
  special().lattice_shifts = conf.special().lattice_shifts;
  
  special().shake_failure_occurred = conf.special().shake_failure_occurred;
  
  special().force_groups = conf.special().force_groups;
  
  boundary_type = conf.boundary_type;
}

/**
 * operator equal
 */
configuration::Configuration & configuration::Configuration::operator=
(
 configuration::Configuration const & conf
 )
{
  m_current = &m_state1;
  m_old = &m_state2;

  current().virial_tensor = conf.current().virial_tensor;
  old().virial_tensor = conf.old().virial_tensor;

  current().kinetic_energy_tensor = conf.current().kinetic_energy_tensor;
  old().kinetic_energy_tensor = conf.old().kinetic_energy_tensor;

  current().pressure_tensor = conf.current().pressure_tensor;
  old().pressure_tensor = conf.old().pressure_tensor;

  for (unsigned int k = 0; k < special().eds.virial_tensor_endstates.size(); ++k) {
    special().eds.virial_tensor_endstates[k] =
            conf.special().eds.virial_tensor_endstates[k];
  }

  
  current().pos = conf.current().pos;
  old().pos = conf.old().pos;
  current().posV = conf.current().posV;
  old().posV = conf.old().posV;
  current().vel = conf.current().vel;
  old().vel = conf.old().vel;
  current().force = conf.current().force;
  old().force = conf.old().force;
  current().stochastic_integral = conf.current().stochastic_integral;
  old().stochastic_integral = conf.old().stochastic_integral;
  current().stochastic_seed = conf.current().stochastic_seed;
  old().stochastic_seed = conf.old().stochastic_seed;
  
  current().box = conf.current().box;
  old().box = conf.old().box;
  
  current().energies = conf.current().energies;
  old().energies = conf.old().energies;
  current().averages = conf.current().averages;
  old().averages = conf.old().averages;
  
  current().perturbed_energy_derivatives =
    conf.current().perturbed_energy_derivatives;
  old().perturbed_energy_derivatives =
    conf.old().perturbed_energy_derivatives;

  current().sasa_area = conf.current().sasa_area;
  old().sasa_area = conf.old().sasa_area;
  current().sasa_buriedvol = conf.current().sasa_buriedvol;
  old().sasa_buriedvol = conf.old().sasa_buriedvol;
  current().gvol = conf.current().gvol;
  old().gvol = conf.old().gvol;
  current().dgvol = conf.current().dgvol;
  old().dgvol = conf.old().dgvol;

  current().sasa_tot = conf.current().sasa_tot;
  old().sasa_tot = conf.old().sasa_tot;
  current().sasa_buriedvol_tot = conf.current().sasa_buriedvol_tot;
  old().sasa_buriedvol_tot = conf.old().sasa_buriedvol_tot;

  special().dihangle_trans.dihedral_angle_minimum = conf.special().dihangle_trans.dihedral_angle_minimum;
  special().dihangle_trans.old_minimum = conf.special().dihangle_trans.old_minimum;
  special().dihangle_trans.resid = conf.special().dihangle_trans.resid;
  special().dihangle_trans.i = conf.special().dihangle_trans.i;
  special().dihangle_trans.j = conf.special().dihangle_trans.j;
  special().dihangle_trans.k = conf.special().dihangle_trans.k;
  special().dihangle_trans.l = conf.special().dihangle_trans.l;
  
  special().umbrellas = conf.special().umbrellas;
  special().flexible_constraint = conf.special().flexible_constraint;
  
  special().jvalue_av = conf.special().jvalue_av;
  special().jvalue_curr = conf.special().jvalue_curr;
  special().jvalue_epsilon = conf.special().jvalue_epsilon;
  
  special().distanceres.av = conf.special().distanceres.av;
  special().distanceres.energy = conf.special().distanceres.energy;
  special().distanceres.d = conf.special().distanceres.d;
 
  special().distancefield.ngrid = conf.special().distancefield.ngrid;
  special().distancefield.distance = conf.special().distancefield.distance;
  special().distancefield.dist = conf.special().distancefield.dist;
  special().distancefield.energy = conf.special().distancefield.energy;
  special().distancefield.energy_deriv = conf.special().distancefield.energy_deriv;
 
  special().rdc = conf.special().rdc;

  special().pscale = conf.special().pscale;
  
  special().rottrans_constr = conf.special().rottrans_constr;

  special().eds = conf.special().eds;
  
  special().lattice_shifts = conf.special().lattice_shifts;
  
  special().shake_failure_occurred = conf.special().shake_failure_occurred;
  
  special().force_groups = conf.special().force_groups;
  
  boundary_type = conf.boundary_type;

  return *this;
}

void configuration::Configuration::init(topology::Topology const & topo,
					simulation::Parameter & param,
					bool gather)
{
  // resize the energy arrays
  const unsigned int num = unsigned(topo.energy_groups().size());
  const unsigned int numb = unsigned(param.multibath.multibath.size());
  
  DEBUG(5, "number of energy groups: " << num 
	<< "\nnumber of baths: " << numb);

  current().energies.resize(num, numb);
  old().energies.resize(num, numb);
  if (param.force.force_groups) {
    special().force_groups.resize(num, 
            std::vector<math::VArray>(num, math::VArray(
            topo.num_atoms(), math::Vec(0.0, 0.0, 0.0))));
  }

  // resize sasa vectors
  const unsigned int num_sasa_atoms = topo.sasa_parameter().size();
  DEBUG(5, "Number of sasa atoms: " << num_sasa_atoms);

  current().sasa_area.resize(num_sasa_atoms, 0.0);
  old().sasa_area.resize(num_sasa_atoms, 0.0);
  current().sasa_buriedvol.resize(num_sasa_atoms, 0.0);
  old().sasa_buriedvol.resize(num_sasa_atoms, 0.0);
  current().gvol.resize(num_sasa_atoms, 0.0);
  old().gvol.resize(num_sasa_atoms, 0.0);
  current().dgvol.resize(num_sasa_atoms, 0.0);
  old().dgvol.resize(num_sasa_atoms, 0.0);

  // check whether this can really stay here! see resize function below
  special().eds.force_endstates.resize(param.eds.numstates);
  for (unsigned int i = 0; i < special().eds.force_endstates.size(); i++){
    special().eds.force_endstates[i].resize(topo.num_atoms());
  }  
  special().eds.virial_tensor_endstates.resize(param.eds.numstates);
  current().energies.eds_vi.resize(param.eds.numstates);
  current().energies.eds_vi_special.resize(param.eds.numstates);
  old().energies.eds_vi.resize(param.eds.numstates);
  old().energies.eds_vi_special.resize(param.eds.numstates);
  
  current().energies.ewarn(param.ewarn.limit);
  old().energies.ewarn(param.ewarn.limit);

  current().perturbed_energy_derivatives.resize(num, numb);
  old().perturbed_energy_derivatives.resize(num, numb);

  current().averages.resize(topo, *this, param);
  old().averages.resize(topo, *this, param);

  // possibly resize the dihedral angle monitoring array
  // initialize or set to such a value that it is recalculated in
  // the first step, initialization would require an additional function
  // which can be done only after reading of the coordinates. Would be a
  // nicer solution, but also requires the parameters...
  if(param.print.monitor_dihedrals){
    special().dihangle_trans.dihedral_angle_minimum.resize
      (topo.solute().dihedrals().size(), 4*math::Pi);
    special().dihangle_trans.old_minimum.resize
      (topo.solute().dihedrals().size(), 0.0);
    special().dihangle_trans.resid.resize
      (topo.solute().dihedrals().size(), 0);
    special().dihangle_trans.i.resize
      (topo.solute().dihedrals().size(), 0);
    special().dihangle_trans.j.resize
      (topo.solute().dihedrals().size(), 0);
    special().dihangle_trans.k.resize
      (topo.solute().dihedrals().size(), 0);
    special().dihangle_trans.l.resize
      (topo.solute().dihedrals().size(), 0);
  }
  
  if (param.constraint.solute.algorithm == simulation::constr_flexshake &&
      special().flexible_constraint.flexible_vel.size() == 0){

    special().flexible_constraint.flexible_vel.resize(topo.solute().distance_constraints().size() +
				  topo.perturbed_solute().distance_constraints().size());

    special().flexible_constraint.flexible_ekin.resize(numb);
  }

  if(param.nemd.nemd != simulation::nemd_off){
    special().nemd_conf.Px = 0.0; //To accumulate the momemtum
    special().nemd_conf.counter = 0; 
    unsigned int num_grid = 2 * param.nemd.slabnum;
    special().nemd_conf.stored_data_per_bin.clear();
    for(unsigned int i = 0; i < num_grid; ++i) {
      special().nemd_conf.stored_data_per_bin.push_back(0.0);
    }
    for(unsigned int i = 0; i < topo.num_atoms(); ++i) {
      special().nemd_conf.vel_per_atom.push_back(0.0);
      special().nemd_conf.dvx_per_atom.push_back(0.0);
    }
  }
  
  
  // resize the arrays
  // to make scripting easier...
  resize(topo.num_atoms());

  // gather the molecules!
  // check box size

  // mc: bugfix: chargegroups should be gathered
  //             problem if submolecules are set to 1 atom
  //             (for whatever esoteric reasons)

  if (gather){
    switch(boundary_type){
      case math::vacuum:
	break;
      case math::rectangular:
	{
	  math::Periodicity<math::rectangular> periodicity(current().box);
	  // periodicity.gather_molecules_into_box(*this, topo);
	  periodicity.gather_chargegroups(*this, topo);
	  
	  break;
	}
      case math::truncoct:
      case math::triclinic:
	{
	  // NO CUTOFF CHECK -- IMPLEMENT!!!
	  math::Periodicity<math::triclinic> periodicity(current().box);
	  // periodicity.gather_molecules_into_box(*this, topo);
	  periodicity.gather_chargegroups(*this, topo);
	  
	  break;
	}
      default:
	std::cout << "wrong periodic boundary conditions!";
	io::messages.add("wrong PBC!", "In_Configuration", io::message::error);
    }
  }

  // check periodicity
  if (!param.multicell.multicell) {
    if (!math::boundary_check_cutoff(current().box, boundary_type, param.pairlist.cutoff_long)) {
      io::messages.add("box is too small: not twice the cutoff!",
              "configuration",
              io::message::error);
    }
  }

  if (boundary_type != math::vacuum){
    if (param.centreofmass.remove_rot){
      io::messages.add("disabling removing of centre of mass rotation (PBC)",
		       "configuration",
		       io::message::notice);
      param.centreofmass.remove_rot = false;
    }
  }
}



/**
 * set the number of atoms.
 */
void configuration::Configuration::resize(unsigned int s)
{
  DEBUG(7, "Configuration resize: " << s);
  
  current().resize(s);
  old().resize(s);
  
  special().lattice_shifts.resize(s);
}

/**
 * set the number of atoms.
 * using resizeAndPreserve. Therefore
 * you can enlarge the system (or shrink it)
 * while keeping all existing positions/velocities/...
 * a faster version would be just resize, but then
 * the arrays contain garbage...
 * the energies have to be sized separately!
 */
void configuration::Configuration::state_struct::resize(unsigned int s)
{
  DEBUG(7, "state struct resize: " << s);

  pos.resize(s);
  posV.resize(s);
  vel.resize(s);
  force.resize(s);
  constraint_force.resize(s);
  stochastic_integral.resize(s);
}

void configuration::Configuration::lattice_sum_struct::init(topology::Topology const & topo,
        simulation::Simulation & sim) {
  DEBUG(1,"Lattice Sum initialization.");
  simulation::Parameter & param = sim.param();
#ifdef OMP
  int tid, size;
#pragma omp parallel private(tid)
  {
    tid = omp_get_thread_num();
    if (tid == 0) {
      size = omp_get_num_threads();
    }
  }
  FFTW3(init_threads());
  FFTW3(plan_with_nthreads(size));
  sim.openmp = true;
#endif
  // get the k space
  if (param.nonbonded.method == simulation::el_ewald) {
    kspace.reserve(param.nonbonded.ewald_max_k_x *
            param.nonbonded.ewald_max_k_y *
            param.nonbonded.ewald_max_k_z);
  }
  
  if (param.nonbonded.method == simulation::el_p3m) {
    const unsigned int Nx = param.nonbonded.p3m_grid_points_x;
    const unsigned int Ny = param.nonbonded.p3m_grid_points_y;
    const unsigned int Nz = param.nonbonded.p3m_grid_points_z;

    const bool do_a2t =
          param.nonbonded.ls_calculate_a2 == simulation::ls_a2t_exact ||
          param.nonbonded.ls_calculate_a2 == simulation::ls_a2t_exact_a2_numerical ||
          param.nonbonded.ls_calculate_a2 == simulation::ls_a2t_ave_a2_numerical;

#ifdef XXMPI
    if (sim.mpi) {
      int rank = MPI::COMM_WORLD.Get_rank();
      int num_threads = MPI::COMM_WORLD.Get_size();
      const int cache_size = std::max(param.nonbonded.p3m_charge_assignment - 1,
              param.nonbonded.p3m_finite_differences_operator);
      
      charge_density = new configuration::ParallelMesh(num_threads, rank, cache_size);
      
      potential = new configuration::ParallelMesh(num_threads, rank, cache_size);
      electric_field.x = new configuration::ParallelMesh(num_threads, rank, cache_size);
      electric_field.y = new configuration::ParallelMesh(num_threads, rank, cache_size);
      electric_field.z = new configuration::ParallelMesh(num_threads, rank, cache_size);

      if (do_a2t)
        squared_charge = new configuration::ParallelMesh(num_threads, rank, cache_size);
      
      ((configuration::ParallelMesh*)charge_density)->resize(Nx, Ny, Nz);
      ((configuration::ParallelMesh*)potential)->resize(Nx, Ny, Nz);
      ((configuration::ParallelMesh*)electric_field.x)->resize(Nx, Ny, Nz);
      ((configuration::ParallelMesh*)electric_field.y)->resize(Nx, Ny, Nz);
      ((configuration::ParallelMesh*)electric_field.z)->resize(Nx, Ny, Nz);
      if (do_a2t)
        ((configuration::ParallelMesh*)squared_charge)->resize(Nx, Ny, Nz);
    } else {
#endif
      charge_density = new configuration::Mesh();
      potential = new configuration::Mesh();
      electric_field.x = new configuration::Mesh();
      electric_field.y = new configuration::Mesh();
      electric_field.z = new configuration::Mesh();
      if (do_a2t)
        squared_charge = new configuration::Mesh();
      charge_density->resize(Nx, Ny, Nz);
      potential->resize(Nx, Ny, Nz);
      electric_field.x->resize(Nx, Ny, Nz);
      electric_field.y->resize(Nx, Ny, Nz);
      electric_field.z->resize(Nx, Ny, Nz);
      if (do_a2t)
        squared_charge->resize(Nx, Ny, Nz);
#ifdef XXMPI
    }
#endif
    
    influence_function.init(param);
  }

  // reset the A term
  a2_tilde = 0.0;


}
namespace configuration
{
  std::ostream &operator<<(std::ostream &os, Configuration &conf)
  {
    os << "a configuration";
    return os;
  }
}


bool configuration::Configuration::check(topology::Topology const & topo, simulation::Simulation & sim) const {
  int error = 0;
  
  // check the positions if nonbonded forces are computed
  if (sim.param().force.nonbonded_crf || sim.param().force.nonbonded_vdw) {
    SPLIT_MY_BOUNDARY(boundary_type, check_positions, error);
  }
  
  return error == 0;
}

template<math::boundary_enum B> 
void configuration::Configuration::check_positions(int & error) const {  
  math::Periodicity<B> periodicity(current().box);
  const unsigned int num_pos = current().pos.size();
  const math::VArray & pos = current().pos;
  math::Vec r;
  for(unsigned int i = 0; i < num_pos; ++i) {
    for(unsigned int j = i + 1; j < num_pos; ++j) {
      periodicity.nearest_image(pos(i), pos(j), r);
      if (math::abs2(r) < math::epsilon) {
        ++error;
        std::ostringstream msg;
        msg << "Singularity: Atoms " << i+1 << " and " << j+1 << " at same position.";
        io::messages.add(msg.str(), "Configuration", io::message::error);
      }
    }
  }
}
