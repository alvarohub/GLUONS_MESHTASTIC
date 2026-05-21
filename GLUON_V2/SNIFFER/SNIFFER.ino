// *************  SNIFFER MODULE   ***********
#include <SPI.h>
#include <RFM69.h>  // NOTE: the module used in this first batch is RFM69W (433MHz)
#include <RFM69_ATC.h> // NOTE: if the radio object is an instance of RFM69_ATC class, it will be able to adjust the
// emitting power when using sendWithRetry function.

#include "moduleRF.h"
#include "utils.h"

#define SNIFFER_ID 254

void setup() {
  Serial.begin(57600);
  myRF.init(SNIFFER_ID);

}

void loop() {

  // Receive RF data (sniffed packets)
  if ( myRF.updateReceive() ) {

    Serial.println(myRF.rawstring);

  }

    // Process SERIAL COMMANDS and send RF commands to all the gluons in the newtork
  processSerialMessage();
 
}


void processSerialMessage() {
  // very simple commands for the time being:
  while (Serial.available()) {
    char val = Serial.read();

    switch (val) {
    case '0': // deactivate sniffer mode on ALL the possible gluons on the network (we will send a broadcast message (ID 255):
      myRF.getSenderMessage().setType(SNIFFER_MODE_OFF);
      myRF.getSenderMessage().setReceiverId(255); // BROADCAST!
      myRF.send();
      break;
    case '1': // activate sniffer mode on all the gluons:
      myRF.getSenderMessage().setType(SNIFFER_MODE_ON);
      myRF.getSenderMessage().setReceiverId(255); // BROADCAST!
      myRF.send();
      break;
    case '2': // activate sniffer mode VERBOSE on all the gluons:
      myRF.getSenderMessage().setType(SNIFFER_MODE_ON_VERBOSE);
      myRF.getSenderMessage().setReceiverId(255); // BROADCAST!
      myRF.send();
      break;

  case '3': // SYNCH timestamps on all gluons
      myRF.getSenderMessage().setType(SYNCH_CLK);
      myRF.getSenderMessage().setReceiverId(255); // BROADCAST!
      myRF.send();
      break;

  case '4': // Perform NETWORK SCAN
      myRF.getSenderMessage().setType(SCAN_NET);
      myRF.getSenderMessage().setReceiverId(255); // BROADCAST!
      myRF.send();
      break;

  case '5': // Disconnect EVERYTHING
      myRF.getSenderMessage().setType(RESET_CON);
      myRF.getSenderMessage().setReceiverId(255); // BROADCAST!
      myRF.send();
      break;
      
    default:
      //...
      break;
    }
  }
}
