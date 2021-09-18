#pragma once

#include <string>
#include <vector>

namespace novac
{
class CCrossSectionData;
class ParametricInstrumentLineShape;
}

class WavelengthCalibrationController
{
public:
    WavelengthCalibrationController();
    ~WavelengthCalibrationController();

    enum class InstrumentLineShapeFitOption
    {
        None = 0,
        SuperGaussian = 1
    };

    /// <summary>
    /// The full path to the spectrum to calibrate
    /// </summary>
    std::string m_inputSpectrumFile;

    /// <summary>
    /// The full path to the dark spectrum of the spectrum to calibrate
    /// </summary>
    std::string m_darkSpectrumFile;

    /// <summary>
    /// The full path to the high resolved solar spectrum
    /// </summary>
    std::string m_solarSpectrumFile;

    /// <summary>
    /// The full path to a file which contains an initial calibration
    /// This can be either a full calibration file, as saved from another novac program,
    /// or just the pixel-to-wavelength mapping file.
    /// </summary>
    std::string m_initialCalibrationFile;

    /// <summary>
    /// The full path to a file which contains an initial measured line shape.
    /// This may be left out if m_initialCalibrationFile does contain an instrument line shape.
    /// </summary>
    std::string m_initialLineShapeFile;

    /// <summary>
    /// The option for if an instrument line shape should be fitted as well during
    /// the retrieval of the pixel-to-wavelength calibration.
    /// </summary>
    InstrumentLineShapeFitOption m_instrumentLineShapeFitOption;

    /// <summary>
    /// The wavelength region in which the instrument line shape should be fitted (in nm).
    /// </summary>
    std::pair<double, double> m_instrumentLineShapeFitRegion;

    /// <summary>
    /// Output: the final estimate for the pixel to wavelength mapping.
    /// </summary>
    std::vector<double> m_resultingPixelToWavelengthMapping;

    /// <summary>
    /// Output: the coefficients of the pixel-to-wavelength mapping polynomial
    /// </summary>
    std::vector<double> m_resultingPixelToWavelengthMappingCoefficients;

    /// <summary>
    /// If we have read the instrument line shape from file then the result is saved here
    /// </summary>
    novac::CCrossSectionData* m_measuredInstrumentLineShape = nullptr;

    /// <summary>
    /// If we have fitted an instrument line shape to the measured spectrum then the sampled result is saved here
    /// </summary>
    novac::CCrossSectionData* m_resultingInstrumentLineShape = nullptr;

    /// <summary>
    /// If we have fitted an instrument line shape to the measured spectrum then the estimated parameters are saved here.
    /// </summary>
    novac::ParametricInstrumentLineShape* m_instrumentLineShapeParameter = nullptr;

    /// <summary>
    /// User friendly description of the fitted parameters for the instrument line shape function.
    /// </summary>
    std::vector<std::pair<std::string, std::string>> m_instrumentLineShapeParameterDescriptions;

    /// <summary>
    /// The range of pixels over which the m_resultingInstrumentLineShape was estimatd (only set if it was estimated).
    /// </summary>
    std::pair<size_t, size_t> m_instrumentLineShapeEstimationPixelRange;

    /// <summary>
    /// If the calibration fails, for some reason, then this message should be set to indicate why.
    /// </summary>
    std::string m_errorMessage;

    struct WavelengthCalibrationDebugState
    {
        WavelengthCalibrationDebugState(size_t estimatedSize)
        {
            inlierCorrespondencePixels.reserve(estimatedSize);
            inlierCorrespondenceWavelengths.reserve(estimatedSize);
            outlierCorrespondencePixels.reserve(estimatedSize);
            outlierCorrespondenceWavelengths.reserve(estimatedSize);
        }

        std::vector<double> initialPixelToWavelengthMapping;

        std::vector<double> inlierCorrespondencePixels;
        std::vector<double> inlierCorrespondenceWavelengths;
        std::vector<double> inlierCorrespondenceMeasuredIntensity; //< the intensity of the measured spectrum
        std::vector<double> inlierCorrespondenceFraunhoferIntensity; //< the intensity of the Fraunhofer spectrum

        std::vector<double> outlierCorrespondencePixels;
        std::vector<double> outlierCorrespondenceWavelengths;

        std::vector<double> measuredSpectrum;

        // All the keypoints from the measured spectrum
        std::vector<double> measuredSpectrumKeypointPixels;
        std::vector<double> measuredSpectrumKeypointIntensities;

        // The inlier keypoints from the measured spectrum
        std::vector<double> measuredSpectrumInlierKeypointPixels;
        std::vector<double> measuredSpectrumInlierKeypointIntensities;

        std::vector<double> fraunhoferSpectrum;

        // All the keypoints from the Fraunhofer spectrum
        std::vector<double> fraunhoferSpectrumKeypointWavelength;
        std::vector<double> fraunhoferSpectrumKeypointIntensities;

        // The inlier keypoints from the Fraunhofer spectrum
        std::vector<double> fraunhoferSpectrumInlierKeypointWavelength;
        std::vector<double> fraunhoferSpectrumInlierKeypointIntensities;
    };

    WavelengthCalibrationDebugState m_calibrationDebug;

    /// <summary>
    /// Performs the actual wavelength calibration
    /// </summary>
    void RunCalibration();

    /// <summary>
    /// Saves the resulting pixel-to-wavelength and instrument-line-shape information as a .std file.
    /// </summary>
    void SaveResultAsStd(const std::string& filename);

    /// <summary>
    /// Saves the resulting pixel-to-wavelength mapping information as a .clb file.
    /// </summary>
    void SaveResultAsClb(const std::string& filename);

    /// <summary>
    /// Saves the resulting instrument line shape information as a .slf file.
    /// </summary>
    void SaveResultAsSlf(const std::string& filename);

private:
    void DeleteSavedInstrumentLineShape();
};