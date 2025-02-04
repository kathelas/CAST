#pragma once

#include <vector>
#include <ostream>
#include <iomanip>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <map>
#include <limits>


namespace histo
{
	template<typename T>
	/**
	 * This class enables histogramming of multidimensional data in multidimensional histograms.
	 * The regular user should forget about all private member(-functions) and use the class as such:
	 *
	 * 1. Construct:
	 * Either construct with a desired number of bins (adjusting width of bin automatically to suit data)
	 * or with a defined width of a bin (adjusting number of bins automatically to suit data)
	 *
	 * 2. Add Values  via add_value
	 * 3. call distribute() to perform histogramming
	 * 4. write data and auxilary data.
	 *
	 * By Dustin Kaiser 2016
	 */
	class DimensionalHistogram //Single dimension multiple histograms
	{
		// Users should not care about the private members of this class.
		// If you need multidimensional histogramming, just use the public member functions,
		// and voila
	private:

		// width and push value
		std::vector<T> w, p;
		// upper and lower limits
		std::vector<T> m_max, m_min;
		// sum and mean
		std::vector<T> m_s, m_m;
		// values
		std::vector< std::vector<T> >  m_values;
		// boxes
		std::vector< std::size_t > m_boxes;
		// number of histograms
		std::size_t const m_dimensions;
		// total number of values
		std::size_t m_valuecount, m_boxcount;

		//FUNCTIONS

		// Maximum of all values in container for multiple histograms
		std::vector<T> const& maximum() const { return m_max; }
		T const& maximum(unsigned int i) const { return m_max[i]; }

		// Minimum of all values in container for multiple histograms
		std::vector<T> const& minimum() const { return m_min; }
		T const& minimum(unsigned int i) const { return m_min[i]; }

		// Range of all values in container for multiple histograms
		std::vector<T> width() const
		{
			std::vector<T> temp(m_min.size(), 0);
			for (unsigned int i = 0u; i < m_min.size(); i++)
			{
				temp[i] = (m_max[i] - m_min[i]);
			}
			return temp;
		}
		T width(unsigned int i) const { return (m_max[i] - m_min[i]); }

		// Sum of all values in single histogram
		std::vector<T> sum() const { return m_s; }
		T sum(unsigned int i) const { return m_s[i]; }

		// Mean of all values in single histogram
		std::vector<T> mean() const { return m_m; }
		T mean(unsigned int i) const { return m_m[i]; }

		// Center of histogram
		std::vector<T> center() const
		{
			std::vector<T> temp(m_dimensions, 0.);
			for (unsigned int i = 0u; i < m_dimensions; i++)
			{
				temp[i] = minimum(i) + (maximum() - minimum());
			}
			return temp;
		}
		T center(unsigned int dimension) const
		{
			return this->center()[dimension];
		}

		// Value associated with single bin (value at center of bin)
		std::vector<T> centerOfSingleBin(std::vector<size_t> const& bins)
		{
			std::vector<T> binsOut(bins.size());
			if (binsOut.size() == m_dimensions)
			{
				for (unsigned int i = 0u; i < binsOut.size(); i++)
				{
					binsOut[i] = this->minimum()[i] + bins[i] * w[i] + w[i] * 0.5;
				}
				return binsOut;
			}
			else
			{
				throw("fatal error in histogramming! -> centerOfSingleBin(std::vector<size_t> bins)\n");
				return std::vector<T>();
			}
		}

		// Sets Minimum and Maximum according to Values inside Histogramm
		// Called from within distribute()
		void setMinMax()
		{
			for (unsigned int i = 0u; i < m_values.size(); i++)
			{
				for (unsigned int j = 0u; j < m_values[i].size(); j++)
				{
					if (m_values[i][j] > m_max[j]) m_max[j] = m_values[i][j];
					if (m_values[i][j] < m_min[j]) m_min[j] = m_values[i][j];
				}
			}
		}

		// Access to the serially stored histogram bin values
		// Justification: Since this class should work for any number of dimensions,
		// bin data has to be stored serially. This is an N-Dimension to 1D-Mapping.
		inline std::size_t toIterator(std::vector<std::size_t> const& in)
		{
			if (in.size() == this->dimensions())
			{
				size_t accessIterator = 0;
				for (unsigned int i = 0u; i < in.size(); i++)
				{
					accessIterator += in[i] * (size_t)pow(m_boxcount, m_dimensions - i - 1);
				}
				return accessIterator;
			}
			else
			{
				std::cout << "ERROR IN DIMENSIONS OF HISTOGRAM!\n" << std::flush;
				return 0u;
			}
		}

