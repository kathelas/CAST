#ifdef USE_PYTHON
#include "energy_int_dftbaby.h"


/*
dftb sysCall functions
*/

void energy::interfaces::dftbaby::create_dftbaby_configfile()
{
	std::ofstream file("dftbaby.cfg");
	file << "[DFTBaby]\n\n";
	file << "gradient_file = " + Config::get().energy.dftbaby.gradfile + "\n\n"; // has to be set in any case cause otherwise gradients are not calculated
	file << "gradient_state = " + std::to_string(Config::get().energy.dftbaby.gradstate) + "\n\n";  // set because standard not ground state
	file << "verbose = " + std::to_string(Config::get().energy.dftbaby.verbose) + "\n\n";
	if (Config::get().energy.dftbaby.longrange == false)
	{    //in dftbaby long range correction is standard
		file << "long_range_correction = 0\n\n";
	}
	if (Config::get().energy.dftbaby.cutoff != 0)
		file << "distance_cutoff = " + std::to_string(Config::get().energy.dftbaby.cutoff) + "\n\n";
	if (Config::get().energy.dftbaby.lr_dist != 0)
		file << "long_range_radius = " + std::to_string(Config::get().energy.dftbaby.lr_dist) + "\n\n";
	if (Config::get().energy.dftbaby.maxiter != 0)
		file << "maxiter = " + std::to_string(Config::get().energy.dftbaby.maxiter) + "\n\n";
	if (Config::get().energy.dftbaby.conv_threshold != "0")
		file << "scf_conv = " + Config::get().energy.dftbaby.conv_threshold + "\n\n";
	if (Config::get().energy.dftbaby.states != 0)
		file << "nstates = " + std::to_string(Config::get().energy.dftbaby.states) + "\n\n";
	if (Config::get().energy.dftbaby.orb_occ != 0)
		file << "nr_active_occ = " + std::to_string(Config::get().energy.dftbaby.orb_occ) + "\n\n";
	if (Config::get().energy.dftbaby.orb_virt != 0)
		file << "nr_active_virt = " + std::to_string(Config::get().energy.dftbaby.orb_virt) + "\n\n";
	if (Config::get().energy.dftbaby.diag_conv != "0")
		file << "diag_conv = " + Config::get().energy.dftbaby.diag_conv + "\n\n";
	if (Config::get().energy.dftbaby.diag_maxiter != 0)
		file << "diag_maxiter = " + std::to_string(Config::get().energy.dftbaby.diag_maxiter) + "\n\n";
	file.close();
}

std::string energy::interfaces::dftbaby::create_pythonpath(std::string numpath, std::string scipath)
{
	std::string pythonpaths_str = Py_GetPath();
	std::string path;
#ifdef __unix__
	std::vector<std::string> pythonpaths = split(pythonpaths_str, ':');
#elif defined(_WIN32) || defined(WIN32)
	std::vector<std::string> pythonpaths = split(pythonpaths_str, ';');
#endif
	path = "import sys\n";
	for (auto p : pythonpaths)  //keep pythonpath of system
	{
		path += "sys.path.append('" + p + "')\n";
	}
	path += "sys.path.append('" + Config::get().energy.dftbaby.path + "')\n"; //path to DFTBaby
	path += "sys.path.append('" + numpath + "')\n";                        //path to numpy
	path += "sys.path.append('" + scipath + "')\n";                        //path to scipy
	return path;
}

energy::interfaces::dftbaby::sysCallInterface::sysCallInterface(coords::Coordinates* cp) :
	energy::interface_base(cp),
	e_bs(0.0), e_coul(0.0), e_rep(0.0), e_tot(0.0)
{
	std::string numpath = get_python_modulepath("numpy");
	std::string scipath = get_python_modulepath("scipy");
	add_path = create_pythonpath(numpath, scipath);
	create_dftbaby_configfile();
	optimizer = Config::get().energy.dftbaby.opt;
}

energy::interfaces::dftbaby::sysCallInterface::sysCallInterface(sysCallInterface const& rhs, coords::Coordinates* cobj) :
	interface_base(cobj),
	e_bs(rhs.e_bs), e_coul(rhs.e_coul), e_rep(rhs.e_rep), e_tot(rhs.e_tot)
{
	interface_base::operator=(rhs);
}

energy::interface_base* energy::interfaces::dftbaby::sysCallInterface::clone(coords::Coordinates* coord_object) const
{
	sysCallInterface* tmp = new sysCallInterface(*this, coord_object);
	return tmp;
}

energy::interface_base* energy::interfaces::dftbaby::sysCallInterface::move(coords::Coordinates* coord_object)
{
	sysCallInterface* tmp = new sysCallInterface(*this, coord_object);
	return tmp;
}

