/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
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

#include <stdlib.h>
#include <unistd.h>

#include "consoleclient.h"
#include "xvdr/demux.h"
#include "xvdr/connection.h"
#include "xvdr/msgpacket.h"

using namespace XVDR;

int main(int argc, char* argv[]) {
  std::string hostname = "192.168.16.10";

  if(argc >= 2) {
    hostname = argv[1];
  }

  ConsoleClient client;

  if(!client.Open(hostname, "scanner client")) {
    client.Log(FAILURE,"Unable to open connection !");
    return 1;
  }

  if(!client.EnableStatusInterface(true)) {
    client.Log(FAILURE,"Unable to enable status interface !");
    return 1;
  }

  if(client.SupportChannelScan()) {
    client.Log(INFO, "channel scanning supported");
  }
  else {
    client.Log(INFO, "channel scanning *NOT* supported");
    client.Close();
    return 0;
  }

  // get setup
  ChannelScannerSetup setup;
  ChannelScannerList sat;
  ChannelScannerList countries;

  if(!client.GetChannelScannerSetup(setup, sat, countries)) {
    return 1;
  }

  client.Log(INFO, "Current scanner setup:");
  client.Log(INFO, "---------------------------------");
  client.Log(INFO, "Verbosity: %i", setup.verbosity);
  client.Log(INFO, "Logfile: %i", setup.logtype);
  client.Log(INFO, "DVB Type: %i", setup.dvbtype);
  client.Log(INFO, "DVBT Inversion: %i", setup.dvbt_inversion);
  client.Log(INFO, "DVBC Inversion: %i", setup.dvbc_inversion);
  client.Log(INFO, "DVBC Symbolrate: %i", setup.dvbc_symbolrate);
  client.Log(INFO, "DVBC QAM: %i", setup.dvbc_qam);
  client.Log(INFO, "Country ID: %i", setup.countryid);
  client.Log(INFO, "Sat ID: %i", setup.satid);
  client.Log(INFO, "Scanflags: %04x", setup.flags);
  client.Log(INFO, "ATSC Type: %i", setup.atsc_type);
  client.Log(INFO, "---------------------------------");

  client.Log(INFO, "Satellites:");
  client.Log(INFO, "---------------------------------");

  for(ChannelScannerList::iterator i = sat.begin(); i != sat.end(); i++) {
    client.Log(INFO, "%06i|%8s|%s", i->first, i->second.shortname.c_str(), i->second.fullname.c_str());
  }

  client.Log(INFO, "Countries:");
  client.Log(INFO, "---------------------------------");

  for(ChannelScannerList::iterator i = countries.begin(); i != countries.end(); i++) {
    client.Log(INFO, "%06i|%8s|%s", i->first, i->second.shortname.c_str(), i->second.fullname.c_str());
  }

  // modify setup and send back
  setup.verbosity = ChannelScannerSetup::LOGLEVEL_DEFAULT;
  setup.countryid = 14; // AUSTRIA
  setup.logtype = ChannelScannerSetup::LOGTYPE_SYSLOG;
  setup.dvbtype = ChannelScannerSetup::DVBTYPE_DVBT;
  setup.flags = ChannelScannerSetup::FLAG_TV | ChannelScannerSetup::FLAG_FTA;

  if(!client.SetChannelScannerSetup(setup)) {
    client.Log(FAILURE, "Unable to set setup parameters !");
  }

  client.StartChannelScanner();

  CondWait::SleepMs(0);

  client.Close();
  return 0;
}
