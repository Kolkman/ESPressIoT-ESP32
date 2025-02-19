//
// ESPressIoT Controller for Espresso Machines
// 2016-2021 by Roman Schmitz
//
// Telnet Server
//

#ifdef ENABLE_TELNET

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void telnetStatus() {
  telnetClient.println(gStatusAsJson);
}

void setupTelnet() {
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  Serial.println("Please connect Telnet Client, exit with ^] and 'quit'");

  Serial.print("Free Heap[B]: ");
  Serial.println(ESP.getFreeHeap());
}

void loopTelnet() {
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient) telnetClient.stop();
      telnetClient = telnetServer.available();
    } else {
      telnetServer.available().stop();
    }
  }
  if (telnetClient && telnetClient.connected() && telnetClient.available()) {
    telnetStatus();
  }
}

#endif
