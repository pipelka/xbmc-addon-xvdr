#pragma once
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

#include "XBMCAddon.h"
#include "XVDRCallbacks.h"
#include <list>
#include <vector>
#include <string>

class cXBMCConfigParameterBase
{
public:

  cXBMCConfigParameterBase();

  virtual ~cXBMCConfigParameterBase();

  virtual bool load() = 0;

  virtual bool set(const void* value) = 0;

  std::string& name();

protected:

  std::string m_setting;

  static std::list<cXBMCConfigParameterBase*>& parameters();

  friend class cXBMCSettings;

private:

  static std::list<cXBMCConfigParameterBase*> m_parameters;

};


template< class T >
class cXBMCConfigParameter : public cXBMCConfigParameterBase
{
public:

  cXBMCConfigParameter(const std::string& setting, T defaultvalue) : m_value(defaultvalue), m_default(defaultvalue)
  {
    m_setting = setting;
  }

  cXBMCConfigParameter(const std::string& setting)
  {
    m_setting = setting;
  }

  bool load()
  {
    if (XBMC->GetSetting(m_setting.c_str(), &m_value))
      return true;

    XVDRLog(XVDR_ERROR, "Couldn't get '%s' setting, falling back to default", m_setting.c_str());
    m_value = m_default;

    return true;
  }

  bool set(const void* value)
  {
    return set(*(T*)value);
  }

  bool set(T value)
  {
    if(m_value == value)
      return false;

    XVDRLog(XVDR_INFO, "Changed Setting '%s'", m_setting.c_str());
    m_value = value;
    return true;
  }

  T& operator()()
  {
    return m_value;
  }

private:

  T m_value;

  T m_default;

};

class cXBMCSettings
{
public:

  virtual ~cXBMCSettings();

  static cXBMCSettings& GetInstance();

  void load();

  bool set(const std::string& setting, const void* value);

  void checkValues();

  cXBMCConfigParameter<std::string> Hostname;
  cXBMCConfigParameter<int> ConnectTimeout;
  cXBMCConfigParameter<bool> HandleMessages;
  cXBMCConfigParameter<int> Priority;
  cXBMCConfigParameter<int> Compression;
  cXBMCConfigParameter<bool> AutoChannelGroups;
  cXBMCConfigParameter<int> AudioType;
  cXBMCConfigParameter<int> UpdateChannels;
  cXBMCConfigParameter<bool> FTAChannels;
  cXBMCConfigParameter<bool> NativeLangOnly;
  cXBMCConfigParameter<bool> EncryptedChannels;
  cXBMCConfigParameter<std::string> caids;
  std::vector<int> vcaids;

protected:

  cXBMCSettings() :
  Hostname("host", "127.0.0,1"),
  ConnectTimeout("timeout", 3),
  HandleMessages("handlemessages", true),
  Priority("priority", 50),
  Compression("compression", 2),
  AutoChannelGroups("autochannelgroups", false),
  AudioType("audiotype", 0),
  UpdateChannels("updatechannels", 3),
  FTAChannels("ftachannels", true),
  NativeLangOnly("nativelangonly", false),
  EncryptedChannels("encryptedchannels", true),
  caids("caids")
  {}

private:

  void ReadCaIDs(const char* buffer, std::vector<int>& array);

};
