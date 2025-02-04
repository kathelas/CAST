#ifndef COORDS_IO_H
#define COORDS_IO_H

#if defined _OPENMP
#include <omp.h>
#endif
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include "Scon/scon_utility.h"
#include "coords.h"
#include "configuration.h"
#include "helperfunctions.h"


namespace coords
{
	/**terminal states: not terminal, C-terminal (protonated or not), N-terminal*/
	enum class terminalState { no, C, C_prot, N };
	/**overloaded output operator for terminalState*/
	inline std::ostream& operator<< (std::ostream& os, const terminalState& T)
	{
		switch (T)
		{
		case terminalState::no: os << "not terminal"; break;
		case terminalState::C:  os << "C-terminal"; break;
		case terminalState::C_prot:  os << "C-terminal"; break;
		case terminalState::N:  os << "N-terminal"; break;
		}
		return os;
	}

	/**known aminoacids: 3-letter codes + some special names inspired by AMBER (http://ambermd.org/tutorials/advanced/tutorial1_orig/section1.htm)
	CYX: cysteine in disulfide bridge
	CYM: deprotonated cysteine
	HID: histidine protonated at N_delta
	HIE: histidine protonated at N_epsilon
	HIP: histidine where both nitrogen atoms are protonated
	XXX: just a wildcard for not known aminoacid*/
	enum class residueName { ALA, ARG, ASN, ASP, CYS, GLN, GLU, GLY, HIS, ILE, LEU, LYS, MET, PHE, PRO, SER, THR, TRP, TYR, VAL, CYX, CYM, HID, HIE, HIP, XXX };
	/**function to convert residueName to string*/
	inline std::string res_to_string(const residueName& res)
	{
		switch (res)
		{
		case residueName::ALA: return "ALA";
		case residueName::ARG: return "ARG";
		case residueName::ASN: return "ASN";
		case residueName::ASP: return "ASP";
		case residueName::CYS: return "CYS";
		case residueName::GLN: return "GLN";
		case residueName::GLU: return "GLU";
		case residueName::GLY: return "GLY";
		case residueName::HIS: return "HIS";
		case residueName::ILE: return "ILE";
		case residueName::LEU: return "LEU";
		case residueName::LYS: return "LYS";
		case residueName::MET: return "MET";
		case residueName::PHE: return "PHE";
		case residueName::PRO: return "PRO";
		case residueName::SER: return "SER";
		case residueName::THR: return "THR";
		case residueName::TRP: return "TRP";
		case residueName::TYR: return "TYR";
		case residueName::VAL: return "VAL";
		case residueName::CYX: return "CYX";
		case residueName::CYM: return "CYM";
		case residueName::HID: return "HID";
		case residueName::HIE: return "HIE";
		case residueName::HIP: return "HIP";
		case residueName::XXX: return "XXX";
		default: return "XXX";
		}
	}
	/**overloaded output operator for residueName*/
	inline std::ostream& operator<< (std::ostream& os, const residueName& res)
	{
		os << res_to_string(res);
		return os;
	}

	/**class for one amino acid*/
	class AminoAcid
	{
	public:
		/**constructor
		@param i: indices of backbone atoms (order: carbonyle O, carbonyle C, C alpha, N)
		@param T: terminal state*/
		AminoAcid(std::vector<std::size_t> i, terminalState T) : indices(i), terminal(T) {};

		/**get all indices*/
		std::vector<std::size_t> get_indices() const { return indices; }
		/**add an index to indices
		@param i: index to be added*/
		void add_index(std::size_t const i) { indices.emplace_back(i); }

