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
#include "consoleclient.h"
#include "xvdr/demux.h"
#include "xvdr/connection.h"

using namespace XVDR;

int main(int argc, char* argv[]) {
  std::string hostname = "192.168.16.10";
  int channel_number = 1;

  if(argc >= 2) {
    hostname = argv[1];
  }
  if(argc >= 3) {
    channel_number = atoi(argv[2]);
  }

  ConsoleClient client;

  if(!client.Open(hostname, "Demux test client")) {
    client.Log(FAILURE,"Unable to open connection !");
    return 1;
  }

  if(!client.EnableStatusInterface(true)) {
    client.Log(FAILURE,"Unable to enable status interface !");
    return 1;
  }

  client.Log(INFO, "Fetching channels ..");
  client.GetChannelsList();
  client.Log(INFO, "Got %i channels.", client.m_channels.size());

  Channel c = client.m_channels[channel_number];

  Demux demux(&client, NULL);

  TimeMs t;
  client.Log(INFO, "Opening channel #%i (%s)", channel_number, c.Name.c_str());

  if(demux.OpenChannel(hostname, c.UID) != Demux::SC_OK) {
    client.Log(FAILURE, "Unable to open channel !");
    return 1;
  }

  ConsoleClient::Packet* p = NULL;
  int firstVideoPacket = 0;
  int firstPacket = 0;
  int switchTime = t.Elapsed();

  client.Log(INFO, "Switched to channel after %i ms", switchTime);

  for(int i = 0; i < 100; i++) {
    p = demux.Read<ConsoleClient::Packet>();

    if(p == NULL) {
      break;
    }

    if(p->data != NULL) {
      if(firstPacket == 0) {
        firstPacket = t.Elapsed();
        client.Log(INFO, "Received first packet after %i ms", firstPacket);
      }
      if(firstVideoPacket == 0 && p->index == 0) {
        firstVideoPacket = t.Elapsed();
        client.Log(INFO, "Received first video packet after %i ms", firstVideoPacket);
      }
      uint32_t header = p->data[0] << 24 | p->data[1] << 16 | p->data[2] << 8 | p->data[3];
      client.Log(INFO, "Demux (index: %i length: %i bytes) Header: %08X PTS: %lli", p->index, p->length, header, p->pts);
    }

    client.FreePacket(p);
  }

  client.Log(INFO, "Stopping ...");
  t.Set(0);

  // wait for pending notifications
  if(p == NULL) {
    CondWait::SleepMs(5000);
  }

  demux.CloseChannel();
  client.Close();

  int stopTime = t.Elapsed();

  client.Log(INFO, "");
  client.Log(INFO, "Stream summary:");
  client.Log(INFO, "Channel: %i - %s", channel_number, c.Name.c_str());
  client.Log(INFO, "Switch time: %i ms", switchTime);
  client.Log(INFO, "First packet after: %i ms", firstPacket);
  client.Log(INFO, "First video after: %i ms", firstVideoPacket);

  return 0;
}
