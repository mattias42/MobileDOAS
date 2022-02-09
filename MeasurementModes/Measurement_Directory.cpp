#include "stdafx.h"
#include "Measurement_Directory.h"

extern CString g_exePath;  // <-- This is the path to the executable. This is a global variable and should only be changed in DMSpecView.cpp

CMeasurement_Directory::CMeasurement_Directory()
{
}


CMeasurement_Directory::~CMeasurement_Directory()
{
}

void CMeasurement_Directory::Run()
{
    ShowMessageBox("START", "NOTICE");

    // Read configuration file and apply settings
    CString cfgFile = g_exePath + TEXT("cfg.xml");
    m_conf.reset(new Configuration::CMobileConfiguration(cfgFile));
    ApplySettings();
    if (CheckSettings()) {
        ShowMessageBox("Error in configuration settings.", "Error");
        return;
    }
    m_spectrometerDynRange = m_conf->m_spectrometerDynamicRange;

    // Get directory to watch
    if (strlen(m_conf->m_directory) == 0) {
        ShowMessageBox("<Directory> parameter not set in configuration file.", "Error");
        return;
    }
    m_subFolder = m_conf->m_directory;

    // Update MobileLog.txt 
    UpdateMobileLog();

    // get sky 
    if (!ReadSky()) {
        return;
    }

    // get dark (dark or darkcur)
    if (m_conf->m_expTimeMode == m_conf->EXPOSURETIME_ADAPTIVE) {
        if (!ReadDarkcur()) {
            return;
        }
    }
    else {
        if (!ReadDark()) {
            return;
        }
    }

    // get offset if in adaptive mode
    if (m_conf->m_expTimeMode == m_conf->EXPOSURETIME_ADAPTIVE) {
        if (!ReadOffset()) {
            return;
        }
    }

    // Create the output directory
    CreateDirectories();

    // If we should do an automatic calibration, then do so now
    if (m_conf->m_calibration.m_enable)
    {
        RunInstrumentCalibration(m_sky[0], m_dark[0], m_detectorSize);
    }

    // Read reference files
    m_statusMsg.Format("Reading References");
    pView->PostMessage(WM_STATUSMSG);
    if (ReadReferenceFiles()) {
        ShowMessageBox("Error reading reference files.", "Error");
        return;
    }

    InitializeEvaluators(m_conf->m_expTimeMode == m_conf->EXPOSURETIME_ADAPTIVE);

    WriteEvaluationLogFileHeaders();

    // variables used for adaptive mode
    long serialDelay, gpsDelay;
    // other things to do in adaptive mode
    if (m_conf->m_expTimeMode == m_conf->EXPOSURETIME_ADAPTIVE) {
        // subtrace offset from dark
        for (int i = 0; i < MAX_SPECTRUM_LENGTH; ++i) {
            m_darkCur[0][i] = m_darkCur[0][i] - m_offset[0][i];
        }
        m_averageSpectrumIntensity[0] = AverageIntens(m_sky[0], 1);

        // remove the dark spectrum from sky
        GetDark();
        for (int iterator = 0; iterator < MAX_SPECTRUM_LENGTH; ++iterator) {
            m_sky[0][iterator] -= m_tmpDark[0][iterator];
        }

        /* Set the delays and initialize the USB-Connection */
        if (m_connectViaUsb) {
            serialDelay = 10;
        }
        else {
            serialDelay = 2300;
        }
        gpsDelay = 10;
    }

    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    CString lastShown;
    CString filter;
    while (m_isRunning) {
        // get only normal spectrum files
        filter.Format("%s\\?????_?.STD", m_conf->m_directory);
        hFind = FindFirstFile(filter, &ffd);
        while (m_isRunning && INVALID_HANDLE_VALUE == hFind) {
            m_statusMsg.Format("Waiting for spectra file...");
            pView->PostMessage(WM_STATUSMSG);
            Sleep(1000);
            hFind = FindFirstFile(filter, &ffd);
        }

        FILETIME latestFiletime = ffd.ftLastWriteTime;
        CString latestSpectrumFile = ffd.cFileName;
        // loop through files to find latest
        do
        {
            // double check it's not a directory (though shouldn't be with the filter)
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                FILETIME filetime = ffd.ftLastWriteTime;
                CString filename = ffd.cFileName;
                // compare file time
                int comp = CompareFileTime(&filetime, &latestFiletime);
                switch (comp) {
                case 1:
                    // if file has a later timestamp than last spec file processed
                    latestFiletime = filetime;
                    latestSpectrumFile = filename;
                    break;
                case 0:
                    // if file has same time than last spec file processed,
                    // compare file names for spec number and use the larger
                    std::string fn = filename;
                    std::string lfn = latestSpectrumFile;
                    int strcomp = fn.compare(lfn);
                    if (strcomp == 1) {
                        latestFiletime = filetime;
                        latestSpectrumFile = filename;
                    }
                }
            }

        } while (m_isRunning && FindNextFile(hFind, &ffd) != 0);

        if (latestSpectrumFile.Compare(lastShown) == 0) {
            // skip processing if same as before
            Sleep(m_conf->m_sleep);
            continue;
        }

        if (ProcessSpectrum(latestSpectrumFile)) {
            lastShown = latestSpectrumFile; // update last shown spectra
        }
        Sleep(m_conf->m_sleep);
    }
}