void energy::interfaces::dftbaby::sysCallInterface::swap(interface_base& rhs)
{
	swap(dynamic_cast<sysCallInterface&>(rhs));
}

void energy::interfaces::dftbaby::sysCallInterface::swap(sysCallInterface& rhs)
{
	interface_base::swap(rhs);
}

energy::interfaces::dftbaby::sysCallInterface::~sysCallInterface(void)
{

}

/*
Energy class functions that need to be overloaded
*/

// Energy function
double energy::interfaces::dftbaby::sysCallInterface::e(void)
{
	integrity = true;

	//write inputstructure
	std::ofstream file("tmp_struc.xyz");
	file << coords::output::formats::xyz_dftb(*this->coords);
	file.close();

	//call programme
	std::string result_str;
	PyObject* modul, * funk, * prm, * ret;

	PySys_SetPath((char*)"./python_modules"); //set path
	const char* c = add_path.c_str();  //add paths from variable add_path
	PyRun_SimpleString(c);

	modul = PyImport_ImportModule("dftbaby_interface"); //import module

	if (modul)
	{
		funk = PyObject_GetAttrString(modul, "calc_energies"); //create function
		prm = Py_BuildValue("(ss)", "tmp_struc.xyz", "dftbaby.cfg"); //give parameters
		ret = PyObject_CallObject(funk, prm);  //call function with parameters
		result_str = PyString_AsString(ret); //convert result to a C++ string
		if (result_str != "error")  //if DFTBaby was successfull
		{
			result_str = result_str.substr(1, result_str.size() - 2);  //process return
			std::vector<std::string> result_vec = split(result_str, ',');

			//read energies and convert them to kcal/mol
			e_bs = std::stod(result_vec[0]) * energy::au2kcal_mol;
			e_coul = std::stod(result_vec[1]) * energy::au2kcal_mol;
			e_rep = std::stod(result_vec[3]) * energy::au2kcal_mol;
			e_tot = std::stod(result_vec[4]) * energy::au2kcal_mol;
			if (result_vec.size() == 6) e_lr = std::stod(result_vec[5]) * energy::au2kcal_mol;
			else e_lr = 0;
		}
		else
		{
			if (Config::get().general.verbosity >= 2)
			{
				std::cout << "DFTBaby gave an error. Treating structure as broken.\n";
			}
			e_bs = 0;
			e_coul = 0;
			e_rep = 0;
			e_tot = 0;
			e_lr = 0;
			integrity = false;
		}

		//delete PyObjects
		Py_DECREF(prm);
		Py_DECREF(ret);
		Py_DECREF(funk);
		Py_DECREF(modul);
	}
	else
	{
		throw std::runtime_error("ERROR: module dftbaby_interface not found");
	}
	std::remove("tmp_struc.xyz"); // delete file
	return e_tot;
}

// Energy+Gradient function
double energy::interfaces::dftbaby::sysCallInterface::g(void)
{
	integrity = true;

	// write inputstructure
	std::ofstream file("tmp_struc.xyz");
	file << coords::output::formats::xyz_dftb(*this->coords);
	file.close();

	//call programme
	std::string result_str;
	PyObject* modul, * funk, * prm, * ret;

	PySys_SetPath((char*)"./python_modules"); //set path
	const char* c = add_path.c_str();  //add paths from variable add_path
	PyRun_SimpleString(c);

	modul = PyImport_ImportModule("dftbaby_interface"); //import module

	if (modul)
	{
		funk = PyObject_GetAttrString(modul, "calc_gradients"); //create function
		prm = Py_BuildValue("(ss)", "tmp_struc.xyz", "dftbaby.cfg"); //give parameters
		ret = PyObject_CallObject(funk, prm);  //call function with parameters

		result_str = PyString_AsString(ret); //read function return (has to be a string)
		if (result_str != "error")
		{
			result_str = result_str.substr(1, result_str.size() - 2);  //process return
			std::vector<std::string> result_vec = split(result_str, ',');

			//read energies and convert them to kcal/mol
			e_bs = std::stod(result_vec[0]) * energy::au2kcal_mol;
			e_coul = std::stod(result_vec[1]) * energy::au2kcal_mol;
			e_rep = std::stod(result_vec[3]) * energy::au2kcal_mol;
			e_tot = std::stod(result_vec[4]) * energy::au2kcal_mol;
			if (result_vec.size() == 6) e_lr = std::stod(result_vec[5]) * energy::au2kcal_mol;
			else e_lr = 0;
		}
		else
		{
			if (Config::get().general.verbosity >= 2)
			{
				std::cout << "DFTBaby gave an error. Treating structure as broken.\n";
			}
			e_bs = 0;
			e_coul = 0;
			e_rep = 0;
			e_tot = 0;
			e_lr = 0;
			integrity = false;
		}

		//delete PyObjects
		Py_DECREF(prm);
		Py_DECREF(ret);
		Py_DECREF(funk);
		Py_DECREF(modul);
	}
	else
	{
		throw std::runtime_error("ERROR: module dftbaby_interface not found");
	}

	double CONVERSION_FACTOR = energy::Hartree_Bohr2Kcal_MolAng;  // hartree/bohr -> kcal/(mol*A)

	if (integrity == true) //read gradients
	{
		std::string line;
		coords::Representation_3D g_tmp;
		std::ifstream infile(Config::get().energy.dftbaby.gradfile);
		std::getline(infile, line);  //discard fist two lines
		std::getline(infile, line);
		std::string element;
		double x, y, z;
		while (infile >> element >> x >> y >> z)  //read gradients and convert them to kcal/mol
		{
			coords::Cartesian_Point g(x * CONVERSION_FACTOR, y * CONVERSION_FACTOR, z * CONVERSION_FACTOR);
			g_tmp.push_back(g);
		}
		infile.close();
		const char* gradfile = Config::get().energy.dftbaby.gradfile.c_str();
		std::remove(gradfile); // delete file
		coords->swap_g_xyz(g_tmp); //give gradients to coordobject
	}

	std::remove("tmp_struc.xyz"); // delete file
	return e_tot;
}