		/**determine residueName of aminoacid and saving it into res_name
		as this is only done by chemical formula there might be inaccuracies, i.e. for protonation states of HIS or the binding mode of CYS
		those will be corrected later when assigning atomtypes
		@param atoms: atom vector*/
		void determine_aminoacid(Atoms const& atoms);
		/**assigns oplsaa atomtypes to atoms
		@param atoms: atom vector*/
		void assign_atom_types(Atoms& atoms);
		/**function that assigns the correct three-letter codes to those aminoacids where determine_aminoacid() didn't
		i.e. HIP -> HIS, CYM -> CYS, ILE -> LEU (sometimes)
		@param atoms: atom vector*/
		void correct_residue_names(Atoms& atoms);
		/**returns residue name as string*/
		std::string get_res_name() { return res_to_string(res_name); }

	private:
		/**indices of all atoms belonging to amino acid
		first 4 indices are those of carbonyle O, carbonyle C, C alpha, amide N*/
		std::vector<std::size_t> indices;
		/**terminal state of amino acid*/
		terminalState terminal;
		/**residue name*/
		residueName res_name{ residueName::XXX };
		/**chemical formula: number of C, H, N, O, S, other in this order*/
		std::vector<int> chemical_formula;

		/**get chemical formula of aminoacid and save it into chemical_formula
		@param atoms: atom vector*/
		void get_chemical_formula(Atoms const& atoms);
		/**assigns oplsaa atomtypes to backbone atoms (part of assign_atom_types())
		@param atoms: atom vector*/
		void assign_backbone_atom_types(Atoms& atoms);
		/**determines residue name from chemical formula*/
		void get_name_from_chemical_formula();

		/**overloaded output operator for AminoAcid*/
		friend std::ostream& operator<< (std::ostream& os, const AminoAcid& as);
	};

	/**overloaded output operator for AminoAcid*/
	inline std::ostream& operator<< (std::ostream& os, const AminoAcid& as)
	{
		os << as.res_name;
		if (as.terminal != coords::terminalState::no) os << "(" << as.terminal << ")";
		return os;
	}

	/**struct for writing a pdb atom line*/
	struct PDBAtom
	{
		/**default constructor*/
		PDBAtom() :
			record_name("HETATM"), atom_name("X"), residue_name("XXX"),
			residue_number(0), x(0), y(0), z(0), symbol("X") {};

		/**ATOM or HETATM*/
		std::string record_name;
		/**atom name (equal to element symbol except for C_alpha atoms)*/
		std::string atom_name;
		/**residue name*/
		std::string residue_name;
		/**residue number*/
		int residue_number;
		/**cartesian coordinates*/
		double x, y, z;
		/**element symbol*/
		std::string symbol;
	};

	/**struct to get forcefield energy type from amino acids*/
	class AtomtypeFinder
	{

	public:
		/**constructor
		sets size of got_it to number of atoms and sets all of them to false*/
		AtomtypeFinder(Atoms& a) : atoms(a)
		{
			got_it.resize(atoms.size());
			for (auto&& g : got_it) g = false;
		};

		/**function that finds all possible atomtypes*/
		void find_energy_types();

		/**function that creates amino acids with backbone atoms and terminal state*/
		std::vector<AminoAcid> get_aminoacids();

	private:
		/**reference to atoms
		will be changed inside this class (addition of atomtypes)*/
		Atoms& atoms;

		/**vector that tells us if an atom either has already a forcefield type or is in an aminoacid*/
		std::vector<bool> got_it;

		/**function that finds atomtypes of some atoms that are quite easy to determine
		sets their value for got_it to true
		at the moment the atomtypes of Na ions and water molecules are found*/
		void get_some_easy_atomtypes();
		/**function that fills the rest of the atoms into the aminoacids*/
		void complete_atoms_of_aminoacids(std::vector<AminoAcid>& amino_acids);
		/**helperfunction for complete_atoms_of_aminoacids()
		is called recursively on every atom and adds all atoms that are bound to current atom to amino acid
		stops at disulfide bonds*/
		void add_bonds_to_as(int index, AminoAcid& as);
	};

	namespace input
	{
		// format types
		struct types { enum T { TINKER, PDB, AMBER, XYZ }; };