bool CMeasurement_Directory::ProcessSpectrum(CString latestSpectrum) {
    CString specfile;
    CSpectrum spec;
    specfile.Format("%s\\%s", m_conf->m_directory, latestSpectrum);
    if (CSpectrumIO::readSTDFile(specfile, &spec) == 1) {
        m_statusMsg.Format("Error reading %s", specfile);
        pView->PostMessage(WM_STATUSMSG);
        return FAIL;
    }
    else {
        // get spec number and channel num
        if (m_conf->m_expTimeMode == m_conf->EXPOSURETIME_ADAPTIVE) {
            m_scanNum = atoi(latestSpectrum.Left(5)) + 3;
        }
        else {
            m_scanNum = atoi(latestSpectrum.Left(5)) + 2;
        }
        int channel = atoi(latestSpectrum.Mid(7, 1));

        // copy data to current spectrum 
        memcpy((void*)m_curSpectrum[channel], (void*)spec.I, sizeof(double) * MAX_SPECTRUM_LENGTH); // for plot

        // extract spectrum integration time and total spec num
        m_integrationTime = spec.exposureTime;
        m_totalSpecNum = spec.scans;
        pView->PostMessage(WM_SHOWINTTIME);

        // get spectrum date & time
        m_spectrumGpsData[m_spectrumCounter].date
            = stoi(spec.date.substr(7, 2) + spec.date.substr(3, 2) + spec.date.substr(0, 2));
        char startTime[6];
        snprintf(startTime, 6, "%02d%02d%02d", spec.startTime[0], spec.startTime[1], spec.startTime[2]);
        m_spectrumGpsData[m_spectrumCounter].time = atoi(startTime);

        // get spectrum coordinates
        m_spectrumGpsData[m_spectrumCounter].latitude = spec.lat;
        m_spectrumGpsData[m_spectrumCounter].longitude = spec.lon;
        m_spectrumGpsData[m_spectrumCounter].altitude = spec.altitude;
        pView->PostMessage(WM_READGPS);

        // calculate average intensity
        m_averageSpectrumIntensity[channel] = AverageIntens(m_curSpectrum[channel], 1);
        vIntensity.Append(m_averageSpectrumIntensity[channel]);

        // get spectrum info
        GetSpectrumInfo(m_curSpectrum);

        // do the fit
        GetDark();
        GetSky();
        DoEvaluation(m_tmpSky, m_tmpDark, m_curSpectrum);
        m_statusMsg.Format("Showing spectra file %s", specfile);
        pView->PostMessage(WM_STATUSMSG);
        return SUCCESS;
    }
}

