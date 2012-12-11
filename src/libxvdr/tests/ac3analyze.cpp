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
  Connection xvdr(&client);

  if(!xvdr.Open(hostname, "Demux test client")) {
    client.Log(FAILURE,"Unable to open connection !");
    return 1;
  }

  client.Log(INFO, "Fetching channels ..");
  xvdr.GetChannelsList();
  client.Log(INFO, "Got %i channels.", client.m_channels.size());

  Channel c = client.m_channels[channel_number];

  Demux demux(&client);

  client.Log(INFO, "Opening channel #%i", channel_number);

  if(demux.OpenChannel(hostname, c.UID) != Demux::SC_OK) {
    client.Log(FAILURE, "Unable to open channel !");
    return 1;
  }

  int ac3index = -1;
  uint32_t bitrate = 0;
  bool bFound = false;

  client.Log(INFO, "Waiting for stream properties ...");
  while(!bFound) {
    ConsoleClient::Packet* p = demux.Read<ConsoleClient::Packet>();
    client.FreePacket(p);

    // find AC3 stream

    XVDR::StreamProperties props = demux.GetStreamProperties();
    for(XVDR::StreamProperties::iterator i = props.begin(); i!= props.end(); i++) {
      if(i->second.Type == "AC3" && i->second.BitRate != 0) {
        ac3index = i->second.Index;
        bitrate = i->second.BitRate;
        client.Log(INFO, "Bitrate: %i", bitrate);
        bFound = true;
        break;
      }
    }
  }

  if(ac3index == -1) {
    client.Log(FAILURE, "AC3 stream not found !");
    demux.CloseChannel();
    xvdr.Close();
    return 1;
  }

  client.Log(INFO, "Analyzing packets (Stream #%i) ...", ac3index);

  int64_t pts = 0;
  int64_t last_pts = 0;
  double pts_diff_ms = 0;
  double gap = 0;

  for(int i = 0; i < 500; i++) {
    ConsoleClient::Packet* p = demux.Read<ConsoleClient::Packet>();

    if(p->data == NULL || p->index != ac3index)
    {
      client.FreePacket(p);
      continue;
    }

    pts = p->pts;
    if(last_pts == 0)
    {
      client.FreePacket(p);
      last_pts = pts;
      continue;
    }

    pts_diff_ms = (double)(pts - last_pts) / 1000.0;
    double audio_duration_ms = (double)(p->length * 8 * 1000) / (double)bitrate;

    gap += pts_diff_ms;
    gap -= audio_duration_ms;

    client.Log(INFO, "PTS: %lli / Difference: %.1f ms Audio Duration: %.1f ms / Gap: %.1f ms", pts, pts_diff_ms, audio_duration_ms, gap);
    last_pts = pts;

    client.FreePacket(p);
  }

  demux.CloseChannel();

  xvdr.Close();

  return 0;
}
