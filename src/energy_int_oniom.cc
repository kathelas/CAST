#include <cstddef>

#include "energy_int_oniom.h"
#include "Scon/scon_utility.h"
#include "coords_io.h"

::tinker::parameter::parameters energy::interfaces::oniom::ONIOM::tp;

energy::interfaces::oniom::ONIOM::ONIOM(coords::Coordinates* cp) :
	energy::interface_base(cp), qm_indices(Config::get().energy.qmmm.qm_systems),
	new_indices_qm(qmmm_helpers::make_several_new_indices(qm_indices, cp->size())),
	link_atoms(qmmm_helpers::create_several_linkatomsets(qm_indices, cp, tp, Config::get().energy.qmmm.linkatom_sets)),
	qmc(qmmm_helpers::make_several_small_coords(cp, qm_indices, new_indices_qm, Config::get().energy.qmmm.qminterface, Config::get().energy.qmmm.qm_to_file, link_atoms)),
	mmc_small(qmmm_helpers::make_several_small_coords(cp, qm_indices, new_indices_qm, Config::get().energy.qmmm.mminterface, Config::get().energy.qmmm.qm_to_file, link_atoms)),
	mmc_big(qmmm_helpers::make_small_coords(cp, range(cp->size()), range(cp->size()), Config::get().energy.qmmm.mminterface, "Big system: ")),
	QMcenter_indices(qmmm_helpers::get_indices_of_several_QMcenters(Config::get().energy.qmmm.centers, qm_indices, coords)),
	qm_energy(0.0), mm_energy_small(0.0), mm_energy_big(0.0), number_of_qm_systems(qm_indices.size())
{
	for (auto i{ 0u }; i < number_of_qm_systems; ++i)
	{
		mmc_small[i].energyinterface()->charge = qmc[i].energyinterface()->charge;   // set charge of small MM system(s) to the correct value, should always be the same
	}

	if ((Config::get().energy.qmmm.qminterface != config::interface_types::T::DFTB && Config::get().energy.qmmm.qminterface != config::interface_types::T::GAUSSIAN
		&& Config::get().energy.qmmm.qminterface != config::interface_types::T::PSI4 && Config::get().energy.qmmm.qminterface != config::interface_types::T::MOPAC
		&& Config::get().energy.qmmm.qminterface != config::interface_types::T::ORCA)
		||
		(Config::get().energy.qmmm.mminterface != config::interface_types::T::OPLSAA && Config::get().energy.qmmm.mminterface != config::interface_types::T::AMBER &&
			Config::get().energy.qmmm.mminterface != config::interface_types::T::DFTB && Config::get().energy.qmmm.mminterface != config::interface_types::T::GAUSSIAN
			&& Config::get().energy.qmmm.mminterface != config::interface_types::T::PSI4 && Config::get().energy.qmmm.mminterface != config::interface_types::T::MOPAC
			&& Config::get().energy.qmmm.mminterface != config::interface_types::T::ORCA))
	{
		throw std::runtime_error("One of your chosen interfaces is not suitable for ONIOM.");
	}

	if (!file_exists(Config::get().get().general.paramFilename) &&    // if forcefield is desired but no parameterfile is given -> throw error
		(Config::get().energy.qmmm.qminterface == config::interface_types::T::OPLSAA || Config::get().energy.qmmm.qminterface == config::interface_types::T::AMBER
			|| Config::get().energy.qmmm.mminterface == config::interface_types::T::OPLSAA || Config::get().energy.qmmm.qminterface == config::interface_types::T::AMBER))
	{
		throw std::runtime_error("You need a tinker-like parameterfile for your chosen forcefield.");
	}

	if (!tp.valid() && file_exists(Config::get().get().general.paramFilename))
	{
		tp.from_file(Config::get().get().general.paramFilename);
	}

	if (Config::get().periodics.periodic)
	{
		if (Config::get().energy.qmmm.mminterface == config::interface_types::T::OPLSAA || Config::get().energy.qmmm.mminterface == config::interface_types::T::AMBER)
		{
			double const min_cut = std::min({ Config::get().periodics.pb_box.x(), Config::get().periodics.pb_box.y(), Config::get().periodics.pb_box.z() }) / 2.0;
			if (Config::get().energy.cutoff > min_cut)
			{
				std::cout << "\n!!! WARNING! Forcefield cutoff too big! Your cutoff should be smaller than " << min_cut << "! !!!\n\n";
			}
		}

		double const min_cut = std::min({ Config::get().periodics.pb_box.x(), Config::get().periodics.pb_box.y(), Config::get().periodics.pb_box.z() }) / 2.0;
		if (Config::get().energy.qmmm.cutoff > min_cut)
		{
			std::cout << "\n!!! WARNING! QM/MM cutoff too big! Your cutoff should be smaller than " << min_cut << "! !!!\n\n";
		}
	}
}

