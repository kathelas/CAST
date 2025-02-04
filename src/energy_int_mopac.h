/*DW                            *
MOPAC Interface
*                            DW*/

#pragma once

#include <vector>
#include <string>

#include "energy.h"


namespace energy
{
	namespace interfaces
	{
		namespace mopac
		{

			class sysCallInterface
				: public energy::interface_base
			{

			public:

				sysCallInterface(coords::Coordinates*);
				~sysCallInterface(void);

				/*
				Energy class functions that need to be overloaded
				*/

				interface_base* clone(coords::Coordinates* coord_object) const;
				interface_base* move(coords::Coordinates* coord_object);

				void swap(interface_base&);
				void swap(sysCallInterface&);

				// Energy function
				double e(void);
				// Energy+Gradient function
				double g(void);
				// Energy+Gradient+Hessian function
				double h(void);
				// Optimization in the interface(d program)
				double o(void);

				/**get atom charges*/
				std::vector<coords::float_type> charges() const override {
					return atomic_charges;
				};
				/**function to get gradients on external charges*/
				std::vector<coords::Cartesian_Point> get_g_ext_chg() const override {
					return grad_ext_charges;
				}

				//MOPAC7_HB VAR
				bool grad_var;
				// Output functions
				void print_E(std::ostream&) const;
				void print_E_head(std::ostream&, bool const endline = true) const;
				void print_E_short(std::ostream&, bool const endline = true) const;
				void to_stream(std::ostream&) const;
				// "update" function
				void update(bool const) { }

			private:

				// constructor for clone and move functions
				sysCallInterface(sysCallInterface const& rhs, coords::Coordinates* cobj);

				// heat of formation
				double hof_kcal_mol, hof_kj_mol;
				// energies
				double e_total, e_electron, e_core;

				/**vector of atomic charges*/
				std::vector<coords::float_type> atomic_charges;

				/**gradients of external charges*/
				std::vector<coords::Cartesian_Point> grad_ext_charges;

				// FAILCOUNTER
				size_t failcounter;

				/*
				Mopac sysCall functions
				*/

				int callMopac(void);
				void print_mopacInput(bool const grad = true, bool const hess = false, bool const opt = true);
				void read_mopacOutput(bool const grad = true, bool const hess = false, bool const opt = true);

				/**writes mol.in file in case of QM/MM calculation*/
				void write_mol_in();
				/**reads atomic charges from mopac outputfile*/
				void read_charges();
			};

		}
	}
}
