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

#include "addons/library.kodi.guilib/libKODI_guilib.h"

namespace XVDR {

class CGUIDialogBase {
public:

  CGUIDialogBase(const char* xml, CHelper_libKODI_guilib* GUI);

  virtual ~CGUIDialogBase();

  void Show();

  void Close();

  void DoModal();

  void SetControlLabel(int controlId, const char* label);

protected:

  virtual bool OnInit();

  virtual bool OnClick(int controlId);

  virtual bool OnFocus(int controlId);

  virtual bool OnAction(int actionId);

  virtual void OnClose();

  CAddonGUIWindow* m_window;

  CHelper_libKODI_guilib* m_gui;

private:

  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);

  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);

  static bool OnInitCB(GUIHANDLE cbhdl);

  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);
};

} // namespace XVDR