// Hessian function
double energy::interfaces::dftbaby::sysCallInterface::h(void)
{
	integrity = true;

	//write inputstructure
	std::ofstream file("tmp_struc.xyz");
	file << coords::output::formats::xyz_dftb(*this->coords);
	file.close();

	//call programme
	std::string result_str;
	PyObject* modul, * funk, * prm, * ret;

	PySys_SetPath((char*)"./python_modules"); //set path
	const char* c = add_path.c_str();  //add paths from variable add_path
	PyRun_SimpleString(c);

	modul = PyImport_ImportModule("dftbaby_interface"); //import module

	if (modul)
	{
		funk = PyObject_GetAttrString(modul, "hessian"); //create function
		prm = Py_BuildValue("(ss)", "tmp_struc.xyz", "dftbaby.cfg"); //give parameters
		ret = PyObject_CallObject(funk, prm);  //call function with parameters

		result_str = PyString_AsString(ret); //read function return (has to be a string)
		if (result_str != "error")
		{
			result_str = result_str.substr(1, result_str.size() - 2);  //process return
			std::vector<std::string> result_vec = split(result_str, ',');

			//read energies and convert them to kcal/mol
			e_bs = std::stod(result_vec[0]) * 627.503;
			e_coul = std::stod(result_vec[1]) * 627.503;
			e_rep = std::stod(result_vec[3]) * 627.503;
			e_tot = std::stod(result_vec[4]) * 627.503;
			if (result_vec.size() == 6) e_lr = std::stod(result_vec[5]) * 627.503;
			else e_lr = 0;
		}
		else
		{
			if (Config::get().general.verbosity >= 2)
			{
				std::cout << "DFTBaby gave an error. Treating structure as broken.\n";
			}
			e_bs = 0;
			e_coul = 0;
			e_rep = 0;
			e_tot = 0;
			e_lr = 0;
			integrity = false;
		}

		//delete PyObjects
		Py_DECREF(prm);
		Py_DECREF(ret);
		Py_DECREF(funk);
		Py_DECREF(modul);
	}
	else
	{
		throw std::runtime_error("ERROR: module dftbaby_interface not found");
	}

	double CONVERSION_FACTOR = energy::Hartree_Bohr2Kcal_MolAngSquare;

	if (integrity == true) //read hessian
	{
		std::string line;
		std::ifstream infile("hessian.txt");
		std::vector<std::vector<double>> hess;
		while (std::getline(infile, line))  //for every line
		{
			std::vector<std::string> linevec = split(line, ' ');
			std::vector<double> doublevec;
			for (auto v : linevec)
			{
				doublevec.push_back(std::stod(v) * CONVERSION_FACTOR);
			}
			hess.push_back(doublevec);
		}
		infile.close();

		coords->set_hessian(hess);  //set hessian
		std::remove("hessian.txt"); // delete file
	}
	std::remove("tmp_struc.xyz"); // delete file

	return e_tot;
}