energy::interfaces::oniom::ONIOM::ONIOM(ONIOM const& rhs,
	coords::Coordinates* cobj) : interface_base(cobj),
	qm_indices(rhs.qm_indices),
	new_indices_qm(rhs.new_indices_qm), link_atoms(rhs.link_atoms),
	qmc(rhs.qmc), mmc_small(rhs.mmc_small), mmc_big(rhs.mmc_big),
	qm_energy(rhs.qm_energy), mm_energy_small(rhs.mm_energy_small), mm_energy_big(rhs.mm_energy_big)
{
	interface_base::operator=(rhs);
}

energy::interfaces::oniom::ONIOM::ONIOM(ONIOM&& rhs, coords::Coordinates* cobj)
	: interface_base(cobj),
	qm_indices(std::move(rhs.qm_indices)),
	new_indices_qm(std::move(rhs.new_indices_qm)), link_atoms(std::move(rhs.link_atoms)),
	qmc(std::move(rhs.qmc)), mmc_small(std::move(rhs.mmc_small)), mmc_big(std::move(rhs.mmc_big)),
	qm_energy(std::move(rhs.qm_energy)), mm_energy_small(std::move(rhs.mm_energy_small)), mm_energy_big(std::move(rhs.mm_energy_big))
{
	interface_base::operator=(rhs);
}


energy::interface_base* energy::interfaces::oniom::ONIOM::clone(coords::Coordinates* c) const
{
	ONIOM* tmp = new ONIOM(*this, c);
	return tmp;
}

energy::interface_base* energy::interfaces::oniom::ONIOM::move(coords::Coordinates* c)
{
	ONIOM* tmp = new ONIOM(std::move(*this), c);
	return tmp;
}


void energy::interfaces::oniom::ONIOM::swap(interface_base& rhs)
{
	swap(dynamic_cast<ONIOM&>(rhs));
}

void energy::interfaces::oniom::ONIOM::swap(ONIOM& rhs)
{
	interface_base::swap(rhs);
	qm_indices.swap(rhs.qm_indices);
	link_atoms.swap(rhs.link_atoms);
	new_indices_qm.swap(rhs.new_indices_qm);
	qmc.swap(rhs.qmc);
	mmc_big.swap(rhs.mmc_big);
	mmc_small.swap(rhs.mmc_small);
	std::swap(qm_energy, rhs.qm_energy);
	std::swap(mm_energy_big, rhs.mm_energy_big);
	std::swap(mm_energy_small, rhs.mm_energy_small);
}

// update structure (account for topology or rep change)
void energy::interfaces::oniom::ONIOM::update(bool const skip_topology)
{
	if (!skip_topology)
	{
		*this = ONIOM(this->coords);
	}
	else
	{
		update_representation();
		for (auto i{ 0u }; i < number_of_qm_systems; ++i)
		{
			qmc[i].energy_update(true);
			mmc_small[i].energy_update(true);
		}
		mmc_big.energy_update(true);
	}
}

void energy::interfaces::oniom::ONIOM::update_representation()
{
	for (auto j{ 0u }; j < number_of_qm_systems; ++j)    // for each QM system
	{
		std::size_t qi = 0u;    // update position of QM atoms in small systems
		for (auto i : qm_indices[j])
		{
			qmc[j].move_atom_to(qi, coords->xyz(i), true);
			mmc_small[j].move_atom_to(qi, coords->xyz(i), true);
			++qi;
		}

		for (auto& l : link_atoms[j]) l.calc_position(coords); // update positions of link atoms in small systems
		for (auto i = 0u; i < link_atoms[j].size(); ++i)
		{
			int index = qm_indices[j].size() + i;
			coords::cartesian_type& new_pos = link_atoms[j][i].position;
			qmc[j].move_atom_to(index, new_pos, true);
			mmc_small[j].move_atom_to(index, new_pos, true);
		}
	}

	for (std::size_t mi = 0u; mi < coords->size(); mi++)   // update positions of all atoms in big system
	{
		mmc_big.move_atom_to(mi, coords->xyz()[mi], true);
	}

}

