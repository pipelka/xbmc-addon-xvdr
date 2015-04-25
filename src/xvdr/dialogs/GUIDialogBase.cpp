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

#include "GUIDialogBase.h"

using namespace XVDR;

CGUIDialogBase::CGUIDialogBase(const char* xml,  CHelper_libKODI_guilib* GUI) : m_gui(GUI) {
  m_window = m_gui->Window_create(xml, "skin.confluence", false, true);

  m_window->m_cbhdl   = this;
  m_window->CBOnInit  = OnInitCB;
  m_window->CBOnFocus = OnFocusCB;
  m_window->CBOnClick = OnClickCB;
  m_window->CBOnAction= OnActionCB;
}

CGUIDialogBase::~CGUIDialogBase(){
  m_gui->Window_destroy(m_window);
}

bool CGUIDialogBase::OnClickCB(GUIHANDLE cbhdl, int controlId) {
  CGUIDialogBase* dialog = static_cast<CGUIDialogBase*>(cbhdl);
  return dialog->OnClick(controlId);
}

bool CGUIDialogBase::OnFocusCB(GUIHANDLE cbhdl, int controlId) {
  CGUIDialogBase* dialog = static_cast<CGUIDialogBase*>(cbhdl);
  return dialog->OnFocus(controlId);
}

bool CGUIDialogBase::OnInitCB(GUIHANDLE cbhdl) {
  CGUIDialogBase* dialog = static_cast<CGUIDialogBase*>(cbhdl);
  return dialog->OnInit();
}

bool CGUIDialogBase::OnActionCB(GUIHANDLE cbhdl, int actionId) {
  CGUIDialogBase* dialog = static_cast<CGUIDialogBase*>(cbhdl);
  return dialog->OnAction(actionId);
}

void CGUIDialogBase::SetControlLabel(int controlId, const char* label) {
  m_window->SetControlLabel(controlId, label);
}

void CGUIDialogBase::Show() {
  m_window->Show();
}

void CGUIDialogBase::Close() {
  OnClose();
  m_window->Close();
}

void CGUIDialogBase::DoModal() {
  m_window->DoModal();
}

void CGUIDialogBase::OnClose() {
}

bool CGUIDialogBase::OnFocus(int controlId) {
  return true;
}

bool CGUIDialogBase::OnInit() {
  return true;
}

bool CGUIDialogBase::OnClick(int controlId) {
  return false;
}

bool CGUIDialogBase::OnAction(int actionId) {
  if (actionId == ADDON_ACTION_CLOSE_DIALOG || actionId == ADDON_ACTION_PREVIOUS_MENU) {
    Close();
    return true;
  }
  return false;
}
