#ifndef cast_ic_util_h_guard
#define cast_ic_util_h_guard

#include<cstring>
#include<type_traits>
#include<vector>
#include<array>

/*!
 *  \addtogroup ic_util
 *  @{
 */
namespace ic_util{

  using coords::float_type;

  inline coords::Representation_3D grads_to_bohr(coords::Representation_3D const& grads) {
	  coords::Representation_3D bohr_grads;
	  bohr_grads.reserve(grads.size());
	  for (auto const& g : grads) {
		  bohr_grads.emplace_back(g / energy::Hartree_Bohr2Kcal_MolAng);
	  }
	  return bohr_grads;
  }
  inline coords::Representation_3D rep3d_bohr_to_ang(coords::Representation_3D const& bohr) {
	  coords::Representation_3D ang;
	  ang.reserve(bohr.size());
	  for (auto const& b : bohr) {
		  ang.emplace_back(b * energy::bohr2ang);
	  }
	  return ang;
  }

  template<template <typename, typename, typename ...> class Map, typename Key, typename Value, typename DefValue, typename ... Args>
  inline Value getValueByKeyOrDefault(Map<Key, Value, Args...> const& map, Key const& key, DefValue const& defaultValue){
      typename Map<Key, Value, Args ...>::const_iterator it = map.find(key);
      if(it == map.end()){
          return defaultValue;
      }
      return it->second;
  }

  template<template <typename, typename...> class Vec, typename VecType, typename ... VecArgs>
  inline auto get_mean(Vec<VecType, VecArgs...> const & vec) {
    auto mean = std::accumulate(vec.begin(), vec.end(), VecType(), std::plus<VecType>());
    mean /= static_cast<float_type> (vec.size());
    return mean;
  }

  /*!
  \brief Converts a std::array to a std::vector.
  \tparam U Arithmetic type. Enforced by type traits.
  \tparam N Size of the array.
  \param arr Input array.
  \return std::vector.
  */
  template <template<typename, typename ...> class Conatiner, typename VectorType, typename ... VectorArguments> 
  inline typename std::enable_if<std::is_arithmetic<VectorType>::value, std::vector<VectorType>>::type
    arr_to_vec(const Conatiner<VectorType, VectorArguments ...>& container) {
    return std::vector<VectorType>(container.begin(), container.end());
  }
  template <typename U, std::size_t N>
  inline typename std::enable_if<std::is_arithmetic<U>::value, std::vector<U>>::type
  arr_to_vec(const std::array<U, N>& arr) {
    std::vector<U> res(arr.begin(), arr.end());
    return res;
  }

  template <template<typename> class MatrixType, typename T, template<typename> class CoordType, template <typename, typename ...> class ContainerType, typename ... ContainerArgs>
  inline typename std::enable_if<std::is_arithmetic<T>::value, MatrixType<T>>::type
  Rep3D_to_Mat(const ContainerType<CoordType<T>, ContainerArgs ...>& rep) {
    using Mat = MatrixType<T>;

    Mat A(rep.size(), 3);
    for (std::size_t i = 0; i != rep.size(); ++i) {
      A(i, 0) = rep.at(i).x();
      A(i, 1) = rep.at(i).y();
      A(i, 2) = rep.at(i).z();
    }
    return A;
  }


  // radius of gyration from Rep3D
  // coords have to be in Bohr
  template <typename T, template<typename> class CoordType, template<typename, typename ...> class ContainerType, typename ... ContainerArgs>
  inline typename std::enable_if<std::is_arithmetic<T>::value, T>::type
  rad_gyr(ContainerType<CoordType<T>, ContainerArgs...> const& rep) {
    auto mean = get_mean(rep);
    auto diff = rep - mean;
    auto sum{ 0.0 };
    for (auto& i : diff) {
      auto temp = std::pow(i.x(), 2) + std::pow(i.y(), 2) + std::pow(i.z(), 2);
      sum += temp;
    }
    sum /= rep.size();
    return std::sqrt(sum);
  }

  /*!
  \brief Normalizes a vector given as Cartesian_Point.
  \param a Vector, which shall be normalized.
  \return Normalized vector.
  */
  inline coords::Cartesian_Point normalize(const coords::Cartesian_Point& a) {
    return a / scon::geometric_length(a);
  }

