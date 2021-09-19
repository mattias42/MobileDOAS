#include "StdAfx.h"

#undef max
#undef min
#include <filesystem>
#include <sstream>
#include "WavelengthCalibrationController.h"
#include <SpectralEvaluation/Spectra/Spectrum.h>
#include <SpectralEvaluation/File/File.h>
#include <SpectralEvaluation/File/STDFile.h>
#include <SpectralEvaluation/Calibration/InstrumentCalibration.h>
#include <SpectralEvaluation/Calibration/WavelengthCalibration.h>
#include <SpectralEvaluation/Calibration/InstrumentLineShapeEstimation.h>
#include <SpectralEvaluation/Calibration/FraunhoferSpectrumGeneration.h>
#include <SpectralEvaluation/Interpolation.h>
#include <SpectralEvaluation/VectorUtils.h>

// TODO: It should be possible to remove this header...
#include <SpectralEvaluation/Calibration/WavelengthCalibrationByRansac.h>

// From InstrumentLineShapeCalibrationController...
std::vector<std::pair<std::string, std::string>> GetFunctionDescription(const novac::ParametricInstrumentLineShape* lineShapeFunction);

WavelengthCalibrationController::WavelengthCalibrationController()
    : m_calibrationDebug(0U),
    m_instrumentLineShapeFitOption(InstrumentLineShapeFitOption::None)
{
}

WavelengthCalibrationController::~WavelengthCalibrationController()
{
    this->m_initialCalibration.release();
    this->m_resultingCalibration.release();
}

/// <summary>
/// Reads and parses m_initialLineShapeFile and saves the result to the provided settings
/// </summary>
void ReadInstrumentLineShape(const std::string& initialLineShapeFile, novac::WavelengthCalibrationSettings& settings)
{
    novac::CCrossSectionData measuredInstrumentLineShape;
    if (!novac::ReadCrossSectionFile(initialLineShapeFile, measuredInstrumentLineShape))
    {
        throw std::invalid_argument("Cannot read the provided instrument lineshape file");
    }
    settings.initialInstrumentLineShape = measuredInstrumentLineShape;
    settings.estimateInstrumentLineShape = novac::InstrumentLineshapeEstimationOption::None;
}

/// <summary>
/// Guesses for an instrment line shape from the measured spectrum. Useful if no measured instrument line shape exists
/// </summary>
void CreateGuessForInstrumentLineShape(const std::string& solarSpectrumFile, const novac::CSpectrum& measuredSpectrum, novac::WavelengthCalibrationSettings& settings)
{
    // The user has not supplied an instrument-line-shape, create a guess for one.
    std::vector<std::pair<std::string, double>> noCrossSections;
    novac::FraunhoferSpectrumGeneration fraunhoferSpectrumGen{ solarSpectrumFile, noCrossSections };

    novac::InstrumentLineShapeEstimationFromKeypointDistance ilsEstimation{ settings.initialPixelToWavelengthMapping };

    double resultInstrumentFwhm;
    ilsEstimation.EstimateInstrumentLineShape(fraunhoferSpectrumGen, measuredSpectrum, settings.initialInstrumentLineShape, resultInstrumentFwhm);

    // no need to estimate further (or is it?)
    settings.estimateInstrumentLineShape = novac::InstrumentLineshapeEstimationOption::None;
}

