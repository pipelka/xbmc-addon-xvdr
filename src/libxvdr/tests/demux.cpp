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

  Demux demux(&client);

  client.Log(INFO, "Opening channel #%i", channel_number);

  if(demux.OpenChannel(hostname, c.UID) != Demux::SC_OK) {
    client.Log(FAILURE, "Unable to open channel !");
    return 1;
  }

  for(int i = 0; i < 100; i++) {
    ConsoleClient::Packet* p = demux.Read<ConsoleClient::Packet>();
    if(p->data != NULL) {
      uint32_t header = p->data[0] << 24 | p->data[1] << 16 | p->data[2] << 8 | p->data[3];
      client.Log(INFO, "Demux (index: %i length: %i bytes) Header: %08X PTS: %lli", p->index, p->length, header, p->pts);
    }
    client.FreePacket(p);
  }

  demux.CloseChannel();

  client.Close();

  return 0;
}