  //not tested
  inline coords::Cartesian_Point
    normal_unit_vector(const coords::Cartesian_Point& a,
      const coords::Cartesian_Point& b,
      const coords::Cartesian_Point& c) {
    using scon::cross;
    using scon::len;

    auto a_vec = b - a;
    auto b_vec = b - c;
    auto t1 = cross(a_vec, b_vec);
    auto result = t1 / (len(t1));
    return result;
  }

  struct AtomConnector {
    using returnType = std::vector<std::pair<std::size_t, std::size_t>>;

    AtomConnector(std::vector<std::string> const& elem_vec,
      coords::Representation_3D const& cp_vec) : sequenceOfSymbols{ elem_vec }, cartesianRepresentation{ cp_vec }, firstAtomIndex{ 0u }, secondAtomIndex{ 0u } {}
    returnType operator()();

  private:
    double getThresholdForBeingNotConnected(std::string const& oneAtom, std::string const& otherAtom);
    void findTheFirstAtom(returnType & connectedAtoms);
    void findAtomWithIndexHigherThanFirst(returnType & connectedAtoms);
    void connectIfCloseEnough(returnType & connectedAtoms);
    bool areTheyCloseEnough();

    std::vector<std::string> const& sequenceOfSymbols;
    coords::Representation_3D const& cartesianRepresentation;

    std::size_t firstAtomIndex;
    std::size_t secondAtomIndex;
  };

  inline std::vector<std::pair<std::size_t, std::size_t>> AtomConnector::operator()() {
    std::vector<std::pair<std::size_t, std::size_t>> connectedAtoms;
    findTheFirstAtom(connectedAtoms);
    return connectedAtoms;
  }

  inline void AtomConnector::findTheFirstAtom(returnType & connectedAtoms){
    for (firstAtomIndex = 0u; firstAtomIndex < sequenceOfSymbols.size(); ++firstAtomIndex) {
      findAtomWithIndexHigherThanFirst(connectedAtoms);
    }
  }

  inline void AtomConnector::findAtomWithIndexHigherThanFirst(returnType & connectedAtoms){
    for (secondAtomIndex = firstAtomIndex+1u; secondAtomIndex < sequenceOfSymbols.size(); ++secondAtomIndex) {
      connectIfCloseEnough(connectedAtoms);
    }
  }

  inline void AtomConnector::connectIfCloseEnough(returnType & connectedAtoms) {
    if (areTheyCloseEnough()) {
      connectedAtoms.emplace_back(firstAtomIndex, secondAtomIndex);
    }
  }

  inline bool AtomConnector::areTheyCloseEnough() {
    double const threshold = getThresholdForBeingNotConnected(sequenceOfSymbols.at(firstAtomIndex), sequenceOfSymbols.at(secondAtomIndex));
    double const actualDistance = scon::len(cartesianRepresentation.at(firstAtomIndex) - cartesianRepresentation.at(secondAtomIndex));
    return actualDistance < threshold;
  }

  /*!
\brief Number of elements currently supported by the code.
*/
  static constexpr auto N_elem{ 97u };

  /*!
  \brief Covalent radii of the first N_elem elements of the PSE.
  \details Corresponds to the symbol array.
  \see "Covalent radii revisited", Cordero et al., Dalton Trans., 21, 2008,
  pp. 2832-2838
  */
  static constexpr std::array<double, N_elem> radius{
	0.00, 0.31, 0.28, 1.28, 0.96, 0.84, 0.76, 0.71, 0.66, 0.57, 0.58, 1.66, 1.41,
	1.21, 1.11, 1.07, 1.05, 1.02, 1.06, 2.03, 1.76, 1.70, 1.60, 1.53, 1.39, 1.61,
	1.52, 1.50, 1.24, 1.32, 1.22, 1.22, 1.20, 1.19, 1.20, 1.20, 1.16, 2.20, 1.95,
	1.90, 1.75, 1.64, 1.54, 1.47, 1.46, 1.42, 1.39, 1.45, 1.44, 1.42, 1.39, 1.39,
	1.38, 1.39, 1.40, 2.44, 2.15, 2.07, 2.04, 2.03, 2.01, 1.99, 1.98, 1.98, 1.96,
	1.94, 1.92, 1.92, 1.89, 1.90, 1.87, 1.87, 1.75, 1.70, 1.62, 1.51, 1.44, 1.41,
	1.36, 1.36, 1.32, 1.45, 1.46, 1.48, 1.40, 1.50, 1.50, 2.60, 2.21, 2.15, 2.06,
	2.00, 1.96, 1.90, 1.87, 1.80, 1.69
  };

