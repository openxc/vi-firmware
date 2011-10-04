/* Welcome to the ECU Reader project. This sketch uses the Canbus library.
It requires the CAN-bus shield for the Arduino. This shield contains the MCP2515 CAN controller and the MCP2551 CAN-bus driver.
A connector for an EM406 GPS receiver and an uSDcard holder with 3v level convertor for use in data logging applications.
The output data can be displayed on a serial LCD.

The SD test functions requires a FAT16 formated card with a text file of WRITE00.TXT in the card.


SK Pang Electronics www.skpang.co.uk

v3.0 21-02-11  Use library from Adafruit for sd card instead.

*/

#include <SdFat.h>        /* Library from Adafruit.com */
#include <SdFatUtil.h>
#include <NewSoftSerial.h>
#include <Canbus.h>


Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

NewSoftSerial sLCD =  NewSoftSerial(3, 6); /* Serial LCD is connected on pin 14 (Analog input 0) */
#define COMMAND 0xFE
#define CLEAR   0x01
#define LINE0   0x80
#define LINE1   0xC0


/* Define Joystick connection */
#define UP     A1
#define RIGHT  A2
#define DOWN   A3
#define CLICK  A4
#define LEFT   A5

  
char buffer[512];  //Data will be temporarily stored to this buffer before being written to the file
char tempbuf[15];
char lat_str[14];
char lon_str[14];

int read_size=0;   //Used as an indicator for how many characters are read from the file
int count=0;       //Miscellaneous variable

int D10 = 10;

int LED2 = 8;
int LED3 = 7;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
  PgmPrint("error: ");
  SerialPrintln_P(str);
  
  clear_lcd();
  sLCD.print("SD error");
  
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
   
  }
  while(1);
}

NewSoftSerial mySerial =  NewSoftSerial(4, 5);

#define COMMAND 0xFE
//#define powerpin 4

#define GPSRATE 4800
//#define GPSRATE 38400


// GPS parser for 406a
#define BUFFSIZ 90 // plenty big
//char buffer[BUFFSIZ];
char *parseptr;
char buffidx;
uint8_t hour, minute, second, year, month, date;
uint32_t latitude, longitude;
uint8_t groundspeed, trackangle;
char latdir, longdir;
char status;
uint32_t waypoint = 0;
 
void setup() {
  Serial.begin(GPSRATE);
  mySerial.begin(GPSRATE);
  pinMode(LED2, OUTPUT); 
  pinMode(LED3, OUTPUT); 
 
  digitalWrite(LED2, LOW);
  pinMode(UP,INPUT);
  pinMode(DOWN,INPUT);
  pinMode(LEFT,INPUT);
  pinMode(RIGHT,INPUT);
  pinMode(CLICK,INPUT);

  digitalWrite(UP, HIGH);       /* Enable internal pull-ups */
  digitalWrite(DOWN, HIGH);
  digitalWrite(LEFT, HIGH);
  digitalWrite(RIGHT, HIGH);
  digitalWrite(CLICK, HIGH);
  
  
  Serial.begin(9600);
  Serial.println("ECU Reader");  /* For debug use */
  
  sLCD.begin(9600);              /* Setup serial LCD and clear the screen */
  clear_lcd();
 
  sLCD.print("D:CAN  U:GPS");
  sLCD.print(COMMAND,BYTE);
  sLCD.print(LINE1,BYTE); 
  sLCD.print("L:SD   R:LOG");
  
  while(1)
  {
    
    if (digitalRead(UP) == 0){
      Serial.println("gps");
      sLCD.print("GPS");
      gps_test();
    }
    
    if (digitalRead(DOWN) == 0) {
      sLCD.print("CAN");
      Serial.println("CAN");
      break;
    }
    
    if (digitalRead(LEFT) == 0) {
    
      Serial.println("SD test");
      sd_test();
    }
    
    if (digitalRead(RIGHT) == 0) {
    
      Serial.println("Logging");
      logging();
    }
    
  }
  
  clear_lcd();
  
  if(Canbus.init(CANSPEED_500))  /* Initialise MCP2515 CAN controller at the specified speed */
  {
    sLCD.print("CAN Init ok");
  } else
  {
    sLCD.print("Can't init CAN");
  } 
   
  delay(1000); 

}
 

