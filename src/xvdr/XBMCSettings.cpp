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

#include "XBMCSettings.h"
#include <algorithm>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

std::list<cXBMCConfigParameterBase*> cXBMCConfigParameterBase::m_parameters;


cXBMCConfigParameterBase::cXBMCConfigParameterBase()
{
  m_parameters.push_back(this);
}

cXBMCConfigParameterBase::~cXBMCConfigParameterBase()
{
  std::list<cXBMCConfigParameterBase*>::iterator i = std::find(m_parameters.begin(), m_parameters.end(), this);

  if(i != m_parameters.end())
    m_parameters.erase(i);
}

std::list<cXBMCConfigParameterBase*>& cXBMCConfigParameterBase::parameters()
{
  return m_parameters;
}

std::string& cXBMCConfigParameterBase::name()
{
  return m_setting;
}


cXBMCSettings::~cXBMCSettings()
{
}

cXBMCSettings& cXBMCSettings::GetInstance()
{
  static cXBMCSettings singleton;
  return singleton;
}

bool cXBMCSettings::set(const std::string& setting, const void* value)
{
  std::list<cXBMCConfigParameterBase*>& list = cXBMCConfigParameterBase::parameters();
  std::list<cXBMCConfigParameterBase*>::iterator i;

  for(i = list.begin(); i != list.end(); i++)
  {
    if((*i)->name() == setting)
      return (*i)->set(value);
  }

  return false;
}

void cXBMCSettings::checkValues()
{
  ReadCaIDs(caids().c_str(), vcaids);

  if (!EncryptedChannels())
  {
    vcaids.clear();
    vcaids.push_back(0xFFFF); // disable encrypted channels by invalid caid
  }

  // check priority setting (and set a sane value)
  if(Priority() > 21)
    Priority.set(10);
}

void cXBMCSettings::load()
{
  std::list<cXBMCConfigParameterBase*>& list = cXBMCConfigParameterBase::parameters();
  std::list<cXBMCConfigParameterBase*>::iterator i;

  for(i = list.begin(); i != list.end(); i++)
    (*i)->load();

  checkValues();
}

void cXBMCSettings::ReadCaIDs(const char* buffer, std::vector<int>& array)
{
  array.clear();
  char* p = strdup(buffer);
  char* s = p;

  for(;;)
  {
    char* n = strpbrk(p, ",;/");
    if(n != NULL) *n = 0;
    uint32_t caid = 0;
    sscanf(p, "%04x", &caid);

    if(caid != 0)
      array.push_back(caid);

    if(n == NULL)
      break;

    p = ++n;
  }
  free(s);
}


template<>
bool cXBMCConfigParameter<std::string>::load()
{
  char buffer[512];
  if (XBMC->GetSetting(m_setting.c_str(), buffer))
  {
    m_value = buffer;
    return true;
  }

  XVDRLog(XVDR_ERROR, "Couldn't get '%s' setting, falling back to default", m_setting.c_str());
  m_value = m_default;

  return true;
}

template<>
bool cXBMCConfigParameter<std::string>::set(const void* value)
{
  const char* str = (const char*)value;
  if(strcmp(str, m_value.c_str()) == 0)
    return false;

  XVDRLog(XVDR_INFO, "Changed Setting '%s'", m_setting.c_str());
  m_value = str;
  return true;
}
