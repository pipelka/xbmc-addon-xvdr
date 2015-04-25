#pragma once
/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2012 Alexander Pipelka
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

#include "addons/library.xbmc.addon/libXBMC_addon.h"
#include "addons/library.kodi.guilib/libKODI_guilib.h"
#include "addons/library.xbmc.pvr/libXBMC_pvr.h"
#include "addons/library.xbmc.codec/libXBMC_codec.h"

extern ADDON::CHelper_libXBMC_addon *XBMC;
extern CHelper_libKODI_guilib *GUI;
extern CHelper_libXBMC_pvr *PVR;
extern CHelper_libXBMC_codec *CODEC;