void loop() {
 
  if(Canbus.ecu_req(ENGINE_RPM,buffer) == 1)          /* Request for engine RPM */
  {
    sLCD.print(COMMAND,BYTE);                   /* Move LCD cursor to line 0 */
    sLCD.print(LINE0,BYTE);
    sLCD.print(buffer);                         /* Display data on LCD */
   
    
  } 
  digitalWrite(LED3, HIGH);
   
  if(Canbus.ecu_req(VEHICLE_SPEED,buffer) == 1)
  {
    sLCD.print(COMMAND,BYTE);
    sLCD.print(LINE0 + 9,BYTE);
    sLCD.print(buffer);
   
  }
  
  if(Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer) == 1)
  {
    sLCD.print(COMMAND,BYTE);
    sLCD.print(LINE1,BYTE);                     /* Move LCD cursor to line 1 */
    sLCD.print(buffer);
   
   
  }
  
  if(Canbus.ecu_req(THROTTLE,buffer) == 1)
  {
    sLCD.print(COMMAND,BYTE);
    sLCD.print(LINE1 + 9,BYTE);
    sLCD.print(buffer);
     file.print(buffer);
  }  
//  Canbus.ecu_req(O2_VOLTAGE,buffer);
     
   
   digitalWrite(LED3, LOW); 
   delay(100); 
   
   

}


void logging(void)
{
  clear_lcd();
  
  if(Canbus.init(CANSPEED_500))  /* Initialise MCP2515 CAN controller at the specified speed */
  {
    sLCD.print("CAN Init ok");
  } else
  {
    sLCD.print("Can't init CAN");
  } 
   
  delay(500);
  clear_lcd(); 
  sLCD.print("Init SD card");  
  delay(500);
  clear_lcd(); 
  sLCD.print("Press J/S click");  
  sLCD.print(COMMAND,BYTE);
  sLCD.print(LINE1,BYTE);                     /* Move LCD cursor to line 1 */
   sLCD.print("to Stop"); 
  
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!card.init(SPI_HALF_SPEED,9)) error("card.init failed");
  
  // initialize a FAT volume
  if (!volume.init(&card)) error("volume.init failed");
  
  // open the root directory
  if (!root.openRoot(&volume)) error("openRoot failed");

  // create a new file
  char name[] = "WRITE00.TXT";
  for (uint8_t i = 0; i < 100; i++) {
    name[5] = i/10 + '0';
    name[6] = i%10 + '0';
    if (file.open(&root, name, O_CREAT | O_EXCL | O_WRITE)) break;
  }
  if (!file.isOpen()) error ("file.create");
  Serial.print("Writing to: ");
  Serial.println(name);
  // write header
  file.writeError = 0;
  file.print("READY....");
  file.println();  

  while(1)    /* Main logging loop */
  {
     read_gps();
     
     file.print(waypoint++);
     file.print(',');
     file.print(lat_str);
     file.print(',');
     file.print(lon_str);
     file.print(',');
      
    if(Canbus.ecu_req(ENGINE_RPM,buffer) == 1)          /* Request for engine RPM */
      {
        sLCD.print(COMMAND,BYTE);                   /* Move LCD cursor to line 0 */
        sLCD.print(LINE0,BYTE);
        sLCD.print(buffer);                         /* Display data on LCD */
        file.print(buffer);
         file.print(',');
    
      } 
      digitalWrite(LED3, HIGH);
   
      if(Canbus.ecu_req(VEHICLE_SPEED,buffer) == 1)
      {
        sLCD.print(COMMAND,BYTE);
        sLCD.print(LINE0 + 9,BYTE);
        sLCD.print(buffer);
        file.print(buffer);
        file.print(','); 
      }
      
      if(Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer) == 1)
      {
        sLCD.print(COMMAND,BYTE);
        sLCD.print(LINE1,BYTE);                     /* Move LCD cursor to line 1 */
        sLCD.print(buffer);
         file.print(buffer);
       
      }
      
      if(Canbus.ecu_req(THROTTLE,buffer) == 1)
      {
        sLCD.print(COMMAND,BYTE);
        sLCD.print(LINE1 + 9,BYTE);
        sLCD.print(buffer);
         file.print(buffer);
      }  
    //  Canbus.ecu_req(O2_VOLTAGE,buffer);
       file.println();  
  
       digitalWrite(LED3, LOW); 
 
       if (digitalRead(CLICK) == 0){  /* Check for Click button */
           file.close();
           Serial.println("Done");
           sLCD.print(COMMAND,BYTE);
           sLCD.print(CLEAR,BYTE);
     
           sLCD.print("DONE");
          while(1);
        }

  }
 
 
 
 
}
     

