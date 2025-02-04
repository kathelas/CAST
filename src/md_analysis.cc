#include"md_analysis.h"
#include"configuration.h"
#include"md.h"

/**function that fills zones with atoms*/
std::vector<md_analysis::zone> md_analysis::find_zones(md::simulation *md_obj)
{
	std::vector<zone> zones;
	double zone_width = Config::get().md.zone_width;

	// calculate center and distances to it
	std::vector<double> dists = md_obj->calc_distances_from_center();

	// find out number of zones (maximum distance to active site)
	std::vector<double>::iterator it = std::max_element(dists.begin(), dists.end());
	double max_dist = *it;
	int number_of_zones = std::ceil(max_dist / zone_width);
	zones.resize(number_of_zones);

	// create zones, fill them with atoms and create legend
	for (auto i = 0u; i < dists.size(); i++)
	{
		int zone = std::floor((dists)[i] / zone_width);
		zones[zone].atoms.push_back(i);
	}
	for (auto i = 0u; i < zones.size(); i++)
	{
		zones[i].legend = std::to_string(int(i * zone_width)) + " to " + std::to_string(int((i + 1) * zone_width));
	}

	// output
	if (Config::get().general.verbosity > 2)
	{
		std::cout << "Zones:\n";
		for (auto i = 0u; i < zones.size(); i++)
		{
			std::cout << zones[i].atoms.size() << " atoms in zone from " << i * zone_width << " to " << (i + 1) * zone_width << "\n";
		}
	}
	return zones;
}

void md_analysis::write_zones_into_file(md::simulation* md_obj)
{
	std::ofstream zonefile;
	zonefile.open("zones.csv");

	zonefile << "Steps";                               // write headline
	for (auto& z : md_obj->zones) zonefile << "," << z.legend;
	zonefile << "\n";

	for (auto i = 0u; i < Config::get().md.num_steps; ++i)   // for every MD step
	{
		zonefile << i + 1;
		for (auto& z : md_obj->zones) zonefile << "," << z.temperatures[i];  // write a line with temperatures
		zonefile << "\n";
	}
	zonefile.close();
}

void md_analysis::write_dists_into_file(md::simulation* md_obj)
{
	std::ofstream distfile;
	distfile.open("distances.csv");

	distfile << "Steps";                               // write headline
	for (auto const& p : md_obj->ana_pairs) distfile << "," << p.legend;
	distfile << "\n";

	for (auto i = 0u; i < Config::get().md.num_steps; ++i)   // for every MD step
	{
		distfile << i + 1;
		for (auto const& p : md_obj->ana_pairs) distfile << "," << p.dists[i];  // write a line with distances
		distfile << "\n";
	}
	distfile.close();
}

#ifdef USE_PYTHON

/**function to plot temperatures for all zones*/
void md_analysis::plot_zones(md::simulation* md_obj)
{
	write_zones_into_file(md_obj);

	std::string add_path = md_obj->get_pythonpath();

	PyObject* modul, * funk, * prm, * ret, * pValue;

	// create python list with legends
	PyObject* legends = PyList_New(md_obj->zones.size());
	for (std::size_t k = 0; k < md_obj->zones.size(); k++) {
		pValue = PyString_FromString(md_obj->zones[k].legend.c_str());
		PyList_SetItem(legends, k, pValue);
	}

	// create a python list that contains a list with temperatures for every zone
	PyObject* temp_lists = PyList_New(md_obj->zones.size());
	int counter = 0;
	for (auto z : md_obj->zones)
	{
		PyObject* temps = PyList_New(z.temperatures.size());
		for (std::size_t k = 0; k < z.temperatures.size(); k++) {
			pValue = PyFloat_FromDouble(z.temperatures[k]);
			PyList_SetItem(temps, k, pValue);
		}
		PyList_SetItem(temp_lists, counter, temps);
		counter += 1;
	}

	PySys_SetPath((char*)"./python_modules"); //set path
	const char* c = add_path.c_str();  //add paths pythonpath
	PyRun_SimpleString(c);

	modul = PyImport_ImportModule("MD_analysis"); //import module 
	if (modul)
	{
		funk = PyObject_GetAttrString(modul, "plot_zones"); //create function
		prm = Py_BuildValue("(OO)", legends, temp_lists); //give parameters
		ret = PyObject_CallObject(funk, prm);  //call function with parameters
		std::string result_str = PyString_AsString(ret); //convert result to a C++ string
		if (result_str == "error")
		{
			std::cout << "An error occured during running python module 'MD_analysis'\n";
		}
	}
	else
	{
		throw std::runtime_error("Error: module 'MD_analysis' not found!");
	}
	//delete PyObjects
	Py_DECREF(prm);
	Py_DECREF(ret);
	Py_DECREF(funk);
	Py_DECREF(modul);
	Py_DECREF(pValue);
	Py_DECREF(legends);
	Py_DECREF(temp_lists);
}

