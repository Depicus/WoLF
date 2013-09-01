#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>  
#include <MemoryFree.h>

// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0xF2, 0x1E };    
EthernetClient client;
unsigned int localPort = 4343;      // local port to listen on
EthernetUDP Udp;
String packetString = "";
char ffAddress[32];
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; 
byte outpacketBuffer[102];

//byte SNTP_server_IP[]    = { 192, 43, 244, 18}; // time.nist.gov
//byte SNTP_server_IP[] = { 130,149,17,21};    // ntps1-0.cs.tu-berlin.de
byte SNTP_server_IP[] = { 
  192,53,103,108};   // ptbtime1.ptb.de
const  long timeZoneOffset = 0L; // set this to the offset in seconds to your local time;

void setup() {
  // start the Ethernet and UDP:
  Serial.begin(9600);
  Serial.println(F("Starting....."));
  Serial.println(freeMemory());
  Serial.println(F("Requesting DHCP Address."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }

  // print your local IP address:
  Serial.print(F("IP Address assigned: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(F(".")); 
  }
  Serial.println(F(""));
  Udp.begin(localPort);
  delay(500);
  Serial.println(F("Waiting for time sync"));
  setSyncProvider(getNtpTime);
  while(timeStatus() == timeNotSet)   
    ; // wait until the time is set
}

void loop() {
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    Serial.print(F("Received packet of size "));
    Serial.print(packetSize);
    Serial.print(F(" "));
    Serial.print(getCurrentTime());
    Serial.print(F("From "));
    IPAddress remote = Udp.remoteIP();
    for (int i =0; i < 4; i++)
    {
      Serial.print(remote[i], DEC);
      if (i < 3)
      {
        Serial.print(F("."));
      }
    }
    Serial.print(F(", port "));
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer

    packetString = "";
    int breakOnSix = 0;
    int packetNo = 0;
    memset(outpacketBuffer, 0, 102); 
    int sBuff = 0;
    while (Udp.available() > 0) {
      // read the packet into packetBufffer
      char udpByte = Udp.read();
      sprintf(ffAddress,"%02X",(unsigned char)udpByte);
      packetString = packetString + udpByte;
      outpacketBuffer[sBuff] = udpByte;
      sBuff +=1;
      Serial.print(ffAddress);
      if (breakOnSix < 5)
      {
        Serial.print( ":" );
      }
      packetNo += 1;
      breakOnSix += 1;
      if (breakOnSix > 5)
      {
        Serial.println();
        breakOnSix = 0;
      }

    } 
    Serial.println();
    Serial.println(freeMemory());
    if (packetSize == 102)
    {
      Serial.println(F("Sending reply to 255.255.255.255 port 999"));
      Udp.beginPacket("255.255.255.255", 999);
      Udp.write(outpacketBuffer,102);
      Udp.endPacket();
    }
  }
}

String getCurrentTime()
{
  char x[19];
  sprintf(x,"%02i:%02i:%02i Zulu Time",hour(),minute(),second());
  return x; 
}

/*-------- NTP code ----------*/

unsigned long getNtpTime()
{
  sendNTPpacket(SNTP_server_IP);
  delay(1000);
  if ( Udp.available() ) {
    for(int i=0; i < 40; i++)
      Udp.read(); // ignore every field except the time
    const unsigned long seventy_years = 2208988800UL + timeZoneOffset;        
    return getUlong() -  seventy_years;      
  }
  return 0; // return 0 if unable to get the time
}

unsigned long sendNTPpacket(byte *address)
{
  Udp.begin(123);
  Udp.beginPacket(address, 123);
  Udp.write(B11100011);   // LI, Version, Mode
  Udp.write(0,1);    // Stratum
  Udp.write(6);  // Polling Interval
  Udp.write(0xEC); // Peer Clock Precision
  write_n(0, 8);    // Root Delay & Root Dispersion
  Udp.write(49); 
  Udp.write(0x4E);
  Udp.write(49);
  Udp.write(52);
  write_n(0, 32); //Reference and time stamps  
  Udp.endPacket();   
}

unsigned long getUlong()
{
  unsigned long ulong = (unsigned long)Udp.read() << 24;
  ulong |= (unsigned long)Udp.read() << 16;
  ulong |= (unsigned long)Udp.read() << 8;
  ulong |= (unsigned long)Udp.read();
  return ulong;
}

void write_n(int what, int how_many)
{
  for( int i = 0; i < how_many; i++ )
    Udp.write(what);
}





