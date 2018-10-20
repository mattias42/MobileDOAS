#pragma once

#include <vector>
#include <string>

/** A class for storing spectrum data in a convenient and flexible way. */
class CSpectrum final
{
public:
	/** Allocates an empty spectrum */
	CSpectrum();

	/** Allocates a spectrum with the provided size */
	CSpectrum(int size);

	~CSpectrum();

	// -------- Copying CSpectrum objects --------
	CSpectrum(const CSpectrum& other);
	CSpectrum& operator=(CSpectrum other);
	friend void swap(CSpectrum& first, CSpectrum& second);

	std::vector<double> data; // The spectral data itself

	long    scans;			// The number of co-added spectra (exposures)
	long    exposureTime;	// The exposure-time in milliseconds
	double  lat;			// Latitude, in decimal degrees
	double  lon;			// Longitude, in decimal degrees
	double  altitude;		// Altitude, in meters above sea-level
	int     date[3];		// The date the spectrum acquisition started. date[0] is the year, date[1] is the month and date[2] is the day
	int     startTime[3];	// The local time-of-day when the spectrum acquisition started. [0] is hour, [1] is minute, [2] is seconds.
	int     stopTime[3];	// The local time-of-day when the spectrum acquisition stopped. [0] is hour, [1] is minute, [2] is seconds.
	bool    isDark;			// Set to true if this spectrum is dark.

	std::string spectrometerSerial; // Serial number of the spectrometer
	std::string spectrometerModel;  // Model of the spectrometer
	std::string name;               // name of the spectrum

	// properties
	int     Length() const { return (int)data.size(); }

	// statistics
	void    GetMinMax(double& minValue, double& maxValue) const;
	double  GetMax() const;
	double  GetAverage() const;
	double  GetAverage(int low, int high) const; // gets the average value between the indexes 'low' and 'high' (inclusive)
	void    GetStatistics(double& average, double& variance) const; // gets the average and variance of the entire spectrum
	double  GetSum() const;
	double  GetSum(int low, int high) const; // gets the sum of all value between the indexes 'low' and 'high' (inclusive)

	// spectrum math
	bool    Add(CSpectrum &spec2);
	bool    Div(CSpectrum &spec2);
	bool    Sub(CSpectrum &spec2);

	// scalar math
	bool    Add(double value);
	bool    Div(double value);
	bool    Sub(double value);
	bool    Mult(double value);

	// clearing out the information in the spectrum
	void    Clear();

	// allocating the spectrum. Sets 'length' and allocates 'data'
	void    Resize(int size);

	// Returns a pointer to the beginning of the dataset, for backwards compatibility
	double*       Ptr() { return this->data.data(); }
	const double* Ptr() const { return this->data.data(); }

	// appends one more spectrum data point to 'data' and increments 'size' with one.
	//  returns the new size of the spectrum
	int     AppendPoint(double value);
};