void md_analysis::plot_distances(md::simulation* md_obj)
{
	write_dists_into_file(md_obj);

	std::string add_path = md_obj->get_pythonpath();

	PyObject* modul, * funk, * prm, * ret, * pValue;

	// create python list with legends
	PyObject* legends = PyList_New(md_obj->ana_pairs.size());
	for (std::size_t k = 0; k < md_obj->ana_pairs.size(); k++) {
		pValue = PyString_FromString(md_obj->ana_pairs[k].legend.c_str());
		PyList_SetItem(legends, k, pValue);
	}

	// create a python list that contains a list with distances for every atom pair that is to be analyzed
	PyObject* distance_lists = PyList_New(md_obj->ana_pairs.size());
	int counter = 0;

	for (auto const & a : md_obj->ana_pairs)
	{
		PyObject* dists = PyList_New(a.dists.size());
		for (std::size_t k = 0; k < a.dists.size(); k++) {
			pValue = PyFloat_FromDouble(a.dists[k]);
			PyList_SetItem(dists, k, pValue);
		}
		PyList_SetItem(distance_lists, counter, dists);
		counter += 1;
	}

	PySys_SetPath((char*)"./python_modules"); //set path
	const char* c = add_path.c_str();  //add paths pythonpath
	PyRun_SimpleString(c);

	modul = PyImport_ImportModule("MD_analysis"); //import module 
	if (modul)
	{
		funk = PyObject_GetAttrString(modul, "plot_dists"); //create function
		prm = Py_BuildValue("(OO)", legends, distance_lists); //give parameters
		ret = PyObject_CallObject(funk, prm);  //call function with parameters
		std::string result_str = PyString_AsString(ret); //convert result to a C++ string
		if (result_str == "error")
		{
			std::cout << "An error occured during running python module 'MD_analysis'\n";
		}
	}
	else
	{
		throw std::runtime_error("Error: module 'MD_analysis' not found!");
	}
	//delete PyObjects
	Py_DECREF(prm);
	Py_DECREF(ret);
	Py_DECREF(funk);
	Py_DECREF(modul);
	Py_DECREF(pValue);
	Py_DECREF(legends);
	Py_DECREF(distance_lists);
}
#endif

void md_analysis::add_analysis_info(md::simulation* md_obj)
{
	// calculate distances that should be analyzed
	if (md_obj->ana_pairs.size() > 0)
	{
		for (auto& p : md_obj->ana_pairs)
		{
			p.dists.push_back(dist(md_obj->get_coords().xyz(p.a), md_obj->get_coords().xyz(p.b)));
		}
	}

	// calculate average temperature for every zone
	if (Config::get().md.analyze_zones == true)
	{
		for (auto& z : md_obj->zones)
		{
			int dof = 3u * z.atoms.size();
			z.temperatures.push_back(md_obj->Ekin(z.atoms) * (2.0 / (dof * md::R)));
		}
	}
}

void md_analysis::write_and_plot_analysis_info(md::simulation* md_obj)
{
#ifdef USE_PYTHON
	// plot distances from MD analyzing
	if (Config::get().md.ana_pairs.size() > 0)
	{
		plot_distances(md_obj);
		for (auto& p : md_obj->ana_pairs) p.dists.clear();  
	}

// plot average temperatures of every zone
if (Config::get().md.analyze_zones == true) plot_zones(md_obj);
#else
if (Config::get().md.ana_pairs.size() > 0)
{
	std::cout << "Plotting is not possible without python!\n";
	write_dists_into_file(md_obj);
	for (auto& p : md_obj->ana_pairs) p.dists.clear();   
}
if (Config::get().md.analyze_zones == true)
{
	std::cout << "Plotting is not possible without python!\n";
	write_zones_into_file(md_obj); 
}
#endif
}

void md_analysis::create_ana_pairs(md::simulation* md_obj)
{
	for (auto p : Config::get().md.ana_pairs)
	{
		ana_pair ap(p[0], p[1]);
		ap.symbol_a = md_obj->get_coords().atoms(ap.a).symbol();
		ap.symbol_b = md_obj->get_coords().atoms(ap.b).symbol();
		ap.name_a = ap.symbol_a + std::to_string(ap.a + 1);
		ap.name_b = ap.symbol_b + std::to_string(ap.b + 1);
		ap.legend = ap.name_a + "-" + ap.name_b;
		md_obj->ana_pairs.push_back(ap);
	}
}