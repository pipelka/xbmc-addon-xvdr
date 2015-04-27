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

#include "GUIDialogOk.h"

using namespace XVDR;

CGUIDialogOk::CGUIDialogOk(CHelper_libKODI_guilib* GUI, const char* heading, const char* line0, const char* line1, const char* line2) : CGUIDialogBase("yesno.xml", GUI) {
  m_heading = heading;
  m_line[0] = line0;
  m_line[1] = line1;
  m_line[2] = line2;
}

CGUIDialogOk::~CGUIDialogOk() {
}

bool CGUIDialogOk::OnInit() {
  SetLabels(m_heading, m_line[0], m_line[1], m_line[2]);
  return true;
}

bool CGUIDialogOk::OnClick(int controlId) {
  if(controlId == 10) {
    Close();
  }

  return true;
}

void CGUIDialogOk::SetLabels(const char* heading, const char* line0, const char* line1, const char* line2) {
  if(heading == NULL) heading = "";
  if(line0 == NULL) line0 = "";
  if(line1 == NULL) line1 = "";
  if(line2 == NULL) line2 = "";

  SetControlLabel(101, heading);
  SetControlLabel(102, line0);
  SetControlLabel(103, line1);
  SetControlLabel(104, line2);
}

void CGUIDialogOk::Show(CHelper_libKODI_guilib* GUI, const char* heading, const char* line0, const char* line1, const char* line2) {
  CGUIDialogOk dialog(GUI, heading, line0, line1, line2);
  dialog.DoModal();
}
