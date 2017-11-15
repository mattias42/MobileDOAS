/* The following two definitions are necesarry since
	VC6 (and VC7) does not handle the long long data-type 
	used by the OceanOptics OmniDriver
*/
#define HIGHRESTIMESTAMP_H
//#define SPECTROMETERCHANNEL_H

#include "stdafx.h"
#include "../DMSpec.h"
#include "Configure_Spectrometer.h"

using namespace Configuration;

extern CString g_exePath;  // <-- This is the path to the executable. This is a global variable and should only be changed in DMSpecView.cpp

// CConfigure_Spectrometer dialog

IMPLEMENT_DYNAMIC(CConfigure_Spectrometer, CPropertyPage)
CConfigure_Spectrometer::CConfigure_Spectrometer()
	: CPropertyPage(CConfigure_Spectrometer::IDD)
{
	m_conf = NULL;
	m_availableBaudrates[0] = 4800;
	m_availableBaudrates[1] = 9600;
	m_availableBaudrates[2] = 19200;
	m_availableBaudrates[3] = 38400;
	m_availableBaudrates[4] = 57600;
	m_availableBaudrates[5] = 115200;
}

CConfigure_Spectrometer::~CConfigure_Spectrometer()
{
	m_conf = NULL;
}

void CConfigure_Spectrometer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	// The type of connection to the spectrometer
	DDX_Radio(pDX, IDC_RADIO_CONNECTION_USB, m_conf->m_spectrometerConnection);

	// The determination of the exposure-time
	DDX_Radio(pDX, IDC_RADIO_EXPTIME_AUTOMATIC, m_conf->m_expTimeMode);

	// The parameters for judging how to calculate the exp-time
	DDX_Text(pDX,		IDC_EDIT_SPECCENTER,		m_conf->m_specCenter);
	DDX_Text(pDX,		IDC_EDIT_PERCENT,				m_conf->m_percent);
	DDX_Text(pDX,		IDC_EDIT_FIXEXPTIME,		m_conf->m_fixExpTime);
	DDX_Text(pDX,		IDC_EDIT_TIMERESOLUTION,m_conf->m_timeResolution);
	DDX_Control(pDX,IDC_EDIT_SPECCENTER,		m_editSpecCenter);
	DDX_Control(pDX,IDC_EDIT_PERCENT,				m_editPercent);
	DDX_Control(pDX,IDC_EDIT_FIXEXPTIME,		m_editFixExpTime);

	// Misc. settings
	DDX_Text(pDX,		IDC_EDIT_MAXCOLUMN,			m_conf->m_maxColumn);

	// The time resolution
	DDX_Control(pDX,IDC_EDIT_TIMERESOLUTION,m_editTimeResolution);

	// The removal of the offset
	DDX_Control(pDX,	IDC_EDIT_OFFSETFROM,	m_editOffsetFrom);
	DDX_Control(pDX,	IDC_EDIT_OFFSETTO,		m_editOffsetTo);

	DDX_Text(pDX,		IDC_EDIT_OFFSETFROM,		m_conf->m_offsetFrom);
	DDX_Text(pDX,		IDC_EDIT_OFFSETTO,			m_conf->m_offsetTo);

	// The combo-boxes
	DDX_Control(pDX, IDC_COMBO_PORT,			m_specPort);
	DDX_Control(pDX, IDC_COMBO_BAUDRATE,	m_specBaudrate);
	DDX_Control(pDX, IDC_COMBO_NCHANNELS,	m_nChannels);
}


BEGIN_MESSAGE_MAP(CConfigure_Spectrometer, CPropertyPage)
	// Changing the contents of the radio-boxes:
	ON_LBN_SELCHANGE(IDC_COMBO_PORT,			SaveSettings)
	ON_LBN_SELCHANGE(IDC_COMBO_BAUDRATE,	SaveSettings)
	ON_LBN_SELCHANGE(IDC_COMBO_NCHANNELS, SaveSettings)

	// Changing the selection using the radio-buttons:
	ON_BN_CLICKED(IDC_RADIO_CONNECTION_USB,			SaveSettings)
	ON_BN_CLICKED(IDC_RADIO_CONNECTION_SERIAL,	SaveSettings)
	ON_BN_CLICKED(IDC_RADIO_EXPTIME_AUTOMATIC,	SaveSettings)
	ON_BN_CLICKED(IDC_RADIO_EXPTIME_FIXED,			SaveSettings)
	ON_BN_CLICKED(IDC_RADIO_EXPTIME_ADAPTIVE,		SaveSettings)

	// Changing the contents of the edit-boxes
	ON_EN_CHANGE(IDC_EDIT_SPECCENTER,						SaveSettings)
	ON_EN_CHANGE(IDC_EDIT_PERCENT,							SaveSettings)
	ON_EN_CHANGE(IDC_EDIT_FIXEXPTIME,						SaveSettings)
	ON_EN_CHANGE(IDC_EDIT_TIMERESOLUTION,				SaveSettings)
	ON_EN_CHANGE(IDC_EDIT_OFFSETFROM,						SaveSettings)
	ON_EN_CHANGE(IDC_EDIT_OFFSETTO,							SaveSettings)