bool CMeasurement_Directory::ReadSky() {
    WIN32_FIND_DATA ffd; // handle to spec file
    CString specfile; // spec file name
    CSpectrum spec; // spectrum object
    CString filter; // spec file filter
    filter.Format("%s\\sky_0.STD", m_conf->m_directory);
    HANDLE hFind = FindFirstFile(filter, &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        if (strlen(m_conf->m_defaultSkyFile) == 0) {
            ShowMessageBox("No sky file specified or in directory.", "Error");
            return FAIL;
        }
        specfile.Format(m_conf->m_defaultSkyFile);
    }
    else {
        specfile.Format("%s\\%s", m_conf->m_directory, ffd.cFileName);
    }

    if (CSpectrumIO::readSTDFile(specfile, &spec) == 1) {
        ShowMessageBox("Error reading sky file.", "Error");
        return FAIL;
    }
    else {
        memcpy((void*)m_sky[0], (void*)spec.I, sizeof(double) * MAX_SPECTRUM_LENGTH);
        m_integrationTime = spec.exposureTime;
        m_totalSpecNum = spec.scans;
        m_detectorSize = spec.length;
        m_spectrometerName = spec.spectrometerSerial;
    }
    return SUCCESS;
}

bool CMeasurement_Directory::ReadDark() {
    WIN32_FIND_DATA ffd; // handle to spec file
    CString specfile; // spec file name
    CSpectrum spec; // spectrum object
    CString filter; // spec file filter
    filter.Format("%s\\dark_0.STD", m_conf->m_directory);
    HANDLE hFind = FindFirstFile(filter, &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        if (strlen(m_conf->m_defaultDarkFile) == 0) {
            ShowMessageBox("No dark file specified or in directory.", "Error");
            return FAIL;
        }
        specfile.Format(m_conf->m_defaultDarkFile);
    }
    else {
        specfile.Format("%s\\%s", m_conf->m_directory, ffd.cFileName);
    }
    if (CSpectrumIO::readSTDFile(specfile, &spec) == 1) {
        ShowMessageBox("Error reading dark file.", "Error");
        return FAIL;
    }
    else {
        memcpy((void*)m_dark[0], (void*)spec.I, sizeof(double) * MAX_SPECTRUM_LENGTH);
    }
    return SUCCESS;
}

bool CMeasurement_Directory::ReadDarkcur() {
    WIN32_FIND_DATA ffd; // handle to spec file
    CString specfile; // spec file name
    CSpectrum spec; // spectrum object
    CString filter; // spec file filter
    filter.Format("%s\\darkcur_0.STD", m_conf->m_directory);
    HANDLE hFind = FindFirstFile(filter, &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        if (strlen(m_conf->m_defaultDarkcurFile) == 0) {
            ShowMessageBox("No darkcur file specified or in directory.", "Error");
            return FAIL;
        }
        specfile.Format(m_conf->m_defaultDarkcurFile);
    }
    else {
        specfile.Format("%s\\%s", m_conf->m_directory, ffd.cFileName);
    }
    if (CSpectrumIO::readSTDFile(specfile, &spec) == 1) {
        ShowMessageBox("Error reading darkcur file.", "Error");
        return FAIL;
    }
    else {
        memcpy((void*)m_darkCur[0], (void*)spec.I, sizeof(double) * MAX_SPECTRUM_LENGTH);
    }
    return SUCCESS;
}

bool CMeasurement_Directory::ReadOffset() {
    WIN32_FIND_DATA ffd; // handle to spec file
    CString specfile; // spec file name
    CSpectrum spec; // spectrum object
    CString filter; // spec file filter
    filter.Format("%s\\offset_0.STD", m_conf->m_directory);
    HANDLE hFind = FindFirstFile(filter, &ffd);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        if (strlen(m_conf->m_defaultOffsetFile) == 0) {
            ShowMessageBox("No offset file specified or in directory.", "Error");
            return FAIL;
        }
        specfile.Format(m_conf->m_defaultOffsetFile);
    }
    else {
        specfile.Format("%s\\%s", m_conf->m_directory, ffd.cFileName);
    }
    if (CSpectrumIO::readSTDFile(specfile, &spec) == 1) {
        ShowMessageBox("Error reading offset file.", "Error");
        return FAIL;
    }
    else {
        memcpy((void*)m_offset[0], (void*)spec.I, sizeof(double) * MAX_SPECTRUM_LENGTH);
    }
    return SUCCESS;
}