  /*!
  \brief List containing the first N_elem of the PSE.
  \details Corresponds to the radii of the radius array.
  */
  static constexpr std::array<char const*, N_elem> symbol{
	"DD", "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg",
	"Al", "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",  "Cr", "Mn",
	"Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr",
	"Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb",
	"Te", "I",  "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd",
	"Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W",  "Re", "Os", "Ir",
	"Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th",
	"Pa", "U",  "Np", "Pu", "Am", "Cm"
  };

  enum class period : int {
	  none = 0,
	  one,
	  two,
	  three
  };

  static const std::unordered_map<std::string, std::tuple<std::size_t, float_type, period>> pse{
	{ "DD", std::make_tuple(0,0.0,period::one) },
	{ "H", std::make_tuple(1,0.31,period::one) },
	{ "He", std::make_tuple(2,0.28,period::one) },
	{ "Li", std::make_tuple(3,1.28,period::two) },
	{ "Be", std::make_tuple(4,0.96,period::two) },
	{ "B", std::make_tuple(5,0.84,period::two) },
	{ "C", std::make_tuple(6,0.76,period::two) },
	{ "N", std::make_tuple(7,0.71,period::two) },
	{ "O", std::make_tuple(8,0.66,period::two) },
	{ "F", std::make_tuple(9,0.57,period::two) },
	{ "Ne", std::make_tuple(10,0.58,period::two) },
	{ "Na", std::make_tuple(11,1.66,period::three) },
	{ "Mg", std::make_tuple(12,1.41,period::three) },
	{ "Al", std::make_tuple(13,1.21,period::three) },
	{ "Si", std::make_tuple(14,1.11,period::three) },
	{ "P", std::make_tuple(15,1.07,period::three) },
	{ "S", std::make_tuple(16,1.05,period::three) },
	{ "Cl", std::make_tuple(17,1.02,period::three) },
	{ "Ar", std::make_tuple(18,1.06,period::three) },
	{ "K", std::make_tuple(19,2.03,period::none) },
	{ "Ca", std::make_tuple(20,1.76,period::none) },
	{ "Sc", std::make_tuple(21,1.7,period::none) },
	{ "Ti", std::make_tuple(22,1.6,period::none) },
	{ "v", std::make_tuple(23,1.53,period::none) },
	{ "Cr", std::make_tuple(24,1.39,period::none) },
	{ "Mn", std::make_tuple(25,1.5,period::none) },
	{ "Fe", std::make_tuple(26,1.42,period::none) },
	{ "Co", std::make_tuple(27,1.38,period::none) },
	{ "Ni", std::make_tuple(28,1.24,period::none) },
	{ "Cu", std::make_tuple(29,1.32,period::none) },
	{ "Zn", std::make_tuple(30,1.22,period::none) },
	{ "Ga", std::make_tuple(31,1.22,period::none) },
	{ "Ge", std::make_tuple(32,1.2,period::none) },
	{ "As", std::make_tuple(33,1.19,period::none) },
	{ "Se", std::make_tuple(34,1.2,period::none) },
	{ "Br", std::make_tuple(35,1.2,period::none) },
	{ "Kr", std::make_tuple(36,1.16,period::none) },
	{ "Rb", std::make_tuple(37,2.2,period::none) },
	{ "Sr", std::make_tuple(38,1.95,period::none) },
	{ "Y", std::make_tuple(39,1.9,period::none) },
	{ "Zr", std::make_tuple(40,1.75,period::none) },
	{ "Nb", std::make_tuple(41,1.64,period::none) },
	{ "Mo", std::make_tuple(42,1.54,period::none) },
	{ "Tc", std::make_tuple(43,1.47,period::none) },
	{ "Ru", std::make_tuple(44,1.46,period::none) },
	{ "Rh", std::make_tuple(45,1.42,period::none) },
	{ "Pd", std::make_tuple(46,1.39,period::none) },
	{ "Ag", std::make_tuple(47,1.45,period::none) },
	{ "Cd", std::make_tuple(48,1.44,period::none) },
	{ "In", std::make_tuple(49,1.42,period::none) },
	{ "Sn", std::make_tuple(50,1.39,period::none) },
	{ "Sb", std::make_tuple(51,1.39,period::none) },
	{ "Te", std::make_tuple(52,1.38,period::none) },
	{ "I", std::make_tuple(53,1.39,period::none) },
	{ "Xe", std::make_tuple(54,1.4,period::none) },
	{ "Cs", std::make_tuple(55,2.44,period::none) },
	{ "Ba", std::make_tuple(56,2.15,period::none) },
	{ "La", std::make_tuple(57,2.07,period::none) },
	{ "Ce", std::make_tuple(58,2.04,period::none) },
	{ "Pr", std::make_tuple(59,2.03,period::none) },
	{ "Nd", std::make_tuple(60,2.01,period::none) },
	{ "Pm", std::make_tuple(61,1.99,period::none) },
	{ "Sm", std::make_tuple(62,1.98,period::none) },
	{ "Eu", std::make_tuple(63,1.98,period::none) },
	{ "Gd", std::make_tuple(64,1.96,period::none) },
	{ "Tb", std::make_tuple(65,1.94,period::none) },
	{ "Dy", std::make_tuple(66,1.92,period::none) },
	{ "Ho", std::make_tuple(67,1.92,period::none) },
	{ "Er", std::make_tuple(68,1.89,period::none) },
	{ "Tm", std::make_tuple(69,1.9,period::none) },
	{ "Yb", std::make_tuple(70,1.87,period::none) },
	{ "Lu", std::make_tuple(71,1.87,period::none) },
	{ "Hf", std::make_tuple(72,1.75,period::none) },
	{ "Ta", std::make_tuple(73,1.7,period::none) },
	{ "W", std::make_tuple(74,1.62,period::none) },
	{ "Re", std::make_tuple(75,1.51,period::none) },
	{ "Os", std::make_tuple(76,1.44,period::none) },
	{ "Ir", std::make_tuple(77,1.41,period::none) },
	{ "Pt", std::make_tuple(78,1.36,period::none) },
	{ "Au", std::make_tuple(79,1.36,period::none) },
	{ "Hg", std::make_tuple(80,1.32,period::none) },
	{ "Tl", std::make_tuple(81,1.45,period::none) },
	{ "Pb", std::make_tuple(82,1.46,period::none) },
	{ "Bi", std::make_tuple(83,1.48,period::none) },
	{ "Po", std::make_tuple(84,1.4,period::none) },
	{ "At", std::make_tuple(85,1.5,period::none) },
	{ "Rn", std::make_tuple(86,1.5,period::none) },
	{ "Fr", std::make_tuple(87,2.6,period::none) },
	{ "Ra", std::make_tuple(88,2.21,period::none) },
	{ "Ac", std::make_tuple(89,2.15,period::none) },
	{ "Th", std::make_tuple(90,2.06,period::none) },
	{ "Pa", std::make_tuple(91,2.0,period::none) },
	{ "U", std::make_tuple(92,1.96,period::none) },
	{ "Np", std::make_tuple(93,1.9,period::none) },
	{ "Pu", std::make_tuple(94,1.87,period::none) },
	{ "Am", std::make_tuple(95,1.8,period::none) },
	{ "Cm", std::make_tuple(96,1.69,period::none) },
	{ "Bk", std::make_tuple(97,1.68,period::none) },
	{ "Cf", std::make_tuple(98,1.68,period::none) },
	{ "Es", std::make_tuple(99,1.65,period::none) },
	{ "Fm", std::make_tuple(100,1.67,period::none) },
	{ "Md", std::make_tuple(101,1.73,period::none) },
	{ "No", std::make_tuple(102,1.76,period::none) },
	{ "Lr", std::make_tuple(103,1.61,period::none) },
	{ "Rf", std::make_tuple(104,1.57,period::none) },
	{ "Db", std::make_tuple(105,1.49,period::none) },
	{ "Sg", std::make_tuple(106,1.43,period::none) },
	{ "Bh", std::make_tuple(107,1.41,period::none) },
	{ "Hs", std::make_tuple(108,1.34,period::none) },
	{ "Mt", std::make_tuple(109,1.29,period::none) },
	{ "Ds", std::make_tuple(110,1.28,period::none) },
	{ "Rg", std::make_tuple(111,1.21,period::none) },
	{ "Cn", std::make_tuple(112,1.22,period::none) },
	{ "Uut", std::make_tuple(113,1.36,period::none) },
	{ "Uuq", std::make_tuple(114,1.43,period::none) },
	{ "Uup", std::make_tuple(115,1.62,period::none) },
	{ "Uuh", std::make_tuple(116,1.75,period::none) },
	{ "Uus", std::make_tuple(117,1.65,period::none) },
	{ "Uuo", std::make_tuple(118,1.57,period::none) }
  };