END_MESSAGE_MAP()


// CConfigure_Spectrometer message handlers
BOOL CConfigure_Spectrometer::OnInitDialog(){

	CPropertyPage::OnInitDialog();

	int k;
	CString str;

	// Initialize the serial-port combo-box
	int toSelect = -1;
	for(k = 1; k < 22; ++k){
		str.Format("COM%d", k);
		m_specPort.AddString(str);
		if(Equals(str, m_conf->m_serialPort))
			toSelect = k - 1;
	}
	m_specPort.SetCurSel(toSelect);

	// Initialize the baudrate-combo-box
	toSelect = -1;
	for(k = 0; k < 6; ++k){
		str.Format("%d", m_availableBaudrates[k]);
		m_specBaudrate.AddString(str);
		if(m_availableBaudrates[k] == m_conf->m_baudrate)
			toSelect = k;
	}
	m_specBaudrate.SetCurSel(toSelect);

	// Initialize the channels - combo-box
	str.Format("1");		m_nChannels.AddString(str);
	str.Format("2");		m_nChannels.AddString(str);
	m_nChannels.SetCurSel(m_conf->m_nChannels - 1);

	EnableControls();

	// setup the tool tips
	InitToolTips();

	return TRUE;
}

void CConfigure_Spectrometer::SaveSettings(){
	UpdateData(TRUE); // <-- save the data in the dialog
	int curPort = m_specPort.GetCurSel();
	int curBaud = m_specBaudrate.GetCurSel();
	int curChan = m_nChannels.GetCurSel();

	// The port
	m_conf->m_serialPort.Format("COM%d", curPort + 1);

	// The baudrate
	if(curBaud == -1){
		m_conf->m_baudrate = 9600;
		m_specBaudrate.SetCurSel(1);
	}else{
		m_conf->m_baudrate	=	m_availableBaudrates[curBaud];
	}

	// The channel
	m_conf->m_nChannels = max(curChan, 0) + 1;

	EnableControls();
}

/** Enables the controls that should be enabled, and disables
		the ones which should be disabled */
void CConfigure_Spectrometer::EnableControls(){
	UpdateData(TRUE); // <-- save the data in the dialog

	// Enable or disable the combo-boxes based on the preferred method of connection
	if(m_conf->m_spectrometerConnection == CMobileConfiguration::CONNECTION_USB){
		m_specPort.EnableWindow(FALSE);
		m_specBaudrate.EnableWindow(FALSE);
	}else if(m_conf->m_spectrometerConnection == CMobileConfiguration::CONNECTION_RS232){
		m_specPort.EnableWindow(TRUE);
		m_specBaudrate.EnableWindow(TRUE);
	}

	// Enable or disable the edit-boxes based on the preferred method 
	//		of calculating the exposure time
	if(m_conf->m_expTimeMode == CMobileConfiguration::EXPOSURETIME_AUTOMATIC){
		m_editSpecCenter.EnableWindow(TRUE);
		m_editPercent.EnableWindow(TRUE);
		m_editFixExpTime.EnableWindow(FALSE);
	}else if(m_conf->m_expTimeMode == CMobileConfiguration::EXPOSURETIME_FIXED){
		m_editSpecCenter.EnableWindow(FALSE);
		m_editPercent.EnableWindow(FALSE);
		m_editFixExpTime.EnableWindow(TRUE);
	}else if(m_conf->m_expTimeMode == CMobileConfiguration::EXPOSURETIME_ADAPTIVE){
		m_editSpecCenter.EnableWindow(TRUE);
		m_editPercent.EnableWindow(TRUE);
		m_editFixExpTime.EnableWindow(FALSE);
	}
}