		/**converts string into object of enum input::types::T*/
		inline types::T get_type(std::string const& str)
		{
			if (str.find("PDB") != str.npos) return types::PDB;
			else if (str.find("AMBER") != str.npos) return types::AMBER;
			else if (str.find("XYZ") != str.npos) return types::XYZ;
			else return types::TINKER;
		}

		/**general format class
		input formats like AMBER, TINKER, PDB or XYZ inherit from this*/
		class format
		{
		protected:
			format(void) { }
			coords::Ensemble_PES input_ensemble;
		public:
			virtual ~format(void) { }
			// Number of Atoms (N) and Structures (M)
			std::size_t atoms(void) const { return input_ensemble.size() > 0 ? input_ensemble.back().size() : 0U; }
			std::size_t size(void) const { return input_ensemble.size(); }
			/** Read Structure and return coordinates (this function must be overwritten for newly implemented inputformats)*/
			virtual Coordinates read(std::string) = 0;
			// Get structure i
			PES_Point structure(std::size_t const i = 0u) const { return input_ensemble[i]; }
			Ensemble_PES const& PES(void) const { return input_ensemble; }
			Ensemble_PES& PES(void) { return input_ensemble; }
			// Iterators
			Ensemble_PES::iterator begin(void) { return input_ensemble.begin(); }
			Ensemble_PES::iterator end(void) { return input_ensemble.end(); }
			Ensemble_PES::const_iterator begin(void) const { return input_ensemble.begin(); }
			Ensemble_PES::const_iterator end(void) const { return input_ensemble.end(); }
		};

		// new format creator
		input::format* new_format(void);
		//new format creator for interface creation
		input::format* additional_format(void);
		input::format* new_interf_format(void);

		namespace formats
		{
			// stuff for AMBER input

			/**number of sections in AMBER input file*/
			static const unsigned int sections_size = 91u;
			/**names of sections in AMBER input file*/
			static std::string const amber_sections[sections_size] =
			{ "TITLE", "POINTERS", "ATOM_NAME", "CHARGE", "ATOMIC_NUMBER",
				"MASS", "ATOM_TYPE_INDEX", "NUMBER_EXCLUDED_ATOMS", "NONBONDED_PARM_INDEX", "POLARIZABILITY",
				"RESIDUE_LABEL", "RESIDUE_POINTER", "BOND_FORCE_CONSTANT", "BOND_EQUIL_VALUE", "ANGLE_FORCE_CONSTANT",
				"ANGLE_EQUIL_VALUE", "DIHEDRAL_FORCE_CONSTANT", "DIHEDRAL_PERIODICITY", "DIHEDRAL_PHASE", "SCEE_SCALE_FACTOR",
				"SCNB_SCALE_FACTOR", "SOLTY", "LENNARD_JONES_ACOEF", "LENNARD_JONES_BCOEF", "BONDS_INC_HYDROGEN",
				"BONDS_WITHOUT_HYDROGEN", "ANGLES_INC_HYDROGEN", "ANGLES_WITHOUT_HYDROGEN", "DIHEDRALS_INC_HYDROGEN", "DIHEDRALS_WITHOUT_HYDROGEN",
				"EXCLUDED_ATOMS_LIST", "HBOND_ACOEF", "HBOND_BCOEF", "HBCUT", "AMBER_ATOM_TYPE",
				"TREE_CHAIN_CLASSIFICATION", "JOIN_ARRAY", "IROTAT", /*"SOLVENT_POINTERS",*/ "ATOMS_PER_MOLECULE", // SOLVENT_POINTERS for now disable because it causes trouble with std::find and POINTERS
				"BOX_DIMENSIONS", "CAP_INFO", "CAP_INFO2", "RADIUS_SET", "RADII",
				"IPOL"
			};

			/**class for reading AMBER input*/
			class amber : public coords::input::format
			{
			public:
				/**function that reads in AMBER input*/
				Coordinates read(std::string) override;
			private:
				//struct sections;
				static const unsigned int sections_size = 91u;