		// Covert serially stored histogram bin values to dimensional vector
		// Justification: Since this class should work for any number of dimensions,
		// bin data has to be stored serially. This is an N-Dimension to 1D-Mapping.
		inline std::vector<std::size_t> fromIterator(std::size_t in)
		{
			//in std::vec dim 0 is first bin, counting starts at 0.
			std::vector<std::size_t> out;
			out.resize(this->dimensions());

			if (in == 0) return std::vector<std::size_t>(this->dimensions(), 0u);

			size_t tempIn = in;
			for (int i = (int)out.size() - 1; i > -1; i--)
			{
				size_t tempCalc = tempIn % (int)this->numberOfBinsPerDimension();
				out[i] = tempCalc;
				tempIn = tempIn - tempCalc;
				if (tempIn < this->numberOfBinsPerDimension())
				{
					tempIn = this->numberOfBinsPerDimension();
				}
				else
				{
					tempIn = size_t(float(tempIn) / float(this->numberOfBinsPerDimension()));
				}
			}
			return out;
		}

		inline std::size_t dimensions() const { return m_dimensions; }
		inline std::size_t numberOfBinsPerDimension() const { return m_boxcount; }

		// Returns number of elements in bin associated with (dimensionally stored) "in" 
		inline std::size_t element(std::vector<size_t> const& in) const
		{
			if (in.size() != m_dimensions || m_boxcount <= 0u)
			{
				std::cout << "fatal error in histogramming, desired elements have wrong dimension. Talk to your admin.";
				return 0u;
			}
			else
			{
				return m_boxes[toIterator(in)];
			}
		}

		// Returns number of elements in bin associated with (1D-serial) "accessIterator" 
		// This should actually never be needed to be called exept for debug purposes.
		inline std::size_t element(std::size_t accessIterator)
		{
			return m_boxes[accessIterator];
		}


	public:

		// Construct a set of multiple histograms (number of histograms = "number")
		// where each bin has a width of "width". The number of bins is generated dynamicly
		// from this width of one bin and the total range of values later on.
		// "push" is appareantly a value by which the entire histogram is shifted.
		DimensionalHistogram(std::size_t const dimensions, T const width, T const push = T()) :
			w(dimensions, width), p(dimensions, push), m_max(dimensions, 0.), m_min(dimensions, 0.),
			m_s(dimensions, 0.), m_m(dimensions, 0u), m_values(0),
			m_boxes(dimensions, 0u), m_dimensions(dimensions), m_valuecount(0U), m_boxcount(0U) {};

		// Construct a set of multiple histograms (number of histograms = "number")
		// with a specified number of bins.
		// "push" is appareantly a value by which the entire histogram is shifted.
		DimensionalHistogram(std::size_t const dimensions, std::size_t const desiredNumberOfBoxes_, T const push = T()) :
			w(dimensions, 0.), p(dimensions, push), m_max(dimensions, 0.), m_min(dimensions, 0.),
			m_s(dimensions, 0.), m_m(dimensions, 0.), m_values(0),
			m_boxes(dimensions, 0u), m_dimensions(dimensions), m_valuecount(0U), m_boxcount(desiredNumberOfBoxes_) {};

		// Adds a value "newValue" to the histogram nr. "desiredHistogramInSet"
		inline void add_value(std::vector<T> const newValues)
		{
			// CHeck if value has valid dimensions
			if (newValues.size() <= m_dimensions)
			{ //CHeck if value = firstValue
				if (m_valuecount < 1U)
				{
					for (unsigned int i = 0u; i < newValues.size(); i++)
					{//set min max
						m_max[i] = newValues[i];
						m_min[i] = newValues[i];
					}
				}
				else
				{
					//set min max
					for (unsigned int i = 0u; i < newValues.size(); i++)
					{
						m_max[i] = std::max(newValues[i], m_max[i]);
						m_min[i] = std::min(newValues[i], m_min[i]);
					}
				}
				//set sum and mean
				for (unsigned int i = 0u; i < newValues.size(); i++)
				{
					T tempsum = sqrt(newValues[i] * newValues[i]);
					m_s[i] += tempsum;
					m_m[i] = m_s[i] / T(m_values.size());
				}
				//save values
				m_values.push_back(newValues);
				++m_valuecount;
			}
			else
			{
				std::cout << "Fatal ERROR in multidimensional histogramming: add value\n" << std::flush;
			}
		}

