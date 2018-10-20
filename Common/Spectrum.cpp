#include "StdAfx.h"
#include "Spectrum.h"
#include "../Common.h"
#include <algorithm>

#undef min
#undef max

CSpectrum::CSpectrum()
{
	data = std::vector<double>(MAX_SPECTRUM_LENGTH, 0);
	this->Clear();
}

CSpectrum::CSpectrum(int size)
{
	data = std::vector<double>(size, 0);
	this->Clear();
}

CSpectrum::~CSpectrum(void)
{
}

CSpectrum::CSpectrum(const CSpectrum& other) {
	this->isDark = other.isDark;
	this->exposureTime = other.exposureTime;
	this->lat = other.lat;
	this->lon = other.lon;
	this->altitude = other.altitude;
	this->scans = other.scans;
	this->spectrometerSerial = other.spectrometerSerial;
	this->spectrometerModel = other.spectrometerModel;
	this->name = other.name;

	memcpy(this->date, other.date, 3 * sizeof(int));
	memcpy(this->startTime, other.startTime, 3 * sizeof(int));
	memcpy(this->stopTime, other.stopTime, 3 * sizeof(int));

	this->data = std::vector<double>(begin(other.data), end(other.data));
}

CSpectrum& CSpectrum::operator=(CSpectrum other)
{
	swap(*this, other);
	return *this;
}

void swap(CSpectrum & first, CSpectrum & second)
{
	using std::swap;

	swap(first.isDark, second.isDark);
	swap(first.exposureTime, second.exposureTime);
	swap(first.lat, second.lat);
	swap(first.lon, second.lon);
	swap(first.altitude, second.altitude);
	swap(first.scans, second.scans);
	swap(first.spectrometerSerial, second.spectrometerSerial);
	swap(first.spectrometerModel, second.spectrometerModel);
	swap(first.name, second.name);

	for(int ii = 0; ii < 3; ++ii) {
		swap(first.date[ii], second.date[ii]);
		swap(first.startTime[ii], second.startTime[ii]);
		swap(first.stopTime[ii], second.stopTime[ii]);
	}

	swap(first.data, second.data);
}

void CSpectrum::GetMinMax(double& minValue, double&maxValue) const
{
	if (0 == this->data.size())
	{
		minValue = 0.0;
		maxValue = 0.0;
		return;
	}

	minValue = data[0];
	maxValue = data[0];

	for (int ii = 0; ii < this->data.size(); ++ii)
	{
		minValue = std::min(minValue, data[ii]);
		maxValue = std::max(maxValue, data[ii]);
	}
}

double CSpectrum::GetMax() const {
	double maxValue = data[0];

	for (size_t i = 1; i < this->data.size(); ++i)
	{
		maxValue = std::max(maxValue, data[i]);
	}

	return maxValue;
}

double CSpectrum::GetAverage() const {

	if (this->data.size() == 0)
		return 0;
	else {
		double sum = this->GetSum();
		return (sum / this->data.size());
	}
}

double CSpectrum::GetAverage(int low, int high) const {
	low = std::max(low, 0);
	high = std::min(high, (int)data.size());

	if (this->data.size() == 0) {
		return 0;
	}

	if (high == low) {
		return 0;
	}

	double sum = this->GetSum(low, high);
	return (sum / (high - low));
}

void CSpectrum::GetStatistics(double& average, double& variance) const
{
	double count = 0;
	double mean  = 0.0;
	double m2    = 0.0;

	for (size_t ii = 0; ii < this->data.size(); ++ii)
	{
		++count;
		const double delta = data[ii] - mean;
		mean += delta / count;
		const double delta2 = data[ii] - mean;
		m2 += delta * delta2;
	}

	average = mean;
	variance = m2 / count;
}

double CSpectrum::GetSum() const {
	double sum = 0;
	
	for (size_t i = 0; i < this->data.size(); ++i) {
		sum += data[i];
	}

	return sum;
}

double CSpectrum::GetSum(int low, int high) const {
	double sum = 0;

	if (low > high) {
		return 0;
	}

	low  = std::max(low, 0);
	high = std::min(high, (int)this->data.size());

	for (int i = low; i < high; ++i) {
		sum += data[i];
	}

	return sum;
}

bool CSpectrum::Add(CSpectrum &spec2) {
	if (this->data.size() != spec2.data.size()) {
		return false;
	}

	for (size_t i = 0; i < this->data.size(); ++i) {
		this->data[i] += spec2.data[i];
	}

	scans += spec2.scans;

	return true;
}

bool CSpectrum::Div(CSpectrum &spec2) {
	if (this->data.size() != spec2.data.size()) {
		return false;
	}

	for (size_t i = 0; i < this->data.size(); ++i) {
		if (spec2.data[i] == 0) {
			this->data[i] = 0;
		}
		else {
			this->data[i] /= spec2.data[i];
		}
	}
	return true;
}

bool CSpectrum::Sub(CSpectrum &spec2) {
	if (this->data.size() != spec2.data.size()) {
		return false;
	}

	for (size_t i = 0; i < this->data.size(); ++i) {
		this->data[i] -= spec2.data[i];
	}
	return true;
}

bool CSpectrum::Add(double value) {
	for (size_t i = 0; i < this->data.size(); ++i) {
		this->data[i] += value;
	}
	return true;
}

bool CSpectrum::Div(double value) {
	if (value == 0) {
		return false;
	}

	for (size_t i = 0; i < this->data.size(); ++i) {
		this->data[i] /= value;
	}
	return true;
}

bool CSpectrum::Mult(double value) {
	if (value == 0) {
		return false;
	}

	for (size_t i = 0; i < this->data.size(); ++i) {
		this->data[i] *= value;
	}
	return true;
}

bool CSpectrum::Sub(double value) {
	for (size_t i = 0; i < this->data.size(); ++i) {
		this->data[i] -= value;
	}
	return true;
}

// clearing out the information in the spectrum
void CSpectrum::Clear() {
	scans = 0;
	exposureTime = 0;
	lat = 0.0;
	lon = 0.0;
	altitude = 0.0;
	spectrometerSerial = "";
	spectrometerModel = "";
	name = "";

	date[0] = date[1] = date[2] = 0;
	startTime[0] = startTime[1] = startTime[2] = 0;
	stopTime[0] = stopTime[1] = stopTime[2] = 0;

	isDark = false;
	data   = std::vector<double>(MAX_SPECTRUM_LENGTH, 0);
}

void CSpectrum::Resize(int size)
{
	if (size < 0)
	{
		throw std::invalid_argument("Cannot set a negative spectrum size.");
	}

	this->data   = std::vector<double>(size, 0);
}

int CSpectrum::AppendPoint(double value)
{
	this->data.push_back(value);

	return (int)this->data.size();
}
