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

#include "GUIDialogBase.h"

namespace XVDR {

class CGUIDialogOk : public virtual CGUIDialogBase {
public:

  CGUIDialogOk(CHelper_libKODI_guilib* GUI, const char* heading, const char* line0, const char* line1 = NULL, const char* line2 = NULL);

  virtual ~CGUIDialogOk();

  void SetLabels(const char* heading, const char* line0, const char* line1 = NULL, const char* line2 = NULL);

  static void Show(CHelper_libKODI_guilib* GUI, const char* heading, const char* line0, const char* line1 = NULL, const char* line2 = NULL);

protected:

  bool OnInit();

  bool OnClick(int controlId);

private:

  const char* m_heading;
  const char* m_line[3];

};

} // namespace XVDR