  inline std::size_t element_number(std::string const& key) {
	  return std::get<0>(pse.at(key));
  }

  inline float_type element_radius(std::string const& key) {
	  return std::get<1>(pse.at(key));
  }

  inline period element_period(std::string const& key) {
	  return std::get<2>(pse.at(key));
  }

  inline double AtomConnector::getThresholdForBeingNotConnected(std::string const& oneAtom, std::string const& otherAtom) {
    return 1.2 * (element_radius(oneAtom) + element_radius(otherAtom));
  }

  inline std::vector<std::pair<std::size_t, std::size_t>>
  bonds(std::vector<std::string> const& elem_vec,
        coords::Representation_3D const& cp_vec) {

    AtomConnector atomCreator(elem_vec, cp_vec);
    return atomCreator();

  }

  /*!
  \brief Flattens a std::vector of scon::c3<> vectors.
  \tparam T Arithmetic type. Enforced by type traits.
  \param vec std::vector that is to be flattened.
  \return std::vector.
  */
  template <typename T, template<typename> class CoordType, template<typename, typename...> class ContainerType, typename ... ContainerArgs>
  inline typename std::enable_if<std::is_arithmetic<T>::value, std::vector<T>>::type
  flatten_c3_vec(const ContainerType<CoordType<T>, ContainerArgs ...>& vec) {
    std::vector<T> result;
    result.reserve(3 * vec.size());
    for (auto& i : vec) {
      result.emplace_back(i.x());
      result.emplace_back(i.y());
      result.emplace_back(i.z());
    }
    return result;
  }

