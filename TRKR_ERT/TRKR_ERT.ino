#include <TinyGPS++.h>

#define GPS_PORT Serial5
#define GPS_BAUD 57600
#define GPS_FREQ 1 

#define COM_PORT Serial
#define COM_BAUD 57600

#define DEBUG true 

TinyGPSPlus gps;
unsigned char serial2bufferRead[1000]; // Additional memory for GPS port 

double target_lat = 0;
double target_lon = 0;
double target_alt = 0;

struct coordinate {
  double latitude;
  double longitude;
  double alt;
};

struct cmd {
  double azm;
  double elv;
};

String buff = ""; // Stores the target data sent by computer or human 

void setup() {
  Serial.begin(115200);
  COM_PORT.begin(COM_BAUD);
  gps_setup(GPS_BAUD, GPS_FREQ, 2, 1, 0); // baud, Hz, mode, nmea, cog filter (0 = Off, 1 = On)
}

void loop() {
  // If new base or target GPS position
  if (parse_target() or (parse_gps() and !DEBUG)) {
    send_cmd(cmpt_cmd(get_base(), get_target()));
  } 
}

cmd cmpt_cmd(coordinate base, coordinate target) {
  cmd result; 

  double d_h = target.alt-base.alt;
  double dist = distance(base, target);

  result.azm = azimuth(base,target);
  result.elv = atan2(d_h, dist)/PI*180;

  return result;
}

double azimuth(coordinate base, coordinate target) {                                              
  return TinyGPSPlus::courseTo(base.latitude, base.longitude, target.latitude, target.longitude);                                      
}

double distance(coordinate base, coordinate target) {
  return TinyGPSPlus::distanceBetween(base.latitude, base.longitude, target.latitude, target.longitude);
}

void set_target(coordinate new_target) {
  target_lat = new_target.latitude;
  target_lon = new_target.longitude; 
  target_alt = new_target.alt;
}

coordinate get_target() {
  coordinate new_target;
  
  new_target.latitude   = target_lat;
  new_target.longitude  = target_lon;
  new_target.alt        = target_alt;
  
  return new_target;
}

coordinate get_base() {
  coordinate new_base;
  if (!DEBUG) {
    new_base.latitude   = gps.location.lat();
    new_base.longitude  = gps.location.lng();
    new_base.alt        = gps.altitude.meters(); 
  }
  else {
    new_base.latitude   = 0;
    new_base.longitude  = 0;
    new_base.alt        = 0;
  }
  return new_base;
}

bool parse_target() {
  if (COM_PORT.available()) {
    char a = COM_PORT.read();
    if (a != 10) {
      buff = buff+a;  
      return false;
    }
    else {
      String memory = "";
      double data[3];
      unsigned int count = 0;
      for (unsigned i(0); i<buff.length(); i++) {
        if (buff.charAt(i) != ',') {
          memory = memory + buff.charAt(i);
          data[count] = memory.trim().toFloat();
        }
        else {
          memory = "";
          count++;
        }
      }
      coordinate new_target;
      
      new_target.latitude   = data[0];
      new_target.longitude  = data[1];
      new_target.alt        = data[2];
      
      set_target(new_target);
      
      buff = "";
      return true;
    }
  }
  return false; 
}

bool parse_gps() {  
  while (GPS_PORT.available()) {
     gps.encode(GPS_PORT.read()); 
  }
  if (!DEBUG) {
    return gps.location.isUpdated();
  }
  else {
    return true;
  }
}

void send_cmd(cmd new_command) {
  String command_text;
  command_text = "AZM:"+String(new_command.azm)+", ELV:"+String(new_command.elv);
  COM_PORT.println(command_text);
}

void sendPacket(byte *packet, byte len){
    for (byte i = 0; i < len; i++) { GPS_PORT.write(packet[i]); }
}

void gps_setup(int a, int b, int c, int d, int e){
  
  if (a == 9600) {
    GPS_PORT.begin(9600); 
    delay(100); 
    byte packet1[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0xB5};
    sendPacket(packet1, sizeof(packet1));
  }
    
  if (a == 57600) { 
    GPS_PORT.begin(9600); 
    delay(100);
    byte packet2[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC0, 0x08, 0x00, 0x00, 0x00, 0xE1, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCE, 0xC9};
    sendPacket(packet2, sizeof(packet2));
    GPS_PORT.end(); 
    GPS_PORT.begin(57600); 
    delay(100);
  }

  if (a == 115200) { 
    GPS_PORT.begin(9600); 
    delay(100);
    byte packet3[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC0, 0x08, 0x00, 0x00, 0x00, 0xC2, 0x01, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x7E};
    sendPacket(packet3, sizeof(packet3)); 
    GPS_PORT.end(); 
    GPS_PORT.begin(115200);
    delay(100);
  }

  if (b == 1) { 
    byte packet4[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x39};
    sendPacket(packet4, sizeof(packet4));
  }

  if (b == 5) { 
    byte packet5[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A};
    sendPacket(packet5, sizeof(packet5));
  }
    
  if (b == 10) { 
    byte packet6[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12};
    sendPacket(packet6, sizeof(packet6));
  }

  if (c == 0) { 
    byte packet7[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x5E, 0x01, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x3C};
    sendPacket(packet7, sizeof(packet7));
  }
    
  if (c == 1) {
    byte packet8[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x5E, 0x01, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x08};
    sendPacket(packet8, sizeof(packet8)); 
  }

  if (c == 2) {
    byte packet8[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x5E, 0x01, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x2A};
    sendPacket(packet8, sizeof(packet8)); 
  }

  if (c == 4) {
    byte packet8[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x5E, 0x01, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x4C};
    sendPacket(packet8, sizeof(packet8)); 
  }

  if (d == 1) { 
    byte packet9[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x03, 0x35};
    byte packet10[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x05, 0x43};
    sendPacket(packet9, sizeof(packet9));
    sendPacket(packet10, sizeof(packet10));
  }

  if (e == 1) {
    byte packet11[] = {0xB5, 0x62, 0x06, 0x1E, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x32, 0x00, 0x00, 0x99, 0x4C, 0x00, 0x00, 0x5B, 0x10};
    sendPacket(packet11, sizeof(packet11));
  }
  
  GPS_PORT.addMemoryForRead(serial2bufferRead, sizeof(serial2bufferRead));
   
}
