#include <stdlib.h>
#include <unistd.h>

#include "consoleclient.h"
#include "xvdr/demux.h"
#include "xvdr/connection.h"

using namespace XVDR;

int main(int argc, char* argv[]) {
  std::string hostname = "192.168.16.10";

  if(argc >= 2) {
    hostname = argv[1];
  }

  ConsoleClient client;

  if(!client.Open(hostname, "listener client")) {
    client.Log(FAILURE,"Unable to open connection !");
    return 1;
  }

  if(!client.EnableStatusInterface(true)) {
    client.Log(FAILURE,"Unable to enable status interface !");
    return 1;
  }

  CondWait::SleepMs(0);

  client.Log(INFO, "Shutting down ..");
  client.Close();

  client.Log(INFO, "Done.");
  return 0;
}
