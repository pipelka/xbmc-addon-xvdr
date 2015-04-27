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

#include "GUIDialogYesNo.h"

using namespace XVDR;

CGUIDialogYesNo::CGUIDialogYesNo(CHelper_libKODI_guilib* GUI, const char* heading, const char* line0, const char* line1, const char* line2) : CGUIDialogOk(GUI, heading, line0, line1, line2), CGUIDialogBase("yesno.xml", GUI), m_status(false) {
}

CGUIDialogYesNo::~CGUIDialogYesNo() {
}

bool CGUIDialogYesNo::GetStatus() {
  return m_status;
}

bool CGUIDialogYesNo::ShowAndGetInput(CHelper_libKODI_guilib* GUI, const char* heading, const char* line0, const char* line1, const char* line2) {
  CGUIDialogYesNo dialog(GUI, heading, line0, line1, line2);
  dialog.DoModal();

  return dialog.GetStatus();
}

bool CGUIDialogYesNo::OnClick(int controlId) {
  if(controlId != 11 && controlId != 10) {
    return false;
  }

  m_status = (controlId == 11);
  Close();

  return true;
}