				enum section_types
				{
					ILLEGAL = -1,
					TITLE, POINTERS, ATOM_NAME, CHARGE, ATOMIC_NUMBER,
					MASS, ATOM_TYPE_INDEX, NUMBER_EXCLUDED_ATOMS, NONBONDED_PARM_INDEX, POLARIZABILITY,
					RESIDUE_LABEL, RESIDUE_POINTER, BOND_FORCE_CONSTANT, BOND_EQUIL_VALUE, ANGLE_FORCE_CONSTANT,
					ANGLE_EQUIL_VALUE, DIHEDRAL_FORCE_CONSTANT, DIHEDRAL_PERIODICITY, DIHEDRAL_PHASE, SCEE_SCALE_FACTOR,
					SCNB_SCALE_FACTOR, SOLTY, LENNARD_JONES_ACOEF, LENNARD_JONES_BCOEF, BONDS_INC_HYDROGEN,
					BONDS_WITHOUT_HYDROGEN, ANGLES_INC_HYDROGEN, ANGLES_WITHOUT_HYDROGEN, DIHEDRALS_INC_HYDROGEN, DIHEDRALS_WITHOUT_HYDROGEN,
					EXCLUDED_ATOMS_LIST, HBOND_ACOEF, HBOND_BCOEF, HBCUT, AMBER_ATOM_TYPE,
					TREE_CHAIN_CLASSIFICATION, JOIN_ARRAY, IROTAT, SOLVENT_POINTERS, ATOMS_PER_MOLECULE,
					BOX_DIMENSIONS, CAP_INFO, CAP_INFO2, RADIUS_SET, RADII,
					IPOL
				};

				//Important Stuff
				std::vector<unsigned int> sectionsPresent;
				std::vector<unsigned int> bondsWithHydrogen, bondsWithoutHydrogen; // [0] binds to [1], [2] to [3], [4] to [5] and so on....
				Atoms atoms;
				Cartesian_Point position;
				std::string name;
				unsigned int numberOfAtoms;
				int pointers_raw[32];

			};

			/**input format XYZ*/
			class xyz : public coords::input::format
			{
			public:
				/**read from XYZ file*/
				Coordinates read(std::string) override;
			private:
				/**atoms*/
				Atoms atoms;
				/**positions*/
				Cartesian_Point position;

			};

			/**class for reading PDB as input*/
			class pdb : public coords::input::format
			{
			public:
				/**reads input*/
				Coordinates read(std::string);
			private:
				/**atoms (are filled during reading)*/
				Atoms atoms;
				/**positions (are filled during reading)*/
				Cartesian_Point position;
			};


			/*! Class to read from TINKER coordinate file (.arc)
			 *
			 * This class is used to read coordinates from
			 * a tinker xyz file (.arc or sometimes .xyz).
			 * The coordinates are then put into a
			 * coords::Coordinates object.
			 */
			class tinker : public coords::input::format
			{
			public:
				Coordinates read(std::string) override;
			private:
				struct line
				{
					Cartesian_Point position;
					std::vector<std::size_t> bonds;
					std::size_t index, tinker_type;
					std::string symbol;
					line(void)
						: bonds(7U, 0U), index(0U),
						tinker_type(0U) {}
					friend std::istream& operator>> (std::istream& is, line& l)
					{
						is >> l.index >> l.symbol >> l.position.x() >> l.position.y() >> l.position.z() >> l.tinker_type
							>> l.bonds[0u] >> l.bonds[1u] >> l.bonds[2u] >> l.bonds[3u] >> l.bonds[4u] >> l.bonds[5u] >> l.bonds[6u];
						return is;
					}
				};



			};
			struct helper_base {
				static coords::Representation_3D ang_from_bohr(coords::Representation_3D const& rep3D) {
					coords::Representation_3D result;
					for (auto const& a : rep3D) {
						result.emplace_back(a * energy::bohr2ang);
					}
					return result;
				}