/** Called when the 'Save' - button is pressed */
void CConfigure_Spectrometer::OnOK(){
	FILE *f = NULL;
	CString fileName;

	// Get the filename (and path) of the configuration-file
	if(strlen(m_conf->m_cfgFile) == 0){
		fileName.Format("%s\\cfg.xml", g_exePath);
	}else{
		int p = m_conf->m_cfgFile.Find(".txt");
		if(p > 0){
			fileName.Format("%s.xml", m_conf->m_cfgFile.Left(p));
		}else{
			fileName.Format(m_conf->m_cfgFile);
		}
	}

	// Try to open the file for writing
	f = fopen(fileName, "w");
	if(f == NULL){
		CString errMsg;
		errMsg.Format("Could not open file %s for writing.", fileName);
		MessageBox(errMsg, "Error", MB_OK);
		return;
	}

	// ---- Standard beginning ---
	fprintf(f, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	fprintf(f, "<!-- This is the configuration file MobileDOAS -->\n\n");
	fprintf(f, "<Configuration>\n");

	// ------------ Spectrometer Settings ------------------
	if(m_conf->m_spectrometerConnection == CMobileConfiguration::CONNECTION_RS232){
		fprintf(f, "\t<serialPort>%s</serialPort>\n", m_conf->m_serialPort);
		fprintf(f, "\t<serialBaudrate>%d</serialBaudrate>\n",	m_conf->m_baudrate);
	}else{
		fprintf(f, "\t<serialPort>USB</serialPort>\n");
	}
	fprintf(f, "\t<timeResolution>%d</timeResolution>\n",	m_conf->m_timeResolution);

	fprintf(f, "\t<nchannels>%d</nchannels>\n",					m_conf->m_nChannels);

	fprintf(f, "\t<maxColumn>%.2lf</maxColumn>\n",			m_conf->m_maxColumn);

	//if(m_conf->m_nChannels > 1){
	//	fprintf(f,	"NCHANNELS=%d\n",	m_conf->m_nChannels);
	//}

	// ------------ Settings for the Exposure-time -------------
	fprintf(f, "\t<Intensity>\n");
	fprintf(f, "\t\t<Percent>%d</Percent>\n",						m_conf->m_percent);
	fprintf(f, "\t\t<Channel>%d</Channel>\n",						m_conf->m_specCenter);
	if(m_conf->m_expTimeMode == CMobileConfiguration::EXPOSURETIME_FIXED){
		fprintf(f, "\t\t<FixExpTime>%d</FixExpTime>\n",		m_conf->m_fixExpTime);
	}else if(m_conf->m_expTimeMode == CMobileConfiguration::EXPOSURETIME_ADAPTIVE){
		fprintf(f, "\t\t<FixExpTime>-1</FixExpTime>\n");
	}else{
		fprintf(f, "\t\t<FixExpTime>0</FixExpTime>\n");
	}
	fprintf(f, "\t</Intensity>\n");

	// ------------------- GPS-settings --------------------
	fprintf(f, "\t<GPS>\n");
	if(m_conf->m_useGPS)
		fprintf(f, "\t\t<use>1</use>\n");
	else
		fprintf(f, "\t\t<use>0</use>\n");

	fprintf(f, "\t\t<baudrate>%d</baudrate>\n",	m_conf->m_gpsBaudrate);
	fprintf(f, "\t\t<port>%s</port>\n",	m_conf->m_gpsPort);
	fprintf(f, "\t</GPS>\n");

	// ------------------- Offset-settings --------------------
	fprintf(f, "\t<Offset>\n");
	fprintf(f, "\t\t<from>%d</from>\n",	m_conf->m_offsetFrom);
	fprintf(f, "\t\t<to>%d</to>\n",			m_conf->m_offsetTo);
	fprintf(f, "\t</Offset>\n");

	// ----------- Evaluation ----------------
	for(int k = 0; k < m_conf->m_nFitWindows; ++k){
		fprintf(f, "\t<FitWindow>\n");
		fprintf(f, "\t\t<name>%s</name>\n",						m_conf->m_fitWindow[k].name);
		fprintf(f, "\t\t<fitLow>%d</fitLow>\n",					m_conf->m_fitWindow[k].fitLow);
		fprintf(f, "\t\t<fitHigh>%d</fitHigh>\n",				m_conf->m_fitWindow[k].fitHigh);
		fprintf(f, "\t\t<spec_channel>%d</spec_channel>\n",		m_conf->m_fitWindow[k].channel);
		fprintf(f, "\t\t<polynomial>%d</polynomial>\n",			m_conf->m_fitWindow[k].polyOrder);

		for(int j = 0; j < m_conf->m_fitWindow[k].nRef; ++j){
			fprintf(f, "\t\t<Reference>\n");
			fprintf(f, "\t\t\t<name>%s</name>\n",				m_conf->m_fitWindow[k].ref[j].m_specieName);
			fprintf(f, "\t\t\t<path>%s</path>\n",				m_conf->m_fitWindow[k].ref[j].m_path);
			fprintf(f, "\t\t\t<gasFactor>%.2lf</gasFactor>\n",	m_conf->m_fitWindow[k].ref[j].m_gasFactor);

			// Shift
			if(m_conf->m_fitWindow[k].ref[j].m_shiftOption == Evaluation::SHIFT_FIX){
				fprintf(f, "\t\t\t<shift>fix to %.2lf</shift>\n",		m_conf->m_fitWindow[k].ref[j].m_shiftValue);
			}else if(m_conf->m_fitWindow[k].ref[j].m_shiftOption == Evaluation::SHIFT_FREE){
				fprintf(f, "\t\t\t<shift>free</shift>\n");
			}else if(m_conf->m_fitWindow[k].ref[j].m_shiftOption == Evaluation::SHIFT_LINK){
				fprintf(f, "\t\t\t<shift>link to %.0lf</shift>\n",		m_conf->m_fitWindow[k].ref[j].m_shiftValue);
			}

			// Squeeze
			if(m_conf->m_fitWindow[k].ref[j].m_squeezeOption == Evaluation::SHIFT_FIX){
				fprintf(f, "\t\t\t<squeeze>fix to %.2lf</squeeze>\n",		m_conf->m_fitWindow[k].ref[j].m_squeezeValue);
			}else if(m_conf->m_fitWindow[k].ref[j].m_squeezeOption == Evaluation::SHIFT_FREE){
				fprintf(f, "\t\t\t<squeeze>free</squeeze>\n");
			}else if(m_conf->m_fitWindow[k].ref[j].m_squeezeOption == Evaluation::SHIFT_LINK){
				fprintf(f, "\t\t\t<squeeze>link to %.0lf</squeeze>\n",		m_conf->m_fitWindow[k].ref[j].m_squeezeValue);
			}
			fprintf(f, "\t\t</Reference>\n");
		}
		fprintf(f, "\t</FitWindow>\n");
	}

	fprintf(f, "</Configuration>\n");
	fclose(f);
}

void CConfigure_Spectrometer::InitToolTips(){
	// Don't initialize the tool tips twice
	if(m_toolTip.m_hWnd != NULL)
		return;

	// Enable the tool tips
  if(!m_toolTip.Create(this)){
    TRACE0("Failed to create tooltip control\n"); 
  }
	m_toolTip.AddTool(&m_specPort,							IDC_COMBO_PORT);
	m_toolTip.AddTool(&m_specBaudrate,					IDC_COMBO_BAUDRATE);
	m_toolTip.AddTool(&m_nChannels,							IDC_COMBO_NCHANNELS);
	m_toolTip.AddTool(&m_editSpecCenter,				IDC_EDIT_SPECCENTER);
	m_toolTip.AddTool(&m_editPercent,						IDC_EDIT_PERCENT);
	m_toolTip.AddTool(&m_editFixExpTime,				IDC_EDIT_FIXEXPTIME);
	m_toolTip.AddTool(&m_editTimeResolution,		IDC_EDIT_TIMERESOLUTION);
	m_toolTip.AddTool(&m_editOffsetTo,					IDC_EDIT_OFFSETTO);
	m_toolTip.AddTool(&m_editOffsetFrom,				IDC_EDIT_OFFSETFROM);

	m_toolTip.SetMaxTipWidth(SHRT_MAX);
  m_toolTip.Activate(TRUE);
}

BOOL CConfigure_Spectrometer::PreTranslateMessage(MSG* pMsg){
  m_toolTip.RelayEvent(pMsg);

  return CPropertyPage::PreTranslateMessage(pMsg);
}