// Optimization
double energy::interfaces::dftbaby::sysCallInterface::o(void)
{
	//write inputstructure
	std::ofstream file("tmp_struc.xyz");
	file << coords::output::formats::xyz_dftb(*this->coords);
	file.close();

	//call programme
	std::string result_str;
	PyObject* modul, * funk, * prm, * ret;

	PySys_SetPath((char*)"./python_modules"); //set path
	const char* c = add_path.c_str();  //add paths from variable add_path
	PyRun_SimpleString(c);

	modul = PyImport_ImportModule("dftbaby_interface"); //import module

	if (modul)
	{
		funk = PyObject_GetAttrString(modul, "opt"); //create function
		prm = Py_BuildValue("(ss)", "tmp_struc.xyz", "dftbaby.cfg"); //give parameters
		ret = PyObject_CallObject(funk, prm);  //call function with parameters

		result_str = PyString_AsString(ret); //read function return (has to be a string)
		if (result_str != "error")
		{
			result_str = result_str.substr(1, result_str.size() - 2);  //process return
			std::vector<std::string> result_vec = split(result_str, ',');

			//read energies and convert them to kcal/mol
			e_bs = std::stod(result_vec[0]) * energy::au2kcal_mol;
			e_coul = std::stod(result_vec[1]) * energy::au2kcal_mol;
			e_rep = std::stod(result_vec[3]) * energy::au2kcal_mol;
			e_tot = std::stod(result_vec[4]) * energy::au2kcal_mol;
			if (result_vec.size() == 6) e_lr = std::stod(result_vec[5]) * energy::au2kcal_mol;
			else e_lr = 0;
		}
		else
		{
			if (Config::get().general.verbosity >= 2)
			{
				std::cout << "DFTBaby gave an error. Treating structure as broken.\n";
			}
			e_bs = 0;
			e_coul = 0;
			e_rep = 0;
			e_tot = 0;
			e_lr = 0;
			integrity = false;
		}

		//delete PyObjects
		Py_DECREF(prm);
		Py_DECREF(ret);
		Py_DECREF(funk);
		Py_DECREF(modul);
	}
	else
	{
		throw std::runtime_error("ERROR: module dftbaby_interface not found");
	}

	if (integrity == true)   //read new geometry
	{
		std::string line;
		coords::Representation_3D xyz_tmp;
		std::ifstream infile("tmp_struc_opt.xyz");
		std::getline(infile, line);  //discard fist two lines
		std::getline(infile, line);
		std::string element;
		double x, y, z;
		while (infile >> element >> x >> y >> z)  //new coordinates
		{
			coords::Cartesian_Point xyz(x, y, z);
			xyz_tmp.push_back(xyz);
		}
		infile.close();
		coords->set_xyz(std::move(xyz_tmp));

		std::remove("tmp_struc_opt.xyz"); // delete file
	}

	std::remove("tmp_struc.xyz"); // delete file
	return e_tot;
}

// Output functions
void energy::interfaces::dftbaby::sysCallInterface::print_E(std::ostream& S) const
{
	S << "Total Energy:      ";
	S << std::right << std::setw(16) << std::fixed << std::setprecision(8) << e_tot;
}

void energy::interfaces::dftbaby::sysCallInterface::print_E_head(std::ostream& S, bool const endline) const
{
	S << "Energies\n";
	S << std::right << std::setw(24) << "E_bs";
	S << std::right << std::setw(24) << "E_coul";
	S << std::right << std::setw(24) << "E_lr";
	S << std::right << std::setw(24) << "E_rep";
	S << std::right << std::setw(24) << "SUM\n";
	if (endline) S << "\n";
}

void energy::interfaces::dftbaby::sysCallInterface::print_E_short(std::ostream& S, bool const endline) const
{
	S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << e_bs;
	S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << e_coul;
	S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << e_lr;
	S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << e_rep;
	S << std::right << std::setw(24) << std::fixed << std::setprecision(8) << e_tot << '\n';
	if (endline) S << "\n";
}

void energy::interfaces::dftbaby::sysCallInterface::to_stream(std::ostream&) const { }

std::vector<coords::float_type>
energy::interfaces::dftbaby::sysCallInterface::charges() const
{
	if (file_exists("dftb_charges.txt") == false)
	{
		throw std::runtime_error("dftbaby chargefile not found.");
	}

	std::vector<coords::float_type> charges;
	std::vector<std::string> chargestrings;
	std::string line;

	std::ifstream chargefile("dftb_charges.txt", std::ios_base::in);
	std::getline(chargefile, line);
	chargestrings = split(line, ';');

	for (auto charge : chargestrings)
	{
		charges.push_back(std::stof(charge));
	}
	return charges;
}
#endif
