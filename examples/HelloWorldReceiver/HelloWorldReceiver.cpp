#include "RFReceiver.h"

#include <iostream>

// need pigpio package
// Compile with > g++ -o HelloWorldReceiver HelloWorldReceiver.cpp RFReceiver.cpp -lpigpio
// Launch with > sudo ./HelloWorldReceiver

const uint8_t   GPIO        = 27; // Pin 13
const uint16_t  PULSELENGTH = 200;

int main() {
  RFReceiver receiver(GPIO, PULSELENGTH);

  if (receiver.begin(GPIO) < 0) {
    std::cerr << "Initialize failed ;(\n";
    return -1;
  }

  char msg[MAX_PACKAGE_SIZE];
  uint8_t senderId = 0;
  uint8_t packageId = 0;

  uint8_t len = receiver.recvPackage((uint8_t *)msg, &senderId, &packageId);

  receiver.stop();

  fprintf(stdout, "Package: %u\n", packageId);
  fprintf(stdout, "Sender: %u\n", senderId);
  fprintf(stdout, "Message: %s\n", msg);

  return 0;
}
