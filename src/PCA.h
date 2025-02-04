﻿#pragma once
#include "matop.h"
#include "histogram.h"
#include "alignment.h"
namespace pca
{
	class PrincipalComponentRepresentation
	{
	public:

		using float_type = coords::float_type;

		PrincipalComponentRepresentation(std::unique_ptr<coords::input::format>& ci, coords::Coordinates& coords);

		PrincipalComponentRepresentation(std::string const& filenameOfPCAModesFile);


		void writePCAModesFile(std::string const& filename = "pca_modes.dat");
		void writeHistogrammedProbabilityDensity(std::string const& filename = "pca_histogrammed.dat");
		void writeStocksDelta(std::string const& filename = "pca_stocksdelta.dat");
		void readEigenvectors(std::string const& filename = "pca_modes.dat");
		void readModes(std::string const& filename = "pca_modes.dat");
		void generateCoordinateMatrix(std::unique_ptr<coords::input::format>& ci, coords::Coordinates& coords);
		void generatePCAEigenvectorsFromCoordinates();
		void generatePCAModesFromPCAEigenvectorsAndCoordinates();
	protected:
		Matrix_Class modes;
		Matrix_Class eigenvectors;
		Matrix_Class eigenvalues;
		Matrix_Class coordinatesMatrix;
	};

	class ProcessedPrincipalComponentRepresentation
		: public PrincipalComponentRepresentation
	{
	public:
		ProcessedPrincipalComponentRepresentation(std::string const& filenameOfPCAModesFile);
		void readAdditionalInformation(std::string const& filename = "pca_modes.dat");
		void determineStructures(std::unique_ptr<coords::input::format>& ci, ::coords::Coordinates& coords);
		void restoreCoordinatesMatrix();
		void writeDeterminedStructures(::coords::Coordinates const& coord_in, std::string const& filenameExtension = "_pca_selection");
	private:
		std::vector<size_t> structuresToBeWrittenToFile;
		std::string additionalInformation;
		std::vector<coords::PES_Point> foundStructures;
	};
}