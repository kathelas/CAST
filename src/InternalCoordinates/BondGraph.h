#ifndef cast_graph_h_guard
#define cast_graph_h_guard

#include "../coords.h"
#include "../coords_rep.h"

#include <algorithm>
#include <array>
#include <boost/graph/adjacency_list.hpp>
#include <boost/variant/get.hpp>
#include <boost/graph/graphviz.hpp>
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

/*!
 *  \addtogroup ic_util
 *  @{
 */
namespace ic_util {

	/*!
	\details The Node struct and Graph class can be thought of as instantiations of
	the graph abstraction provided by the Boost Graph Library (BGL).
	\see "The Boost Graph Library", Jeremy Siek et al., Addison-Wesley, 2002
	*/

	/*!
	\brief Vertex class.
	\details User-defined class type for the VertexProperty template argument of
	boost::adjacency_list<>. The members of Node are the properties (a.k.a. labels)
	that each vertex possesses; in BGL terminology this is called bundled property.
	*/
	struct Node {
		unsigned int atom_serial;
		std::string atom_name;
		std::string element;
		coords::Cartesian_Point cp;
	};

	/*!
	\brief User-defined Graph class that represents an actual instantiation of BGL's
	graph abstraction.
	\tparam Atom_type User-defined type that encapsulates atom-specific data
	retrieved by the PDB parser.
	*/
	template <typename Atom_type>
	class Graph : public boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, Atom_type> {

	public:
		/*!
		\brief User-defined graph type.
		\details Basis of the graph is either an adjacency list or a matrix.
		\tparam vecS The first template param is the container used to store the edge
		list; in this case std::vector.
		\tparam vecS The second template param is the container used to store the
		vertex list; again std::vector is used.
		\tparam undirectedS Specifies that the graph is undirected.
		\tparam Node Specifies vertex properties.
		*/
		using Graph_type = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, Atom_type>;
	private:
		template<typename CoordsAtomType>
		Graph(const std::vector<std::pair<std::size_t, std::size_t>>& connectedAtoms,
			const std::vector<CoordsAtomType>& atomVector)
			: Graph_type{ create_graph(connectedAtoms, atomVector) } {}
	public:


		/*!
		\brief Creates the graph data structure. Called during instantiation of the
		Graph class.
		\param b_at Vector of bonded atom pairs.
		\param vec Vector of atoms.
		\return Graph_type object.
		*/
		template<typename CoordsAtomType>
		Graph_type create_graph(const std::vector<std::pair<std::size_t, std::size_t>>& connectedAtoms,
			const std::vector<CoordsAtomType>& atomVector) {
			Graph_type graph(connectedAtoms.cbegin(), connectedAtoms.cend(), atomVector.size());

			for (auto i = 0u; i < atomVector.size(); ++i) {
				graph[i].atom_serial = atomVector.at(i).atom_serial;
				graph[i].atom_name = atomVector.at(i).atom_name;
				graph[i].element = atomVector.at(i).element;
				graph[i].cp = atomVector.at(i).cp;
			}

			return graph;
		}

		/*!
		\brief Prints the edges of a graph to std::cout.
		\param graph Graph, whose edges shall be printed.
		\return The two vertices of an edge are printed.
		*/
		void print_edges(const Graph_type& graph) {
			using boost::edges;
			using boost::source;
			using boost::target;

			std::cout << "Bond graph edges: \n";
			auto ed = edges(graph);
			for (auto it = ed.first; it != ed.second; ++it) {
				auto u = source(*it, graph);
				auto v = target(*it, graph);
				std::cout << graph[u].atom_serial << " -> " << graph[v].atom_serial
					<< "\n";
			}
		}

		/*!
		\brief Prints the vertices of a graph to std::cout.
		\param graph Graph, whose vertices shall be printed.
		\return Each vertex with its associated properties is printed.
		*/
		void print_vertices(const Graph_type& graph) {
			using boost::vertices;

			std::cout << "Bond graph vertices: \n";
			auto vert = vertices(graph);
			for (auto it = vert.first; it != vert.second; ++it) {
				std::cout << "Atom: " << graph[*it].atom_serial << "; "
					<< graph[*it].atom_name << "; " << graph[*it].element << "; "
					<< graph[*it].cp << "\n";
			}
		}

		/*!
		\brief Specifies a graph in the DOT graph description language.
		\details To obtain a visual representation of the graph the resulting output
		file has to be parsed with GraphViz or similar software.
		\param String specifying the name of the output file.
		\return Text file containing the graph specification in the DOT file format.
		\see http://www.graphviz.org/content/dot-language
		\see http://www.graphviz.org/
		*/
		void visualize_graph(const std::string& out_file) {
			using boost::get;
			using boost::make_label_writer;
			using boost::write_graphviz;

			std::ofstream output(out_file);
			write_graphviz(output, *this, make_label_writer(get(&Node::atom_serial, *this)));
		}
		template<typename AtomType>
		friend Graph<Node> make_graph(std::vector<std::pair<std::size_t, std::size_t>> const& b_atoms, std::vector<AtomType> const& vec);
	};

	template<typename AtomType>
	Graph<Node> make_graph(std::vector<std::pair<std::size_t, std::size_t>> const& b_atoms, std::vector<AtomType> const& vec) {
		return Graph<Node>(b_atoms, vec);
	}

}

/*! @} End of ic_util group*/

#endif // cast_graph_h_guard