  namespace {
    struct MatrixConverterInterface {
      virtual coords::Representation_3D toRep3D() const = 0;
    };

    template<typename Mat>
    struct FlatMatrix : MatrixConverterInterface {
      FlatMatrix(Mat const& mat) : matrix(mat) {
        if (matrix.cols() != 1)
          throw std::runtime_error("Passed a Matrix as flattened Matrix which is not flat at all! Error in class FlatMatrix most likely thrown in MatToRep3D.");
      }
      Mat const& matrix;

      coords::Representation_3D toRep3D() const override {
        coords::Representation_3D result;
        for (auto i = 0u; i < matrix.rows(); i += 3) {
          result.emplace_back(matrix.operator()(i, 0u), matrix.operator()(i+1u, 0u), matrix.operator()(i+2u, 0u));
        }
        return result;
      }
    };
    template<typename Mat>
    struct NotFlatMatrix : MatrixConverterInterface {
      NotFlatMatrix(Mat const& mat) : matrix(mat) {
        if (matrix.cols() != 3)
          throw std::runtime_error("Passed a Matrix as to convert to a Rep3D object, but the matrix has not three rows! Error in class NotFlatMatrix  most likely thrown in MatToRep3D.");
      }
      Mat const& matrix;

      coords::Representation_3D toRep3D() const override {
        coords::Representation_3D result;
        for (auto i = 0u; i < matrix.rows(); ++i) {
          result.emplace_back(matrix(i, 0u), matrix(i, 1u), matrix(i, 2u));
        }
        return result;
      }
    };
  }

  template<typename Mat>
  void printMat(Mat const& mat) {
    for (auto i = 0u; i < mat.rows(); ++i) {
      for (auto j = 0u; j < mat.cols(); ++j) {
        std::cout << mat(i, j) << " ";
      }
      std::cout << "\n";
    }
  }

  template<typename Mat>
  coords::Representation_3D matToRep3D(Mat const& mat) {
    std::unique_ptr<MatrixConverterInterface> converter;
    if (mat.cols() == 1) {
      converter = std::make_unique<FlatMatrix<Mat>>(mat);
    }
    else {
      converter = std::make_unique<NotFlatMatrix<Mat>>(mat);
    }
    return converter->toRep3D();
  }

  template<typename Mat>
  Mat atomsNorm(Mat const& norm) {
	  Mat mat(norm.rows(), 1);
	  for (auto i = 0u; i < norm.rows(); ++i) {
		  mat(i, 0) = norm.row(i).norm();
	  }
	  return mat;
  }

}

/*! @} End of ic_util group*/

#endif // cast_ic_util_h_guard
