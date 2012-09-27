/*
 *      Copyright (C) 2012 Alexander Pipelka
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "xvdr/dataset.h"

using namespace XVDR;

bool XVDR::operator==(Stream const& lhs, Stream const& rhs) {
	return lhs.equals(rhs);
}

bool operator==(DatasetItem const& lhs, DatasetItem const& rhs) {
	return ((std::string)lhs == (std::string)rhs);
}

template<> void DatasetItem::operator=(std::string v) {
	m_value = v;
}

template<> void DatasetItem::operator=(const char* v) {
	m_value = v;
}