void sd_test(void)
{
 clear_lcd(); 
 sLCD.print("SD test"); 
 Serial.println("SD card test");
   
     // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!card.init(SPI_HALF_SPEED,9)) error("card.init failed");
  
  // initialize a FAT volume
  if (!volume.init(&card)) error("volume.init failed");
  
  // open root directory
  if (!root.openRoot(&volume)) error("openRoot failed");
  // open a file
  if (file.open(&root, "LOGGER00.CSV", O_READ)) {
    Serial.println("Opened PRINT00.TXT");
  }
  else if (file.open(&root, "WRITE00.TXT", O_READ)) {
    Serial.println("Opened WRITE00.TXT");    
  }
  else{
    error("file.open failed");
  }
  Serial.println();
  
  // copy file to serial port
  int16_t n;
  uint8_t buf[7];// nothing special about 7, just a lucky number.
  while ((n = file.read(buf, sizeof(buf))) > 0) {
    for (uint8_t i = 0; i < n; i++) Serial.print(buf[i]);
  
  
  }
 clear_lcd();  
 sLCD.print("DONE"); 

  
 while(1);  /* Don't return */ 
    

}
void read_gps(void)
{
 uint32_t tmp;

  unsigned char i;
  unsigned char exit = 0;
  
  
  while( exit == 0)
  { 
    
   readline();
 
  // check if $GPRMC (global positioning fixed data)
   if (strncmp(buffer, "$GPRMC",6) == 0) {
     
        digitalWrite(LED2, HIGH);
        
        // hhmmss time data
        parseptr = buffer+7;
        tmp = parsedecimal(parseptr); 
        hour = tmp / 10000;
        minute = (tmp / 100) % 100;
        second = tmp % 100;
        
        parseptr = strchr(parseptr, ',') + 1;
        status = parseptr[0];
        parseptr += 2;
          
        for(i=0;i<11;i++)
        {
          lat_str[i] = parseptr[i];
        }
        lat_str[12] = 0;
      //  Serial.println(" ");
      //  Serial.println(lat_str);
       
        // grab latitude & long data
        latitude = parsedecimal(parseptr);
        if (latitude != 0) {
          latitude *= 10000;
          parseptr = strchr(parseptr, '.')+1;
          latitude += parsedecimal(parseptr);
        }
        parseptr = strchr(parseptr, ',') + 1;
        // read latitude N/S data
        if (parseptr[0] != ',') {
          
          latdir = parseptr[0];
        }
        
        // longitude
        parseptr = strchr(parseptr, ',')+1;
      
        for(i=0;i<12;i++)
        {
          lon_str[i] = parseptr[i];
        }
        lon_str[13] = 0;
        
        //Serial.println(lon_str);
   
        longitude = parsedecimal(parseptr);
        if (longitude != 0) {
          longitude *= 10000;
          parseptr = strchr(parseptr, '.')+1;
          longitude += parsedecimal(parseptr);
        }
        parseptr = strchr(parseptr, ',')+1;
        // read longitude E/W data
        if (parseptr[0] != ',') {
          longdir = parseptr[0];
        }
        
    
        // groundspeed
        parseptr = strchr(parseptr, ',')+1;
        groundspeed = parsedecimal(parseptr);
    
        // track angle
        parseptr = strchr(parseptr, ',')+1;
        trackangle = parsedecimal(parseptr);
    
        // date
        parseptr = strchr(parseptr, ',')+1;
        tmp = parsedecimal(parseptr); 
        date = tmp / 10000;
        month = (tmp / 100) % 100;
        year = tmp % 100;
        
       
        digitalWrite(LED2, LOW);
        exit = 1;
       }
       
  }   

}

      
      