coords::float_type energy::interfaces::oniom::ONIOM::qmmm_calc(bool if_gradient)
{
	// test if there is no atom in more than one QM system
	std::vector<std::size_t> all_qm_atoms;
	for (auto const& qmi : qm_indices) all_qm_atoms = add_vectors(all_qm_atoms, qmi);
	if (double_element(all_qm_atoms) == true) throw std::runtime_error("ERROR! There is at least one atom in more than one QM system!");

	// test if correct number of link atom types is given
	for (auto j{ 0u }; j < number_of_qm_systems; ++j) // for each QM system
	{
		if (link_atoms[j].size() != Config::get().energy.qmmm.linkatom_sets[j].size())  // test if correct number of link atom types is given
		{                                                                          // can't be done in constructor because interface is first constructed without atoms 
			std::cout << "Wrong number of link atom types given for QM system " << j + 1 << ".";
			std::cout << " You have " << link_atoms[j].size() << " in the following order: \n";
			for (auto& l : link_atoms[j])
			{
				std::cout << "QM atom: " << l.qm + 1 << ", MM atom: " << l.mm + 1 << "\n";
			}
			throw std::runtime_error("wrong number of link atom types");
		}
	}

	update_representation(); // update positions of QM and MM subsystems to those of coordinates object 

	mm_energy_big = 0.0;     // set energies to zero
	mm_energy_small = 0.0;
	qm_energy = 0.0;
	coords::Gradients_3D new_grads;  // save gradients in case of gradient calculation
	bool periodic = Config::get().periodics.periodic;

	// ############### MM ENERGY AND GRADIENTS FOR WHOLE SYSTEM ######################

	try {
		if (!if_gradient)
		{
			mm_energy_big = mmc_big.e();   // calculate MM energy of whole system
		}
		else   // gradient calculation
		{
			mm_energy_big = mmc_big.g();
			new_grads = mmc_big.g_xyz();
		}
		if (mm_energy_big == 0) integrity = false;
		if (Config::get().general.verbosity > 4)
		{
			std::cout << "Energy of big MM system: \n";
			mmc_big.e_head_tostream_short(std::cout);
			mmc_big.e_tostream_short(std::cout);
		}
	}
	catch (...)
	{
		std::cout << "MM programme (for big system) failed. Treating structure as broken.\n";
		integrity = false;  // if MM programme fails: integrity is destroyed
	}

	// if program didn't calculate an energy: return zero-energy (otherwise CAST will break because it doesn't find charges)
	if (integrity == false) return 0.0;

	for (auto j{ 0u }; j < number_of_qm_systems; ++j)
	{
		double current_energy{ 0.0 };

		if (Config::get().energy.qmmm.qminterface == config::interface_types::T::MOPAC) Config::set().energy.mopac.link_atoms = link_atoms[j].size();   // set number of link atoms for MOPAC


		Config::set().periodics.periodic = false;        // deactivate periodic boundaries

		// ############### CREATE MM CHARGES ######################

		std::vector<int> charge_indices;                  // indizes of all atoms that are in charge_vector
		if (Config::get().energy.qmmm.zerocharge_bonds != 0)
		{
			std::vector<double> charge_vector = mmc_big.energyinterface()->charges();
			if (charge_vector.size() == 0) throw std::runtime_error("no charges found in MM interface");
			auto all_indices = range(coords->size());

			charge_indices.clear();
			qmmm_helpers::add_external_charges(qm_indices[j], charge_vector, all_indices, link_atoms[j], charge_indices, coords, QMcenter_indices[j]);
		}

		Config::set().periodics.periodic = false;        // deactivate periodic boundaries

		// ############### QM ENERGY AND GRADIENTS FOR QM SYSTEM ######################
		try {
			if (!if_gradient)
			{
				current_energy = qmc[j].e();  // get energy for QM part 
			}
			else  // gradient calculation
			{
				current_energy = qmc[j].g();            // get energy and calculate gradients
				auto g_qm_small = qmc[j].g_xyz();  // get gradients
				for (auto&& qmi : qm_indices[j])
				{
					new_grads[qmi] += g_qm_small[new_indices_qm[j][qmi]];
				}

				for (auto i = 0u; i < link_atoms[j].size(); ++i)   // take into account link atoms
				{
					LinkAtom l = link_atoms[j][i];

					coords::r3 g_qm, g_mm;        // divide link atom gradient to QM and MM atom
					auto link_atom_grad = g_qm_small[qm_indices[j].size() + i];
					qmmm_helpers::calc_link_atom_grad(l, link_atom_grad, coords, g_qm, g_mm);
					new_grads[l.qm] += g_qm;
					new_grads[l.mm] += g_mm;

					if (Config::get().general.verbosity > 4)
					{
						std::cout << "Link atom between " << l.qm + 1 << " and " << l.mm + 1 << " has a gradient " << link_atom_grad << ".\n";
						std::cout << "It causes a gradient on QM atom " << g_qm << " and on MM atom " << g_mm << ".\n";
					}
				}
			}
			if (current_energy == 0) integrity = false;
			else qm_energy += current_energy;

			if (Config::get().general.verbosity > 4)
			{
				std::cout << "Energy of QM system " << j + 1 << ": \n";
				qmc[j].e_head_tostream_short(std::cout);
				qmc[j].e_tostream_short(std::cout);
			}
		}
		catch (...)
		{
			std::cout << "QM programme failed. Treating structure as broken.\n";
			integrity = false;  // if QM programme fails: integrity is destroyed
		}

		// ############### ONLY AMBER: PREPARATION OF CHARGES FOR SMALL SYSTEM ################

		// temporarily: only QM charges and those of link atoms in amber_charges
		std::vector<double> old_amber_charges;
		if (Config::get().general.input == config::input_types::AMBER || Config::get().general.chargefile)
		{
			old_amber_charges = Config::get().coords.amber_charges;                       // save old amber_charges
			qmmm_helpers::select_from_ambercharges(qm_indices[j]);                           // only QM charges in amber_charges
			for (auto i = 0u; i < link_atoms[j].size(); ++i)                                   // add charges of link atoms
			{
				double la_charge = qmc[j].energyinterface()->charges()[qm_indices[j].size() + i]; // get charge
				Config::set().coords.amber_charges.push_back(la_charge * 18.2223);            // convert it to AMBER units and add it to vector
			}
		}

		// ################ SAVE OUTPUT FOR BIG MM SYSTEM ########################################################

		if (j == 0) qmmm_helpers::save_outputfiles(Config::get().energy.qmmm.mminterface, mmc_big.energyinterface()->id, "big");

		// ############### MM ENERGY AND GRADIENTS FOR SMALL MM SYSTEM ######################

		try {
			if (!if_gradient)
			{
				current_energy = mmc_small[j].e();  // calculate energy of small MM system
			}
			else  // gradient calculation
			{
				current_energy = mmc_small[j].g();     // get energy and calculate gradients
				auto g_mm_small = mmc_small[j].g_xyz(); // get gradients
				for (auto&& qmi : qm_indices[j])
				{
					new_grads[qmi] -= g_mm_small[new_indices_qm[j][qmi]];
				}

				for (auto i = 0u; i < link_atoms[j].size(); ++i)  // take into account link atoms
				{
					LinkAtom l = link_atoms[j][i];

					coords::r3 g_qm, g_mm;             // divide link atom gradient to QM and MM atom
					auto link_atom_grad = g_mm_small[qm_indices[j].size() + i];
					qmmm_helpers::calc_link_atom_grad(l, link_atom_grad, coords, g_qm, g_mm);
					new_grads[l.qm] -= g_qm;
					new_grads[l.mm] -= g_mm;
					if (Config::get().general.verbosity > 4)
					{
						std::cout << "Link atom between " << l.qm + 1 << " and " << l.mm + 1 << " has a gradient " << link_atom_grad << ".\n";
						std::cout << "It causes a gradient on QM atom " << g_qm << " and on MM atom " << g_mm << ".\n";
					}
				}
			}
			if (current_energy == 0) integrity = false;
			else mm_energy_small += current_energy;

			if (Config::get().general.verbosity > 4)
			{
				std::cout << "Energy of small MM system " << j + 1 << ": \n";
				mmc_small[j].e_head_tostream_short(std::cout);
				mmc_small[j].e_tostream_short(std::cout);
			}
		}
		catch (...)
		{
			std::cout << "MM programme (for small system) failed. Treating structure as broken.\n";
			integrity = false;  // if MM programme fails: integrity is destroyed
		}

		// ############### GRADIENTS ON MM ATOMS DUE TO COULOMB INTERACTION WITH QM REGION ###

		if (if_gradient && integrity == true && Config::get().energy.qmmm.zerocharge_bonds != 0)
		{
			auto qmc_g_ext_charges = qmc[j].energyinterface()->get_g_ext_chg();
			auto mmc_small_g_ext_charges = mmc_small[j].energyinterface()->get_g_ext_chg();

			for (auto i = 0u; i < charge_indices.size(); ++i)   // for all external charges
			{
				int mma = charge_indices[i];
				auto grad_qmc = qmc_g_ext_charges[i];
				auto grad_mmc_small = mmc_small_g_ext_charges[i];

				coords::r3 derivQ_qmc{ 0.0, 0.0, 0.0 }, derivQ_mmc{ 0.0, 0.0, 0.0 };   // additional gradients because charge also changes with position
				if (Config::get().energy.qmmm.cutoff != std::numeric_limits<double>::max())
				{
					double constexpr elec_factor = 332.06;
					double const& c = Config::get().energy.qmmm.cutoff;
					double const& ext_chg = Config::get().energy.qmmm.mm_charges[i].original_charge;
					double const& scaling = Config::get().energy.qmmm.mm_charges[i].scaled_charge / Config::get().energy.qmmm.mm_charges[i].original_charge;
					auto chargesQM = qmc[j].energyinterface()->charges();
					auto chargesMM = mmc_small[j].energyinterface()->charges();

					// calculate sum(Q_qm * Q_ext / r) for systems QMC and MMC_SMALL
					double sum_of_QM_interactions_qmc{ 0.0 };
					double sum_of_QM_interactions_mmc{ 0.0 };
					for (auto k{ 0u }; k < qm_indices.size(); ++k)
					{
						double const QMcharge_qmc = chargesQM[k];
						double const QMcharge_mmc = chargesMM[k];
						coords::r3 MMpos{ Config::get().energy.qmmm.mm_charges[i].x,  Config::get().energy.qmmm.mm_charges[i].y,  Config::get().energy.qmmm.mm_charges[i].z };
						double const dist = len(MMpos - coords->xyz(qm_indices[j][k]));
						sum_of_QM_interactions_qmc += (QMcharge_qmc * ext_chg * elec_factor) / dist;
						sum_of_QM_interactions_mmc += (QMcharge_mmc * ext_chg * elec_factor) / dist;
					}

					// additional gradient on external charge due to interaction with QMC
					derivQ_qmc.x() = sum_of_QM_interactions_qmc * 4 * (coords->xyz(QMcenter_indices[j]).x() - Config::get().energy.qmmm.mm_charges[i].x) * std::sqrt(scaling) / (c * c);
					derivQ_qmc.y() = sum_of_QM_interactions_qmc * 4 * (coords->xyz(QMcenter_indices[j]).y() - Config::get().energy.qmmm.mm_charges[i].y) * std::sqrt(scaling) / (c * c);
					derivQ_qmc.z() = sum_of_QM_interactions_qmc * 4 * (coords->xyz(QMcenter_indices[j]).z() - Config::get().energy.qmmm.mm_charges[i].z) * std::sqrt(scaling) / (c * c);
					grad_qmc += derivQ_qmc;

					// additional gradient on external charge due to interaction with MMC_SMALL
					derivQ_mmc.x() = sum_of_QM_interactions_mmc * 4 * (coords->xyz(QMcenter_indices[j]).x() - Config::get().energy.qmmm.mm_charges[i].x) * std::sqrt(scaling) / (c * c);
					derivQ_mmc.y() = sum_of_QM_interactions_mmc * 4 * (coords->xyz(QMcenter_indices[j]).y() - Config::get().energy.qmmm.mm_charges[i].y) * std::sqrt(scaling) / (c * c);
					derivQ_mmc.z() = sum_of_QM_interactions_mmc * 4 * (coords->xyz(QMcenter_indices[j]).z() - Config::get().energy.qmmm.mm_charges[i].z) * std::sqrt(scaling) / (c * c);
					grad_mmc_small += derivQ_mmc;

					// additional gradient on QM atom that defines distance
					new_grads[QMcenter_indices[j]] += (derivQ_mmc - derivQ_qmc);
				}

				new_grads[mma] += grad_qmc;
				new_grads[mma] -= grad_mmc_small;
			}
		}

		// ############### STUFF TO DO AT THE END OF CALCULATION ######################

		Config::set().energy.qmmm.mm_charges.clear();            // clear vector -> no point charges in calculation of mmc_big
		Config::set().periodics.periodic = periodic;             // set back periodics
		Config::set().coords.amber_charges = old_amber_charges;  // set AMBER charges back to total AMBER charges
		if (file_exists("orca.gbw")) std::remove("orca.gbw");    // delete orca MOs for small system, otherwise orca will try to use them for big system and fail

		qmmm_helpers::save_outputfiles(Config::get().energy.qmmm.mminterface, mmc_small[j].energyinterface()->id, std::to_string(j + 1));
		qmmm_helpers::save_outputfiles(Config::get().energy.qmmm.qminterface, qmc[j].energyinterface()->id, std::to_string(j + 1));
	}  // end of the loop over all QM systems

	if (coords->check_bond_preservation() == false) integrity = false;
	else if (coords->check_for_crashes() == false) integrity = false;

	if (if_gradient) coords->swap_g_xyz(new_grads);     // swap gradients into coordobj
	energy = mm_energy_big - mm_energy_small + qm_energy;
	return energy; // return total energy
}