				static std::string removeBlanksFromString(std::string const& s) {
					std::string ret{ s };
					ret.erase(std::remove_if(ret.begin(), ret.end(), [](unsigned char c) {
						return std::isspace(c);
					}), ret.end());
					return ret;
				}
			};
		}
	}

	/**namespace for structure output in different formats*/
	namespace output
	{
		/**motherclass of all output formats*/
		class format
		{
		protected:
			Coordinates const& ref;
			format(Coordinates const& coord_obj) : ref(coord_obj) {}
		public:
			/**this is the function where you have to look if you want to know how the format will look like
			has to be overwritten for all newly implemented formats*/
			virtual void to_stream(std::ostream&) const = 0;
		};

		inline std::ostream& operator<< (std::ostream& stream, format const& out_format)
		{
			out_format.to_stream(stream);
			return stream;
		}

		namespace formats
		{
			/**tinker format*/
			class tinker : public output::format
			{
			public:
				tinker(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".arc")).get_unique_path();
				}
			};

			/**xyz format*/
			class xyz
				: public output::format
			{
			public:
				xyz(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".xyz")).get_unique_path();
				}
			};

			class xyz_cast
				: public output::format
			{
			public:
				xyz_cast(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".xyz")).get_unique_path();
				}
			};

			class xyz_dftb
				: public output::format
			{
			public:
				xyz_dftb(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".xyz")).get_unique_path();
				}
			};

			class xyz_gen
				: public output::format
			{
			public:
				xyz_gen(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".xyz")).get_unique_path();
				}
			};

			class xyz_mopac
				: public output::format
			{
			public:
				xyz_mopac(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return std::string(Config::get().general.outputFilename).append(postfix).append(".xyz");
				}
			};
			class moldenxyz
				: public output::format
			{
			public:
				moldenxyz(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".xyz")).get_unique_path();
				}
			};

			/**output of z matrix*/
			class zmatrix
				: public output::format
			{
			public:
				zmatrix(Coordinates const& coord_obj) : output::format(coord_obj) {}
				void to_stream(std::ostream&) const;
				static std::string filename(std::string postfix)
				{
					return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(".zm")).get_unique_path();
				}
			};

			/**pdb output*/
			class pdb : public output::format
			{
			public:
				/**constructor*/
				pdb(Coordinates const& coord_obj) : output::format(coord_obj) {
					pdb_atoms.resize(coord_obj.size());
				}
				/**prepare system for output
				i. e. determine aminoacids and other residues*/
				void preparation();
				/**output function*/
				void to_stream(std::ostream&) const;

			private:
				/**vector of atoms in a format that can be written to PDB*/
				std::vector<PDBAtom> pdb_atoms;

				/**function that creates pdb atoms for atoms that are not part of an aminoacid
				has to be called for every molecule
				@param molecule: molecule for which pdb atoms should be created
				@param residue_counter: counter for residues*/
				void set_pdb_atoms_of_molecule(Container<std::size_t> const& molecule, int residue_counter);
				/**function to find residue names for a molecule
				currently water ("H2O") and sodium ions ("NA") are recognized, everything else is "XXX"
				@param molecule: current molecule*/
				std::string get_resname_for_molecule(Container<std::size_t> const& molecule);
			};

		}

		inline std::string filename(std::string postfix = "", std::string extension = "")
		{
			if (extension.length() > 0)
			{
				return scon::StringFilePath(std::string(Config::get().general.outputFilename).append(postfix).append(extension)).get_unique_path();
			}
			if (Config::get().general.output == config::output_types::TINKER) return formats::tinker::filename(postfix);
			else if (Config::get().general.output == config::output_types::XYZ) return formats::xyz::filename(postfix);
			else if (Config::get().general.output == config::output_types::MOLDEN) return formats::moldenxyz::filename(postfix);
			else if (Config::get().general.output == config::output_types::ZMATRIX) return formats::zmatrix::filename(postfix);
			return scon::StringFilePath(Config::get().general.outputFilename).get_unique_path();
		}
	}
}
#endif