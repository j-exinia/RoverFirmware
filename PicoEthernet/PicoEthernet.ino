#include <Ethernet.h>
#include <EthernetUdp.h>
#include "TickTwo.h" //library to handle timing processes
#include <ACAN2515.h>


byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED  // sets MAC address of MCU
};
IPAddress ip(192, 168, 1, 3);       // sets the IP of the MCU

unsigned int localPort = 1003;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer0[] = "BASE SENT";        // a string to send back
char ReplyBuffer1[] = "ARM SENT"; 
char ReplyBuffer2[] = "CLAW SENT"; 

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
CANMessage outFrame; //out CAN message

uint32_t BASE_OUT_ID = 0;
uint32_t BASE_OUT_LEN = 6;

uint32_t ARM_OUT_ID = 1;
uint32_t ARM_OUT_LEN = 3;

uint32_t CLAW_OUT_ID = 2;
uint32_t CLAW_OUT_LEN = 3;

uint8_t testy[6] = {255,255,25,2,5,0};

static bool gSendMessage = false ;

uint32_t msgSent = 0;


void configureCan(); 
void configureEthernet();
void printCanMsg(CANMessage &frame);
void onCanRecieve();
bool readEthernet();
void sendAckEthernet(int msg);


void setup() {

  Serial.begin(9600);
  while (!Serial) {
  ; // wait for serial port to connect. Needed for native USB port only
  }
  configureCan();
  configureEthernet();
}

void loop() {
  // if there's data available, read a packet

  //read ETHERNET msg
  // outFrame.id = BASE_OUT_ID;
  // outFrame.len = BASE_OUT_LEN;
  // memcpy(outFrame.data, testy, 6);
  // sendCanMsg();

  delay(750);
  int packetSize = Udp.parsePacket();
  if (packetSize) {
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
    
    switch(packetBuffer[0]){
      case 0xff:
        Serial.println("UPD Address: 0xff");
        outFrame.id = BASE_OUT_ID;
        outFrame.len = BASE_OUT_LEN;
        memcpy(outFrame.data, packetBuffer+1, 6);
        sendCanMsg();
        sendAckEthernet(0);
        break;
      case 0x01:
        Serial.println("UPD Address: 0x01");
        outFrame.id = ARM_OUT_ID;
        outFrame.len = ARM_OUT_LEN;
        memcpy(outFrame.data, packetBuffer+1, 3);
        sendCanMsg();
        sendAckEthernet(1);
        break;
      case 0x02:
        Serial.println("UDP Address: 0x02");
        outFrame.id = CLAW_OUT_ID;
        outFrame.len = CLAW_OUT_LEN;
        memcpy(outFrame.data, packetBuffer+1, 3);
        sendCanMsg();
        sendAckEthernet(2);
        break;
    }
  }
}

void configureCan(){
  SPI1.setSCK (MCP2515_SCK);
  SPI1.setTX (MCP2515_MOSI);
  SPI1.setRX (MCP2515_MISO);
  SPI1.setCS (MCP2515_CS); 
  SPI1.begin();


  Serial.println ("Configure ACAN2515") ;
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 100UL * 1000UL) ; // CAN bit rate 100 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode ; // Select Normal mode
  const uint16_t errorCode = can.begin (settings,  onCanRecieve) ;
  if(errorCode == 0){
    Serial.print ("Actual bit rate: ") ;
    Serial.print (settings.actualBitRate ()) ;
  }
  else{
    Serial.print(errorCode);
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
void onCanRecieve(){
  can.isr();
  can.receive(frame);
  printCanMsg(frame);

}

void printCanMsg(CANMessage &frame){
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

void sendCanMsg(){
  const bool ok = can.tryToSend (outFrame) ;
  const uint16_t n = can.transmitBufferCount (0);
  if(ok){
    Serial.println ("SENT CAN") ;
    //printCanMsg(outFrame);
    msgSent = msgSent+1;
  }
  else{
    Serial.print("Num of messages b4 fail:");Serial.println(msgSent);
    Serial.print("Num of Buff GARFs:");Serial.println(n);
    Serial.println ("Send failure") ;
  }
}

void sendAckEthernet(int msg){
  if(msg = 0){
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer0);
    Udp.endPacket();
  }
  if(msg = 1){
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer1);
    Udp.endPacket();
    
  }
  if(msg = 2){
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer2);
    Udp.endPacket();
  }

}