void WavelengthCalibrationController::RunCalibration()
{
    novac::WavelengthCalibrationSettings settings;
    settings.highResSolarAtlas = this->m_solarSpectrumFile;

    novac::CSpectrum measuredSpectrum;
    if (!novac::ReadSpectrum(this->m_inputSpectrumFile, measuredSpectrum))
    {
        throw std::invalid_argument("Cannot read the provided input spectrum file");
    }

    // subtract the dark-spectrum (if this has been provided)
    novac::CSpectrum darkSpectrum;
    if (novac::ReadSpectrum(this->m_darkSpectrumFile, darkSpectrum))
    {
        measuredSpectrum.Sub(darkSpectrum);
    }

    if (novac::GetFileExtension(this->m_initialCalibrationFile).compare(".std") == 0)
    {
        this->m_initialCalibration = std::make_unique<novac::InstrumentCalibration>();
        if (!novac::ReadInstrumentCalibration(this->m_initialCalibrationFile, *m_initialCalibration))
        {
            throw std::invalid_argument("Failed to read the instrument calibration file");
        }

        settings.initialPixelToWavelengthMapping = this->m_initialCalibration->pixelToWavelengthMapping;
        settings.initialInstrumentLineShape.m_crossSection = this->m_initialCalibration->instrumentLineShape;
        settings.initialInstrumentLineShape.m_waveLength = this->m_initialCalibration->instrumentLineShapeGrid;
    }
    else
    {
        settings.initialPixelToWavelengthMapping = novac::GetPixelToWavelengthMappingFromFile(this->m_initialCalibrationFile);

        if (this->m_initialLineShapeFile.size() > 0)
        {
            ReadInstrumentLineShape(m_initialLineShapeFile, settings);
        }
        else
        {
            CreateGuessForInstrumentLineShape(this->m_solarSpectrumFile, measuredSpectrum, settings);
        }
    }

    // Normalize the initial instrument line shape
    Normalize(this->m_initialCalibration->instrumentLineShape);

    if (this->m_instrumentLineShapeFitOption == WavelengthCalibrationController::InstrumentLineShapeFitOption::SuperGaussian)
    {
        if (this->m_instrumentLineShapeFitRegion.first > this->m_instrumentLineShapeFitRegion.second)
        {
            std::stringstream msg;
            msg << "Invalid region for fitting the instrument line shape ";
            msg << "(" << this->m_instrumentLineShapeFitRegion.first << ", " << this->m_instrumentLineShapeFitRegion.second << ") [nm]. ";
            msg << "From must be smaller than To";
            throw std::invalid_argument(msg.str());
        }
        if (this->m_instrumentLineShapeFitRegion.first < settings.initialPixelToWavelengthMapping.front() ||
            this->m_instrumentLineShapeFitRegion.second > settings.initialPixelToWavelengthMapping.back())
        {
            std::stringstream msg;
            msg << "Invalid region for fitting the instrument line shape ";
            msg << "(" << this->m_instrumentLineShapeFitRegion.first << ", " << this->m_instrumentLineShapeFitRegion.second << ") [nm]. ";
            msg << "Region does not overlap initial pixel to wavelength calibration: ";
            msg << "(" << settings.initialPixelToWavelengthMapping.front() << ", " << settings.initialPixelToWavelengthMapping.back() << ") [nm]. ";
            throw std::invalid_argument(msg.str());
        }

        settings.estimateInstrumentLineShape = novac::InstrumentLineshapeEstimationOption::SuperGaussian;
        settings.estimateInstrumentLineShapeWavelengthRegion.first = this->m_instrumentLineShapeFitRegion.first;
        settings.estimateInstrumentLineShapeWavelengthRegion.second = this->m_instrumentLineShapeFitRegion.second;
    }
    else
    {
        settings.estimateInstrumentLineShape = novac::InstrumentLineshapeEstimationOption::None;
    }

    // Clear the previous result
    this->m_resultingCalibration = std::make_unique<novac::InstrumentCalibration>();


    // So far no cross sections provided...

    novac::WavelengthCalibrationSetup setup{ settings };
    auto result = setup.DoWavelengthCalibration(measuredSpectrum);

    // Copy out the result
    this->m_resultingCalibration->pixelToWavelengthMapping = result.pixelToWavelengthMapping;
    this->m_resultingCalibration->pixelToWavelengthPolynomial = result.pixelToWavelengthMappingCoefficients;

    if (result.estimatedInstrumentLineShape.GetSize() > 0)
    {
        this->m_resultingCalibration->instrumentLineShape = result.estimatedInstrumentLineShape.m_crossSection;
        this->m_resultingCalibration->instrumentLineShapeGrid = result.estimatedInstrumentLineShape.m_waveLength;
        this->m_resultingCalibration->instrumentLineShapeCenter = 0.5 * (this->m_instrumentLineShapeEstimationWavelengthRange.first + this->m_instrumentLineShapeEstimationWavelengthRange.second);

        this->m_instrumentLineShapeEstimationWavelengthRange = settings.estimateInstrumentLineShapeWavelengthRegion;
    }

    if (result.estimatedInstrumentLineShapeParameters != nullptr)
    {
        this->m_instrumentLineShapeParameterDescriptions = GetFunctionDescription(&(*result.estimatedInstrumentLineShapeParameters));

        novac::SuperGaussianLineShape* superGaussian = dynamic_cast<novac::SuperGaussianLineShape*>(result.estimatedInstrumentLineShapeParameters.get());
        if (superGaussian != nullptr)
        {
            this->m_resultingCalibration->instrumentLineShapeParameter = new novac::SuperGaussianLineShape(*superGaussian);
        }
        else
        {
            novac::GaussianLineShape* regularGaussian = dynamic_cast<novac::GaussianLineShape*>(result.estimatedInstrumentLineShapeParameters.get());
            if (regularGaussian != nullptr)
            {
                this->m_resultingCalibration->instrumentLineShapeParameter = new novac::GaussianLineShape(*regularGaussian);
            }
        }

    }

    // Also copy out some debug information, which makes it possible for the user to inspect the calibration
    {
        const auto& calibrationDebug = setup.GetLastCalibrationSetup();
        this->m_calibrationDebug = WavelengthCalibrationDebugState(calibrationDebug.allCorrespondences.size());
        this->m_calibrationDebug.initialPixelToWavelengthMapping = settings.initialPixelToWavelengthMapping;

        for (size_t correspondenceIdx = 0; correspondenceIdx < calibrationDebug.allCorrespondences.size(); ++correspondenceIdx)
        {
            const auto& corr = calibrationDebug.allCorrespondences[correspondenceIdx]; // easier syntax.

            if (calibrationDebug.correspondenceIsInlier[correspondenceIdx])
            {
                this->m_calibrationDebug.inlierCorrespondencePixels.push_back(corr.measuredValue);
                this->m_calibrationDebug.inlierCorrespondenceWavelengths.push_back(corr.theoreticalValue);
                this->m_calibrationDebug.inlierCorrespondenceMeasuredIntensity.push_back(calibrationDebug.measuredKeypoints[corr.measuredIdx].intensity);
                this->m_calibrationDebug.inlierCorrespondenceFraunhoferIntensity.push_back(calibrationDebug.fraunhoferKeypoints[corr.theoreticalIdx].intensity);

                this->m_calibrationDebug.measuredSpectrumInlierKeypointPixels.push_back(calibrationDebug.measuredKeypoints[corr.measuredIdx].pixel);
                this->m_calibrationDebug.measuredSpectrumInlierKeypointIntensities.push_back(calibrationDebug.measuredKeypoints[corr.measuredIdx].intensity);

                this->m_calibrationDebug.fraunhoferSpectrumInlierKeypointWavelength.push_back(calibrationDebug.fraunhoferKeypoints[corr.theoreticalIdx].wavelength);
                this->m_calibrationDebug.fraunhoferSpectrumInlierKeypointIntensities.push_back(calibrationDebug.fraunhoferKeypoints[corr.theoreticalIdx].intensity);
            }
            else
            {
                this->m_calibrationDebug.outlierCorrespondencePixels.push_back(corr.measuredValue);
                this->m_calibrationDebug.outlierCorrespondenceWavelengths.push_back(corr.theoreticalValue);
            }
        }

        this->m_calibrationDebug.measuredSpectrum = std::vector<double>(calibrationDebug.measuredSpectrum->m_data, calibrationDebug.measuredSpectrum->m_data + calibrationDebug.measuredSpectrum->m_length);
        this->m_calibrationDebug.fraunhoferSpectrum = std::vector<double>(calibrationDebug.fraunhoferSpectrum->m_data, calibrationDebug.fraunhoferSpectrum->m_data + calibrationDebug.fraunhoferSpectrum->m_length);

        for (const auto& pt : calibrationDebug.measuredKeypoints)
        {
            this->m_calibrationDebug.measuredSpectrumKeypointPixels.push_back(pt.pixel);
            this->m_calibrationDebug.measuredSpectrumKeypointIntensities.push_back(pt.intensity);
        }

        for (const auto& pt : calibrationDebug.fraunhoferKeypoints)
        {
            this->m_calibrationDebug.fraunhoferSpectrumKeypointWavelength.push_back(pt.wavelength);
            this->m_calibrationDebug.fraunhoferSpectrumKeypointIntensities.push_back(pt.intensity);
        }
    }
}