		// this seems like it actually counts the values and thereby creates the histogram
		void distribute(void)
		{
			using std::ceil;
			using std::floor;
			for (unsigned int j = 0u; j < m_dimensions; j++)
			{
				m_max[j] += p[j];
			}

			// Get number of distribution boxes. 
			// Number of distribution boxes is equal to ceil()
			// of the total range of values devided by
			// the constructor specified width of one bin
			if (w[0] > T(0.))
			{
				T highestWidth = 0., absMax = 0., absMin = 0.;
				for (unsigned int j = 0u; j < m_dimensions; j++)
				{
					highestWidth = std::max(highestWidth, this->width()[j]);
					if (absMax < this->maximum(j)) absMax = this->maximum(j);
					if (absMin > this->minimum(j)) absMin = this->minimum(j);
				}
				m_boxcount = static_cast<std::size_t>(std::ceil((absMax - absMin) / highestWidth));
			}
			else if (m_boxcount == 0)
			{
				std::cout << "Severe Error in histogram.h. Stopping.";
				throw;
			}
			else if (m_boxcount > 0)
			{
				for (unsigned int i = 0u; i < m_dimensions; i++)
				{
					w[i] = (m_max[i] - m_min[i]) / T(m_boxcount);
				}
			}

			// We need m_boxcount^dimension discrete histogram bins, so we resize to this
			//Adjust number of histogram bins
			m_boxes.resize((size_t)pow(m_boxcount, m_dimensions));
			//Fills the std::vector with "m_boxcount^m_dimension" size_ts with value 0u.
			m_boxes.assign((size_t)pow(m_boxcount, m_dimensions), 0u);

			// Now, the actual processing of values takes place.
			std::size_t const numberOfValues = m_values.size();
			for (std::size_t i = 0; i < (size_t)numberOfValues; ++i) // Iterate over the number of values. i is current value.
			{
				std::vector<std::size_t> index(m_dimensions, size_t());
				for (std::size_t h(0U); h < m_dimensions; ++h)
				{
					auto r = std::floor((m_values[i][h] - minimum()[h]) / w[h] + p[h]);
					if (r < 0) index[h] = 0u;
					index[h] = static_cast<std::size_t>(r);
					if (index[h] >= this->numberOfBinsPerDimension())
					{
						index[h] = this->numberOfBinsPerDimension() - 1u;
					}
				}
				++m_boxes[this->toIterator(index)];
			}
		}

		// Write histogrammed data in a format suitable for gnuplotting
		void write(std::string filename)
		{
			std::ofstream stream(filename, std::ios::out);
			stream << std::right << std::setw(13);
			size_t keeperForBlanklines = 0;
			for (unsigned int i = 0u; i < m_boxes.size(); i++) //Iterates over dimensions
			{

				std::vector<T> bins = centerOfSingleBin(fromIterator(i)); // Center of all bins collected in vector

				if (this->dimensions() > 1u)
				{
					if (fromIterator(i)[bins.size() - 2] != keeperForBlanklines) // Check if we have reached second to last dimension (then we insert space)
					{
						keeperForBlanklines = fromIterator(i)[bins.size() - 2];
						stream << "\n";
					}
				}

				for (unsigned int j = 0u; j < bins.size(); j++)
				{
					stream << std::right << std::setw(13) << bins[j] << " "; // Write center of bins
				}
				stream << std::right << std::setw(13) << this->element((size_t)i) << "\n"; // Write value of bin
			}
		}

		// Write histogrammed data in a format suitable for gnuplotting
		// This data will be normated to be a probability density
		void writeProbabilityDensity(std::string filename)
		{
			std::ofstream stream(filename, std::ios::out);
			stream << std::right << std::setw(13);
			size_t keeperForBlanklines = 0;
			for (unsigned int i = 0u; i < m_boxes.size(); i++)
			{

				std::vector<T> bins = centerOfSingleBin(fromIterator(i));
				if (this->dimensions() > 1u)
				{
					if (fromIterator(i)[bins.size() - 2] != keeperForBlanklines)
					{
						keeperForBlanklines = fromIterator(i)[bins.size() - 2];
						stream << "\n";
					}
				}

				for (unsigned int j = 0u; j < bins.size(); j++)
				{
					stream << std::right << std::setw(13) << bins[j] << " ";
				}
				stream << std::right << std::setw(13) << (T)((T)this->element((size_t)i) / (T)this->m_valuecount) << "\n";
			}

		}

		//Write sums and means (->auxilary data) to file.
		void writeAuxilaryData(std::string filename)
		{
			std::ofstream stream(filename, std::ios::out);
			stream << "Number of Dimensions: " << this->dimensions() << "\n";
			for (unsigned int i = 0; i < this->dimensions(); i++)
			{
				stream << std::right << "Mean of dimension " << i + 1 << ": " << this->mean(i) << "\n";
			}
			stream << "\n\n";

			for (unsigned int i = 0; i < this->dimensions(); i++)
			{
				stream << std::right << "Sum of dimension " << i + 1 << ": " << this->sum(i) << "\n";
			}
			stream << "\n\n";
		}
	};
 
}

