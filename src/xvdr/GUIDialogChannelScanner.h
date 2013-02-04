#pragma once
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

#include "addons/library.xbmc.gui/libXBMC_gui.h"
#include "xvdr/connection.h"

class CGUIDialogChannelScanner {
public:

  CGUIDialogChannelScanner(CHelper_libXBMC_gui* GUI, XVDR::Connection* connection);

  virtual ~CGUIDialogChannelScanner();

  void Close();

  void DoModal();

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);

protected:

  virtual bool OnInit();

  virtual bool OnClick(int controlId);

  virtual bool OnFocus(int controlId);

  virtual void OnClose();

  void UpdateVisibility();

  CAddonGUIWindow* m_window;
  CHelper_libXBMC_gui* m_gui;
  XVDR::Connection* m_connection;
  XVDR::ChannelScannerSetup m_setup;
  XVDR::ChannelScannerList m_satellites;
  XVDR::ChannelScannerList m_countries;

private:

  void StartScan();

  CAddonGUISpinControl* m_dvbtype;
  CAddonGUISpinControl* m_country;
  CAddonGUISpinControl* m_inv_dvbt;
  CAddonGUISpinControl* m_inv_dvbc;
  CAddonGUISpinControl* m_symbolrate;
  CAddonGUISpinControl* m_qam;
  CAddonGUISpinControl* m_sat;
  CAddonGUIRadioButton* m_flag_tv;
  CAddonGUIRadioButton* m_flag_radio;
  CAddonGUIRadioButton* m_flag_fta;
  CAddonGUIRadioButton* m_flag_encrypted;
  CAddonGUIRadioButton* m_flag_hd;
};