std::pair<std::string, std::string> FormatProperty(const char* name, double value);

void WavelengthCalibrationController::SaveResultAsStd(const std::string& filename)
{
    if (this->m_resultingCalibration->instrumentLineShape.size() > 0)
    {
        // We have fitted both an instrument line shape and a pixel-to-wavelength mapping.
        //  I.e. m_resultingCalibration contains a full calibration to be saved.
        if (!novac::SaveInstrumentCalibration(filename, *m_resultingCalibration))
        {
            throw std::invalid_argument("Failed to save the resulting instrument calibration file");
        }
        return;
    }

    // Create an instrument calibration, taking the instrument line shape from the initial setup 
    //  and the pixel-to-wavelength mapping from the calibration result
    novac::InstrumentCalibration mixedCalibration;
    mixedCalibration.pixelToWavelengthMapping = this->m_resultingCalibration->pixelToWavelengthMapping;
    mixedCalibration.pixelToWavelengthPolynomial = this->m_resultingCalibration->pixelToWavelengthPolynomial;

    mixedCalibration.instrumentLineShape = this->m_initialCalibration->instrumentLineShape;
    mixedCalibration.instrumentLineShapeGrid = this->m_initialCalibration->instrumentLineShapeGrid;
    mixedCalibration.instrumentLineShapeCenter = this->m_initialCalibration->instrumentLineShapeCenter;

    if (this->m_initialCalibration->instrumentLineShapeParameter != nullptr)
    {
        mixedCalibration.instrumentLineShapeParameter = this->m_initialCalibration->instrumentLineShapeParameter->Clone();
    }

    if (!novac::SaveInstrumentCalibration(filename, mixedCalibration))
    {
        throw std::invalid_argument("Failed to save the resulting instrument calibration file");
    }
}

void WavelengthCalibrationController::SaveResultAsClb(const std::string& filename)
{
    novac::SaveDataToFile(filename, m_resultingCalibration->pixelToWavelengthMapping);
}

void WavelengthCalibrationController::SaveResultAsSlf(const std::string& filename)
{
    novac::CCrossSectionData instrumentLineShape;
    instrumentLineShape.m_crossSection = this->m_resultingCalibration->instrumentLineShape;
    instrumentLineShape.m_waveLength = this->m_resultingCalibration->instrumentLineShapeGrid;

    novac::SaveCrossSectionFile(filename, instrumentLineShape);
}