void gps_test(void){
  uint32_t tmp;
  uint32_t lat;
  unsigned char i;
  
  while(1){
  
   readline();
  
  // check if $GPRMC (global positioning fixed data)
  if (strncmp(buffer, "$GPRMC",6) == 0) {
    
    // hhmmss time data
    parseptr = buffer+7;
    tmp = parsedecimal(parseptr); 
    hour = tmp / 10000;
    minute = (tmp / 100) % 100;
    second = tmp % 100;
    
    parseptr = strchr(parseptr, ',') + 1;
    status = parseptr[0];
    parseptr += 2;
      
    for(i=0;i<11;i++)
    {
      lat_str[i] = parseptr[i];
    }
    lat_str[12] = 0;
     Serial.println("\nlat_str ");
     Serial.println(lat_str);
   
  
    // grab latitude & long data
    // latitude
    latitude = parsedecimal(parseptr);
    if (latitude != 0) {
      latitude *= 10000;
      parseptr = strchr(parseptr, '.')+1;
      latitude += parsedecimal(parseptr);
    }
    parseptr = strchr(parseptr, ',') + 1;
    // read latitude N/S data
    if (parseptr[0] != ',') {
      
      latdir = parseptr[0];
    }
    
    //Serial.println(latdir);
    
    // longitude
    parseptr = strchr(parseptr, ',')+1;
  
    for(i=0;i<12;i++)
    {
      lon_str[i] = parseptr[i];
    }
    lon_str[13] = 0;
    
    Serial.println(lon_str);
   
  
    longitude = parsedecimal(parseptr);
    if (longitude != 0) {
      longitude *= 10000;
      parseptr = strchr(parseptr, '.')+1;
      longitude += parsedecimal(parseptr);
    }
    parseptr = strchr(parseptr, ',')+1;
    // read longitude E/W data
    if (parseptr[0] != ',') {
      longdir = parseptr[0];
    }
    

    // groundspeed
    parseptr = strchr(parseptr, ',')+1;
    groundspeed = parsedecimal(parseptr);

    // track angle
    parseptr = strchr(parseptr, ',')+1;
    trackangle = parsedecimal(parseptr);

    // date
    parseptr = strchr(parseptr, ',')+1;
    tmp = parsedecimal(parseptr); 
    date = tmp / 10000;
    month = (tmp / 100) % 100;
    year = tmp % 100;
    
    Serial.print("\nTime: ");
    Serial.print(hour, DEC); Serial.print(':');
    Serial.print(minute, DEC); Serial.print(':');
    Serial.print(second, DEC); Serial.print(' ');
    Serial.print("Date: ");
    Serial.print(month, DEC); Serial.print('/');
    Serial.print(date, DEC); Serial.print('/');
    Serial.println(year, DEC);
    
    sLCD.print(COMMAND,BYTE);
    sLCD.print(0x80,BYTE);
    sLCD.print("La");
   
    Serial.print("Lat"); 
    if (latdir == 'N')
    {
       Serial.print('+');
       sLCD.print("+");
    }
    else if (latdir == 'S')
    {  
       Serial.print('-');
       sLCD.print("-");
    }
     
    Serial.print(latitude/1000000, DEC); Serial.print('\°', BYTE); Serial.print(' ');
    Serial.print((latitude/10000)%100, DEC); Serial.print('\''); Serial.print(' ');
    Serial.print((latitude%10000)*6/1000, DEC); Serial.print('.');
    Serial.print(((latitude%10000)*6/10)%100, DEC); Serial.println('"');
    
    
    
    sLCD.print(latitude/1000000, DEC); sLCD.print(0xDF, BYTE); sLCD.print(' ');
    sLCD.print((latitude/10000)%100, DEC); sLCD.print('\''); //sLCD.print(' ');
    sLCD.print((latitude%10000)*6/1000, DEC); sLCD.print('.');
    sLCD.print(((latitude%10000)*6/10)%100, DEC); sLCD.print('"');
    
    sLCD.print(COMMAND,BYTE);
    sLCD.print(0xC0,BYTE);
    sLCD.print("Ln");
   
      
    Serial.print("Long: ");
    if (longdir == 'E')
    {
       Serial.print('+');
       sLCD.print('+');
    }
    else if (longdir == 'W')
    { 
       Serial.print('-');
       sLCD.print('-');
    }
    Serial.print(longitude/1000000, DEC); Serial.print('\°', BYTE); Serial.print(' ');
    Serial.print((longitude/10000)%100, DEC); Serial.print('\''); Serial.print(' ');
    Serial.print((longitude%10000)*6/1000, DEC); Serial.print('.');
    Serial.print(((longitude%10000)*6/10)%100, DEC); Serial.println('"');
    
    sLCD.print(longitude/1000000, DEC); sLCD.print(0xDF, BYTE); sLCD.print(' ');
    sLCD.print((longitude/10000)%100, DEC); sLCD.print('\''); //sLCD.print(' ');
    sLCD.print((longitude%10000)*6/1000, DEC); sLCD.print('.');
    sLCD.print(((longitude%10000)*6/10)%100, DEC); sLCD.print('"');
     
  }
  
 //   Serial.println("Lat: ");
 //   Serial.println(latitude);
  
 //   Serial.println("Lon: ");
 //   Serial.println(longitude);
  }



}

void readline(void) {
  char c;
  
  buffidx = 0; // start at begninning
  while (1) {
      c=mySerial.read();
      if (c == -1)
        continue;
  //    Serial.print(c);
      if (c == '\n')
        continue;
      if ((buffidx == BUFFSIZ-1) || (c == '\r')) {
        buffer[buffidx] = 0;
        return;
      }
      buffer[buffidx++]= c;
  }
}
uint32_t parsedecimal(char *str) {
  uint32_t d = 0;
  
  while (str[0] != 0) {
   if ((str[0] > '9') || (str[0] < '0'))
     return d;
   d *= 10;
   d += str[0] - '0';
   str++;
  }
  return d;
}

void clear_lcd(void)
{
  sLCD.print(COMMAND,BYTE);
  sLCD.print(CLEAR,BYTE);
}  