coords::float_type energy::interfaces::oniom::ONIOM::g()
{
	integrity = coords->check_structure();
	if (integrity == true) return qmmm_calc(true);
	else return 0;
}

coords::float_type energy::interfaces::oniom::ONIOM::e()
{
	integrity = coords->check_structure();
	if (integrity == true) return qmmm_calc(false);
	else return 0;
}

coords::float_type energy::interfaces::oniom::ONIOM::h()
{
	throw std::runtime_error("no hessian function implemented for this interface");
}

coords::float_type energy::interfaces::oniom::ONIOM::o()
{
	throw std::runtime_error("this interface doesn't have an own optimizer");
}

void energy::interfaces::oniom::ONIOM::print_E(std::ostream&) const
{
	throw std::runtime_error("function not implemented");
}

void energy::interfaces::oniom::ONIOM::print_E_head(std::ostream& S, bool const endline) const
{
	auto total_number_of_QMatoms{ 0 };
	for (auto const& qm : qm_indices) total_number_of_QMatoms += qm.size();
	S << "QM-atoms: " << total_number_of_QMatoms << '\n';
	S << "MM-atoms: " << coords->size() - total_number_of_QMatoms << '\n';
	S << "Potentials\n";
	S << std::right << std::setw(24) << "MM_big";
	S << std::right << std::setw(24) << "MM_small";
	S << std::right << std::setw(24) << "QM";
	S << std::right << std::setw(24) << "TOTAL";
	if (endline) S << '\n';
}

void energy::interfaces::oniom::ONIOM::print_E_short(std::ostream& S, bool const endline) const
{
	S << '\n';
	S << std::fixed << std::setprecision(1) << std::right << std::setw(24) << mm_energy_big;
	S << std::fixed << std::setprecision(1) << std::right << std::setw(24) << mm_energy_small;
	S << std::fixed << std::setprecision(1) << std::right << std::setw(24) << qm_energy;
	S << std::fixed << std::setprecision(1) << std::right << std::setw(24) << energy;
	if (endline) S << '\n';
}


