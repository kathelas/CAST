
/**
CAST 3
energy_int_orca.h
Purpose: interface to ORCA

@author Susanne Sauer
@version 1.0
*/

#pragma once

#include <vector>
#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <cstdlib>
#include <utility>
#include "helperfunctions.h"
#include "atomic.h"
#include "energy.h"
#include "configuration.h"
#include "coords.h"
#include "coords_io.h"
#include "modify_sk.h"

#if defined (_MSC_VER)
#include "win_inc.h"
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

namespace energy
{
	namespace interfaces
	{
		namespace orca
		{


			class sysCallInterface
				: public energy::interface_base
			{

			public:
				/**constructor: sets optimizer*/
				sysCallInterface(coords::Coordinates*);
				/**delete interface?*/
				~sysCallInterface(void);

				/*
				Energy class functions that need to be overloaded (for documentation see also energy.h)
				*/

				interface_base* clone(coords::Coordinates* coord_object) const;
				interface_base* move(coords::Coordinates* coord_object);

				void swap(interface_base&);
				void swap(sysCallInterface&);

				/** Energy function*/
				double e(void);
				/** Energy+Gradient function*/
				double g(void);
				/** Energy+Hessian function*/
				double h(void);
				/** Optimization in the interface(d program)*/
				double o(void);
				/**prints total energy*/
				void print_E(std::ostream&) const;
				/**prints 'headline' for energies*/
				void print_E_head(std::ostream&, bool const endline = true) const;
				/**prints partial energies (not much sense in it because no partial energies are read)*/
				void print_E_short(std::ostream&, bool const endline = true) const;
				/**does nothing*/
				void to_stream(std::ostream&) const;
				/** "update" function*/
				void update(bool const) { }
				/**returns partial atomic charges*/
				std::vector<coords::float_type> charges() const override;
				/**returns gradients on external charges due to the molecular system (used for QM/MM)*/
				std::vector<coords::Cartesian_Point> get_g_ext_chg() const override;

			private:

				/**constructor for clone and move functions*/
				sysCallInterface(sysCallInterface const& rhs, coords::Coordinates* cobj);

				/**writes orca inputfile
				@param t: type of calculation (0 = energy, 1 = gradient, 2 = hessian, 3 = optimize)*/
				void write_inputfile(int t);

				/**reads orca outputfile (???)
				@param t: type of calculation (0 = energy, 1 = gradient, 2 = hessian, 3 = optimize)*/
				double read_output(int t);

				/**function to read hessian from file
				@filename: name of file (normally ending in .hess)*/
				void read_hessian_from_file(std::string const& filename);

				/**total energy*/
				double energy;

				// partial energies (scf energy = nuc_rep + elec_en, elec_en = one_elec + two_elec)
				// for total energy there might be even more contributions

				/**nuclear repulsion energy*/
				double nuc_rep;
				/**electronic energy*/
				double elec_en;
				/**one-electron contribution*/
				double one_elec;
				/**two-electron contribution*/
				double two_elec;

				// STUFF FOR QM/MM 

				/**mulliken charges of every atom*/
				std::vector<double> mulliken_charges;

				/**function to write external point charges into file
				@param filename: name of the file*/
				void write_external_pointcharges(std::string const& filename);

				/**gradients of external charges*/
				std::vector<coords::Cartesian_Point> grad_ext_charges;
			};

		}
	}
}
