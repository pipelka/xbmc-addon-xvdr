/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2013 Alexander Pipelka
 *
 *      https://github.com/pipelka/xbmc-addon-xvdr
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogChannelScanner.h"

#define CTL_DVBTYPE         2
#define CTL_COUNTRY         3
#define CTL_INVERSION_DVBT  5
#define CTL_INVERSION_DVBC  7
#define CTL_SYMBOLRATE_DVBC 8
#define CTL_QAM_DVBC        9
#define CTL_SATELLITE_DVBS  11
#define CTL_START           20
#define CTL_CANCEL          21

CGUIDialogChannelScanner::CGUIDialogChannelScanner(CHelper_libXBMC_gui* GUI, XVDR::Connection* connection) : m_gui(GUI), m_connection(connection), m_scanning(false) {
  m_window = m_gui->Window_create("ChannelScan.xml", "skin.confluence", false, true);

  m_window->m_cbhdl   = this;
  m_window->CBOnInit  = OnInitCB;
  m_window->CBOnFocus = OnFocusCB;
  m_window->CBOnClick = OnClickCB;
  m_window->CBOnAction= OnActionCB;

}

CGUIDialogChannelScanner::~CGUIDialogChannelScanner(){
  m_gui->Window_destroy(m_window);
}

bool CGUIDialogChannelScanner::OnClickCB(GUIHANDLE cbhdl, int controlId) {
  CGUIDialogChannelScanner* dialog = static_cast<CGUIDialogChannelScanner*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogChannelScanner::OnFocusCB(GUIHANDLE cbhdl, int controlId) {
  CGUIDialogChannelScanner* dialog = static_cast<CGUIDialogChannelScanner*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogChannelScanner::OnInitCB(GUIHANDLE cbhdl) {
  CGUIDialogChannelScanner* dialog = static_cast<CGUIDialogChannelScanner*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogChannelScanner::OnActionCB(GUIHANDLE cbhdl, int actionId) {
  return true;
}

void CGUIDialogChannelScanner::Close() {
  OnClose();
  m_window->Close();
}

void CGUIDialogChannelScanner::DoModal() {
  m_window->DoModal();
}

void CGUIDialogChannelScanner::OnClose() {
  m_gui->Control_releaseSpin(m_dvbtype);
  m_gui->Control_releaseSpin(m_country);
  m_gui->Control_releaseSpin(m_inv_dvbt);
  m_gui->Control_releaseSpin(m_inv_dvbc);
  m_gui->Control_releaseSpin(m_symbolrate);
  m_gui->Control_releaseSpin(m_qam);
  m_gui->Control_releaseSpin(m_sat);
  m_gui->Control_releaseRadioButton(m_flag_tv);
  m_gui->Control_releaseRadioButton(m_flag_radio);
  m_gui->Control_releaseRadioButton(m_flag_fta);
  m_gui->Control_releaseRadioButton(m_flag_encrypted);
  m_gui->Control_releaseRadioButton(m_flag_hd);
}

bool CGUIDialogChannelScanner::OnFocus(int controlId) {
  return true;
}

bool CGUIDialogChannelScanner::OnInit() {
  // load scanner setup
  m_connection->GetChannelScannerSetup(m_setup, m_satellites, m_countries);

  // dvbtype
  m_dvbtype = m_gui->Control_getSpin(m_window, CTL_DVBTYPE);
  m_dvbtype->Clear();
  m_dvbtype->AddLabel("DVB-T", XVDR::ChannelScannerSetup::DVBTYPE_DVBT);
  m_dvbtype->AddLabel("DVB-C", XVDR::ChannelScannerSetup::DVBTYPE_DVBC);
  m_dvbtype->AddLabel("DVB-S/S2", XVDR::ChannelScannerSetup::DVBTYPE_DVBS);
  m_dvbtype->SetValue((int)m_setup.dvbtype);

  // country
  m_country = m_gui->Control_getSpin(m_window, CTL_COUNTRY);
  m_country->Clear();
  for(XVDR::ChannelScannerList::iterator i = m_countries.begin(); i != m_countries.end(); i++) {
    m_country->AddLabel(i->second.fullname.c_str(), i->first);
  }
  m_country->SetValue((int)m_setup.countryid);

  // inversion dvb-t
  m_inv_dvbt = m_gui->Control_getSpin(m_window, CTL_INVERSION_DVBT);
  m_inv_dvbt->Clear();
  m_inv_dvbt->AddLabel("AUTO / OFF", XVDR::ChannelScannerSetup::DVBINVERSION_AUTO);
  m_inv_dvbt->AddLabel("ON", XVDR::ChannelScannerSetup::DVBINVERSION_ON);
  m_inv_dvbt->SetValue((int)m_setup.dvbt_inversion);

  // inversion dvb-c
  m_inv_dvbc = m_gui->Control_getSpin(m_window, CTL_INVERSION_DVBC);
  m_inv_dvbc->Clear();
  m_inv_dvbc->AddLabel("AUTO / OFF", XVDR::ChannelScannerSetup::DVBINVERSION_AUTO);
  m_inv_dvbc->AddLabel("ON", XVDR::ChannelScannerSetup::DVBINVERSION_ON);
  m_inv_dvbc->SetValue((int)m_setup.dvbc_inversion);

  // symbolrate dvb-c
  m_symbolrate = m_gui->Control_getSpin(m_window, CTL_SYMBOLRATE_DVBC);
  m_symbolrate->Clear();
  m_symbolrate->AddLabel("AUTO", XVDR::ChannelScannerSetup::SYMBOLRATE_AUTO);
  m_symbolrate->AddLabel("7000", XVDR::ChannelScannerSetup::SYMBOLRATE_7000);
  m_symbolrate->AddLabel("6875", XVDR::ChannelScannerSetup::SYMBOLRATE_6875);
  m_symbolrate->AddLabel("6811", XVDR::ChannelScannerSetup::SYMBOLRATE_6811);
  m_symbolrate->AddLabel("6790", XVDR::ChannelScannerSetup::SYMBOLRATE_6790);
  m_symbolrate->AddLabel("6250", XVDR::ChannelScannerSetup::SYMBOLRATE_6250);
  m_symbolrate->AddLabel("6111", XVDR::ChannelScannerSetup::SYMBOLRATE_6111);
  m_symbolrate->AddLabel("5900", XVDR::ChannelScannerSetup::SYMBOLRATE_5900);
  m_symbolrate->AddLabel("5156", XVDR::ChannelScannerSetup::SYMBOLRATE_5156);
  m_symbolrate->AddLabel("5000", XVDR::ChannelScannerSetup::SYMBOLRATE_5000);
  m_symbolrate->AddLabel("4583", XVDR::ChannelScannerSetup::SYMBOLRATE_4583);
  m_symbolrate->AddLabel("4000", XVDR::ChannelScannerSetup::SYMBOLRATE_4000);
  m_symbolrate->AddLabel("3450", XVDR::ChannelScannerSetup::SYMBOLRATE_3450);
  m_symbolrate->SetValue((int)m_setup.dvbc_symbolrate);

  // qam dvb-c
  m_qam = m_gui->Control_getSpin(m_window, CTL_QAM_DVBC);
  m_qam->Clear();
  m_qam->AddLabel("AUTO", XVDR::ChannelScannerSetup::QAM_AUTO);
  m_qam->AddLabel("64", XVDR::ChannelScannerSetup::QAM_64);
  m_qam->AddLabel("128", XVDR::ChannelScannerSetup::QAM_128);
  m_qam->AddLabel("256", XVDR::ChannelScannerSetup::QAM_256);
  m_qam->AddLabel("ALL", XVDR::ChannelScannerSetup::QAM_ALL);
  m_qam->SetValue((int)m_setup.dvbc_qam);

  // satellite dvb-s
  m_sat = m_gui->Control_getSpin(m_window, CTL_SATELLITE_DVBS);
  m_sat->Clear();
  for(XVDR::ChannelScannerList::iterator i = m_satellites.begin(); i != m_satellites.end(); i++) {
    m_sat->AddLabel(i->second.fullname.c_str(), i->first);
  }
  m_sat->SetValue((int)m_setup.satid);

  // set flags
  m_flag_tv = m_gui->Control_getRadioButton(m_window, 32);
  m_flag_radio = m_gui->Control_getRadioButton(m_window, 13);
  m_flag_fta = m_gui->Control_getRadioButton(m_window, 14);
  m_flag_encrypted = m_gui->Control_getRadioButton(m_window, 15);
  m_flag_hd = m_gui->Control_getRadioButton(m_window, 16);

  m_flag_tv->SetSelected(m_setup.flags & XVDR::ChannelScannerSetup::FLAG_TV);
  m_flag_radio->SetSelected(m_setup.flags & XVDR::ChannelScannerSetup::FLAG_RADIO);
  m_flag_fta->SetSelected(m_setup.flags & XVDR::ChannelScannerSetup::FLAG_FTA);
  m_flag_encrypted->SetSelected(m_setup.flags & XVDR::ChannelScannerSetup::FLAG_SCRAMBLED);
  m_flag_hd->SetSelected(m_setup.flags & XVDR::ChannelScannerSetup::FLAG_HDTV);

  UpdateVisibility();

  return true;
}

bool CGUIDialogChannelScanner::OnClick(int controlId) {
  if(controlId == CTL_DVBTYPE) {
    UpdateVisibility();
  }

  if(controlId == CTL_START) {
    if(!m_scanning) {
      m_scanning = StartScan();
    }
    else if(m_connection->StopChannelScanner()) {
      m_scanning = false;
    }
    Close();
  }

  if(controlId == CTL_CANCEL) {
    Close();
  }

  return true;
}

void CGUIDialogChannelScanner::UpdateVisibility() {
  static int controls[3][3] = {
    {CTL_INVERSION_DVBT, 0, 0},
    {CTL_INVERSION_DVBC, CTL_SYMBOLRATE_DVBC, CTL_QAM_DVBC},
    {CTL_SATELLITE_DVBS, 0, 0}
  };

  for(int j = 0; j < 3; j++) {
    for(int i = 0; i < 3; i++) {
      CAddonGUISpinControl* c = m_gui->Control_getSpin(m_window, controls[j][i]);
      if(c != NULL) {
        c->SetVisible(m_dvbtype->GetValue() == j);
      }
    }
  }
}

bool CGUIDialogChannelScanner::StartScan() {
  m_setup.dvbtype = (XVDR::ChannelScannerSetup::DVBType)m_dvbtype->GetValue();
  m_setup.countryid = m_country->GetValue();
  m_setup.dvbt_inversion = (XVDR::ChannelScannerSetup::DVBInversion)m_inv_dvbt->GetValue();
  m_setup.dvbc_inversion = (XVDR::ChannelScannerSetup::DVBInversion)m_inv_dvbc->GetValue();
  m_setup.dvbc_symbolrate = (XVDR::ChannelScannerSetup::SymbolRate)m_symbolrate->GetValue();
  m_setup.dvbc_qam = (XVDR::ChannelScannerSetup::QAM)m_qam->GetValue();
  m_setup.satid = m_sat->GetValue();

  m_setup.flags = 0;
  if(m_flag_tv->IsSelected()) m_setup.flags |= XVDR::ChannelScannerSetup::FLAG_TV;
  if(m_flag_radio->IsSelected()) m_setup.flags |= XVDR::ChannelScannerSetup::FLAG_RADIO;
  if(m_flag_fta->IsSelected()) m_setup.flags |= XVDR::ChannelScannerSetup::FLAG_FTA;
  if(m_flag_encrypted->IsSelected()) m_setup.flags |= XVDR::ChannelScannerSetup::FLAG_SCRAMBLED;
  if(m_flag_hd->IsSelected()) m_setup.flags |= XVDR::ChannelScannerSetup::FLAG_HDTV;

  if(!m_connection->SetChannelScannerSetup(m_setup)) {
    return false;
  }

  return m_connection->StartChannelScanner();
}
