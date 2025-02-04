#include <vector>
#include "configuration.h"
#include "coords.h"
#include "optimization_global.h"
#include "optimization_dimer.h"
#include "Scon/scon_utility.h"
#include "Scon/scon_chrono.h"

optimization::global::CoordsOptimizationTS::CoordsOptimizationTS(
	coords::Coordinates* coords)
	: coords(coords) {}
coords::Gradients_Main optimization::global::CoordsOptimizationTS::dimermethod_dihedral(
	std::vector<coords::Gradients_Main> const& tabu_direction) {
	if (coords->atoms().mains().empty()) {
		throw std::runtime_error("System does not contain any main dihedrals. "
			"Dimermethod cannot be applied.");
	}
	using Move_T = optimization::CG_DimerMover<coords::Main_Callback>;
	coords::Main_Callback C(*coords);
	Move_T mover(0.1, 1000u);
	coords::Gradients_Main structure;
	std::size_t const N = coords->m_representation.structure.main.size();
	structure.reserve(N);
	for (std::size_t i(0u); i < N; ++i) {
		structure.emplace_back(coords->m_representation.structure.main[i].radians());
	}
	Move_T::minimum_type minimum(structure);
	minimum.directions = tabu_direction;
	C = mover(minimum, C);
	return minimum.directions.back();
}

optimization::global::optimizers::tabuSearch::tabuSearch(
	coords::Coordinates& c, std::string const& output_name) :
	optimizer(c, output_name), divers_optimizer(new_divers_optimizer(c))
{
}

optimization::global::optimizers::tabuSearch::tabuSearch(
	coords::Coordinates& c, coords::Ensemble_PES const& p, std::string const& output_name) :
	optimizer(c, p, false, output_name), divers_optimizer(new_divers_optimizer(c))
{
}

bool optimization::global::optimizers::tabuSearch::run(std::size_t const iterations, bool const reset)
{
	double energy(0.0);
	if (reset)
	{
		i = 0;
		opt_clock.restart();
		header_to_cout();
	}

	coordobj.set_pes(accepted_minima[min_index].pes);

	init_stereo = coordobj.stereos();

	std::size_t const iter_size(scon::num_digits(Config::get().optimization.global.iterations) + 1);

	if (Config::get().optimization.global.tabusearch.mcm_first)
		diversification();

	std::size_t fails(0);
	for (; i < iterations; ++i)
	{
		std::cout << "start iteration\n";
		scon::chrono::high_resolution_timer step_timer;
		coordobj.to_internal_to_xyz();
		std::cout << "finished conversion\n";
		ascent();
		std::cout << "finished ascent\n";
		if (Config::get().general.verbosity > 1U)
		{
			std::cout << "TS  ";
			// iteration
			std::cout << std::right << std::setw(iter_size) << i + 1 << "/";
			std::cout << std::left << std::setw(iter_size) << Config::get().optimization.global.iterations << ' ';
			// Curennt minimum index
			std::cout << std::setw(10) << min_index << ' ';
			// Current minimum
			std::cout << std::setw(Config::get().optimization.global.precision + 10) << std::left;
			std::cout << std::setprecision(Config::get().optimization.global.precision);
			std::cout << std::showpoint << std::scientific << accepted_minima[min_index].pes.energy << ' ';
			// Transition
			std::cout << std::setw(Config::get().optimization.global.precision + 10) << std::left;
			std::cout << std::setprecision(Config::get().optimization.global.precision);
			std::cout << std::showpoint << std::scientific << coordobj.pes().energy << ' ';
		}
		energy = descent();
		std::cout << "finished descent\n";
		coordobj.to_internal_to_xyz();
		std::cout << "finished conversion\n";
		min_status::T const status(check_pes_of_coords());
		std::cout << "status\n";

		if (!success(status))
			++fails;
		else
			fails = 0;

		if (Config::get().general.verbosity > 1U)
		{
			// final energy
			std::cout << std::setw(Config::get().optimization.global.precision + 10) << std::left;
			std::cout << std::setprecision(Config::get().optimization.global.precision);
			std::cout << std::showpoint << std::scientific << energy << ' ';
			// status
			std::cout << status;
			// number of accpeted and range minima
			std::cout << std::setw(10) << std::left << accepted_minima.size();
			std::cout << std::setw(10) << std::left << range_minima.size();
		}
		if (Config::get().general.verbosity > 1U)
		{
			std::cout << "(" << std::setprecision(2) << std::showpoint << std::fixed << T << " K, " << step_timer << ")\n";
		}
		if (!restore(status) || fails > Config::get().optimization.global.fallback_limit)
		{
			if (fallbacks < Config::get().optimization.global.tabusearch.divers_limit)
			{
				diversification();
				fails = 0;
			}
			else
			{
				std::cout << "Diversification limit reached. Stop.\n";
				break;
			}
		}
	}

	if (i > 0)
		--i;
	return found_new_minimum;
}


void optimization::global::optimizers::tabuSearch::ascent(void)
{
	std::size_t const tabu_dirs(accepted_minima[min_index].main_direction.size());
	CoordsOptimizationTS cts(&coordobj);
	if (tabu_dirs < coordobj.main().size() && tabu_dirs < 6)
	{
		accepted_minima[min_index].main_direction.push_back
		(
			cts.dimermethod_dihedral(accepted_minima[min_index].main_direction)
		);
	}
	else
	{
		accepted_minima[min_index].main_direction.clear();
		accepted_minima[min_index].main_direction.push_back
		(
			cts.dimermethod_dihedral()
		);
	}
}


double optimization::global::optimizers::tabuSearch::descent(void)
{
	return coordobj.o();
}


bool optimization::global::optimizers::tabuSearch::diversification(void)
{
	++fallbacks;
	// iterations is at max number in divers_iterations but not more than iteration count
	std::size_t div_iter_limit = std::min(Config::get().optimization.global.iterations,
		i + Config::get().optimization.global.tabusearch.divers_iterations);
	swap(*divers_optimizer);
	for (auto& m : divers_optimizer->accepted_minima)
	{
		m.visited = 0;
	}
	bool ret = divers_optimizer->run(div_iter_limit);
	swap(*divers_optimizer);
	return ret;
}
