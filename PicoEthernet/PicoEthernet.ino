/*
 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see https://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi

 This code is in the public domain.

 */

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include "TickTwo.h" //library to handle timing processes
#include <ACAN2515.h>


byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED  // sets MAC address of MCU
};
IPAddress ip(192, 168, 1, 3);       // sets the IP of the MCU

unsigned int localPort = 1003;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";        // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// CAN pic configs
static const byte MCP2515_SCK  = 10 ; // SCK input of MCP2515
static const byte MCP2515_MOSI = 11; // SDI input of MCP2515
static const byte MCP2515_MISO = 8 ; // SDO output of MCP2515

static const byte MCP2515_CS  = 9 ;  // CS input of MCP2515
static const byte MCP2515_INT = 7 ;  // INT output of MCP2515

ACAN2515 can (MCP2515_CS, SPI1, MCP2515_INT) ; // CAN Uses SPI 1
static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL ; // 8 MHz 

CANMessage frame; //buffer for CAN frame

void configureCan(); 
void configureEthernet();
void printCanMsg();


void setup() {

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  configureEthernet();
  configureCan();
}

void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i=0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBuffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packetBuffer);


    if(packetBuffer[0] == 1){
      Serial.println("Address: 1");
    }
    if (packetBuffer[1] == 65) {
      Serial.println("Contents:");
      Serial.println("65 received via UDP!");
    }

    // send a reply to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
  delay(10);
}

void configureCan(){
  SPI1.setSCK (MCP2515_SCK);
  SPI1.setTX (MCP2515_MOSI);
  SPI1.setRX (MCP2515_MISO);
  SPI1.setCS (MCP2515_CS); 
  SPI1.begin();


  Serial.println ("Configure ACAN2515") ;
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 100UL * 1000UL) ; // CAN bit rate 100 kb/s
  const uint16_t errorCode = can.begin (settings,  canISR) ;
  if(errorCode == 0){
    Serial.print ("Actual bit rate: ") ;
    Serial.print (settings.actualBitRate ()) ;
  }


} 

void configureEthernet(){
  //init pin on Ethernet Shield
  Ethernet.init(17);

  // start the Ethernet
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(localPort);
}

//handle and updates values when/if a can message is recieved
void canISR(){
  can.isr();
  can.receive(frame);

  printCanMsg(frame);

}

void printCanMsg(CANMessage frame){
    Serial.print ("  id: ");Serial.println (frame.id,HEX);
    Serial.print ("  ext: ");Serial.println (frame.ext);
    Serial.print ("  rtr: ");Serial.println (frame.rtr);
    Serial.print ("  len: ");Serial.println (frame.len);
    Serial.print ("  data: ");
    for(int x=0;x<frame.len;x++) {
      Serial.print (frame.data[x],HEX); Serial.print(":");
    }
    Serial.println ("");
}

