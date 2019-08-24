/*
    Talking to Hue from an Arduino
    By James Bruce (MakeUseOf.com)
    Adapted from code by Gilson Oguime. https://github.com/oguime/Hue_W5100_HT6P20B/blob/master/Hue_W5100_HT6P20B.ino
    
*/
#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>
#include <SD.h>

// RFID Setup
#include <MFRC522.h>

#define SS_PIN 38
#define RST_PIN 39

#define STATUS_PIN_1 6
#define STATUS_PIN_2 7
#define RELAY_PIN 49

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

byte checkArray[7][4]={
  {30,57,118,250},
  {62,228,95,66},
  {62,40,109,66},
  {174,66,145,66},
  {78,244,164,105}, // cat
  {138,195,60,121}, // reset
  {158,189,191,250} // find hue bridge
 };

// SD Card Setup
#define ARDUINO_RX 5  //should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 6  //connect to RX of the module
//SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);
#define mp3 Serial3    // Connect the MP3 Serial Player to the Arduino MEGA Serial3 (14 TX3 -> RX, 15 RX3 -> TX)

static int8_t Send_buf[8] = {0}; // Buffer for Send commands.  // BETTER LOCALLY
static uint8_t ansbuf[10] = {0}; // Buffer for the answers.    // BETTER LOCALLY

String mp3Answer;           // Answer from the MP3.

#define CMD_PLAY_W_INDEX  0X03
#define CMD_SET_VOLUME    0X06

#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
#define CMD_PLAY          0X0D
#define CMD_PAUSE         0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F

#define CMD_STOP_PLAY     0X16

#define CMD_SEL_DEV       0X09
#define DEV_TF            0X02

//  Hue constants
 
char hueHubIP[40];  // Hue hub IP
char hueUsername[80];  // Hue username
const int hueHubPort = 80;
IPAddress gatewayIPAddress;

boolean activated = false;

//  Hue variables
boolean hueOn;  // on/off
int hueBri;  // brightness value
long hueHue;  // hue value
String hueCmd;  // Hue command

unsigned long buffer=0;  //buffer for received data storage
unsigned long addr;
 
//  Ethernet
byte mac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };  // W5100 MAC address
IPAddress ip(192,168,1,2);  // Arduino IP
EthernetClient client;
/*
 
    Setup
 
*/
void setup()
{
  Serial.begin(9600);

  mp3.begin(9600);
  delay(500);

  sendCommand(CMD_SEL_DEV, DEV_TF);
  delay(500);
  
  SPI.begin(); // Init SPI bus

  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, LOW);

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 7; i++) {
    key.keyByte[i] = 0xFF;
  }

  analogWrite(STATUS_PIN_1, 0);
  
  Ethernet.begin(mac,ip);

  gatewayIPAddress = Ethernet.gatewayIP();
  Serial.println(gatewayIPAddress);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  readSDCard();
  
  delay(2000);
  analogWrite(STATUS_PIN_1, 150);
  Serial.println("Ready.");

}

  

void loop() 
{
  digitalWrite(SS_PIN, LOW);
  digitalWrite(10, HIGH);
  
  delay(100);

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  Serial.print(F("In hex: "));
  for (int i = 0; i < 4; i++)
  {
    Serial.print(rfid.uid.uidByte[i]);
    Serial.print(" ");
  }
  Serial.println();

  for (int i = 0; i < 7; i++)
  {

    if (rfid.uid.uidByte[0] == checkArray[i][0] &&
      rfid.uid.uidByte[1] == checkArray[i][1] &&
      rfid.uid.uidByte[2] == checkArray[i][2] &&
      rfid.uid.uidByte[3] == checkArray[i][3])
      {
        Serial.println("Success!");
        flashLights(i);
        break;
      }
  }

  if (Ethernet.linkStatus() == LinkON) {
    Serial.println("LinkON");
    analogWrite(STATUS_PIN_1, 150);
  }
  else if (Ethernet.linkStatus() == LinkOFF) {
    analogWrite(STATUS_PIN_1, 0);
    Serial.println("LinkOFF");
  }
}

void flashLights(int house)
{
  // 0 - Gryffindor
  // 1 - Hufflepuff
  // 2 - Ravenclaw
  // 3 - Slytherin
  
  digitalWrite(SS_PIN, HIGH);
  digitalWrite(10, LOW);

  if (house == 0)
  {
    String command = "{\"on\": true,\"hue\": 1000,\"sat\":255,\"bri\":255,\"transitiontime\":5}";
    bool commandSuccess1 = setHue("1",command);

    sendMP3Command('g');

    analogWrite(STATUS_PIN_2, 150);

    delay(24000);
  }
  else if (house == 1)
  {
    String command = "{\"on\": true,\"hue\": 8000,\"sat\":255,\"bri\":255,\"transitiontime\":5}";
    bool commandSuccess1 = setHue("1",command);

    sendMP3Command('h');

    analogWrite(STATUS_PIN_2, 150);

    delay(19000);
  }
  else if (house == 2)
  {
    String command = "{\"on\": true,\"hue\": 45000,\"sat\":255,\"bri\":255,\"transitiontime\":5}";
    bool commandSuccess1 = setHue("1",command);

    sendMP3Command('r');

    analogWrite(STATUS_PIN_2, 150);

    delay(19000);
  }
  else if (house == 3)
  {
    String command = "{\"on\": true,\"hue\": 15000,\"sat\":255,\"bri\":255,\"transitiontime\":5}";
    bool commandSuccess1 = setHue("1",command);

    sendMP3Command('s');

    analogWrite(STATUS_PIN_2, 150);

    delay(24000);
  }
  else if (house == 4)
  {
    digitalWrite(RELAY_PIN, LOW);
  }
  else if (house == 5)
  {
    digitalWrite(RELAY_PIN, HIGH);
    
    String command = "{\"on\": true,\"hue\": 15000,\"sat\":1,\"bri\":255,\"transitiontime\":5}";
    setHue("1",command);
  }
  else if (house == 6)
  {
    autoSearchHue();
  }

    
    delay(3000);
    analogWrite(STATUS_PIN_2, 0);
    
    // so we can track state
    activated = true;
}

void autoSearchHue()
{
  analogWrite(STATUS_PIN_1, 0);
  bool hueFound = false;
  for (int i = 0; i < 255; i++)
  {
    analogWrite(STATUS_PIN_2, 150);
    delay(10);
    analogWrite(STATUS_PIN_2, 0);
    
    if (hueFound)
    {
      break;
    }
    //char ip[40];
    //strcpy( ip, String(i).c_str() );
    gatewayIPAddress[3] = i;
    Serial.println(gatewayIPAddress);
    hueFound = pingIP(gatewayIPAddress);
    if (hueFound)
    {
      analogWrite(STATUS_PIN_1, 150);
      writeIP(gatewayIPAddress);
    }
  }

  // SD Card Setup
  readSDCard();
}

/* setHue() is our main command function, which needs to be passed a light number and a 
 * properly formatted command string in JSON format (basically a Javascript style array of variables
 * and values. It then makes a simple HTTP PUT request to the Bridge at the IP specified at the start.
 */
boolean setHue(String lightNum,String command)
{
  Serial.println("trying to connect to: ");
  Serial.println(hueHubIP);
  if (client.connect(hueHubIP, hueHubPort))
  {
    while (client.connected())
    {
      client.print("PUT /api/");
      client.print(hueUsername);
      client.print("/groups/");
      client.print(0);  // hueLight zero based, add 1
      client.println("/action HTTP/1.1");
      client.println("keep-alive");
      client.print("Host: ");
      client.println(hueHubIP);
      client.print("Content-Length: ");
      client.println(command.length());
      client.println("Content-Type: text/plain;charset=UTF-8");
      client.println();  // blank line before body
      client.println(command);  // Hue command
      client.stop();
    }
    
    analogWrite(STATUS_PIN_1, 150);
    return true;  // command executed
  }
  else
    analogWrite(STATUS_PIN_1, 0);
    return false;  // command failed
}

/* A helper function in case your logic depends on the current state of the light. 
 * This sets a number of global variables which you can check to find out if a light is currently on or not
 * and the hue etc. Not needed just to send out commands
 */
boolean getHue(int lightNum)
{
  if (client.connect(hueHubIP, hueHubPort))
  {
    client.print("GET /api/");
    client.print(hueUsername);
    client.print("/lights/");
    client.print(lightNum);  
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(hueHubIP);
    client.println("Content-type: application/json");
    client.println("keep-alive");
    client.println();
    while (client.connected())
    {
      if (client.available())
      {
        client.findUntil("\"on\":", "\0");
        hueOn = (client.readStringUntil(',') == "true");  // if light is on, set variable to true
 
        client.findUntil("\"bri\":", "\0");
        hueBri = client.readStringUntil(',').toInt();  // set variable to brightness value
 
        client.findUntil("\"hue\":", "\0");
        hueHue = client.readStringUntil(',').toInt();  // set variable to hue value
        
        break;  // not capturing other light attributes yet
      }
    }
    client.stop();
    return true;  // captured on,bri,hue
  }
  else
    return false;  // error reading on,bri,hue
}

boolean pingIP(IPAddress ipAddress)
{
  if (client.connect(ipAddress, hueHubPort))
  {
    Serial.println("connected");
    String command = "{\"devicetype\":\"test user\"}";

    client.print("POST /api");
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(ipAddress);
    client.print("Content-Length: ");
    client.println(command.length());
    client.println("Content-Type: text/plain;charset=UTF-8");
    client.println();  // blank line before body
    client.println(command);  // Hue command
    
    client.println("Content-type: application/json");
    client.println("keep-alive");
    client.println();
    client.println(command);
    delay(2000);
      
    while (client.connected())
    {
      
      if (client.available())
      {
        client.findUntil("username\":\"", "fakeTerminator");
        String username = client.readStringUntil('\"');
        Serial.println("username: ");
        Serial.println(username);

        if (username.length() > 0)
        {
          Serial.println("Found username");
          writeUsername(username);
          writeIP(ipAddress);
          client.stop();
          return true;
        }
        else
        {
          Serial.println("Could not find username");
          client.stop();
          return false;
        }
      }
      client.stop();
      return false;
    }
  }
  else
  {
    Serial.println("could not connect");
    return false;
  }
}

void sendMP3Command(char c) {
  switch (c) {
    case '?':
      Serial.println("HELP  ");
      Serial.println(" p = Play");
      Serial.println(" P = Pause");
      Serial.println(" > = Next");
      Serial.println(" < = Previous");
      Serial.println(" + = Volume UP");
      Serial.println(" - = Volume DOWN");
      Serial.println(" c = Query current file");
      Serial.println(" q = Query status");
      Serial.println(" v = Query volume");
      Serial.println(" x = Query folder count");
      Serial.println(" t = Query total file count");
      Serial.println(" 1 = Play folder 1");
      Serial.println(" 2 = Play folder 2");
      Serial.println(" 3 = Play folder 3");
      Serial.println(" 4 = Play folder 4");
      Serial.println(" 5 = Play folder 5");
      Serial.println(" S = Sleep");
      Serial.println(" W = Wake up");
      Serial.println(" r = Reset");
      break;


    case 'g':
      Serial.println("Playing Gryffindor ");
      sendCommand(CMD_WAKE_UP, 1);
      sendCommand(CMD_SET_VOLUME, 30);
      sendCommand(CMD_PLAY_W_INDEX, 1);
      break;

    case 'h':
      Serial.println("Playing Hufflepuff ");
      sendCommand(CMD_WAKE_UP, 1);
      sendCommand(CMD_SET_VOLUME, 30);
      sendCommand(CMD_PLAY_W_INDEX, 2);
      break;

    case 'r':
      Serial.println("Playing Ravenclaw ");
      sendCommand(CMD_WAKE_UP, 1);
      sendCommand(CMD_SET_VOLUME, 30);
      sendCommand(CMD_PLAY_W_INDEX, 3);
      break;

    case 's':
      Serial.println("Playing Slytherine ");
      sendCommand(CMD_WAKE_UP, 1);
      sendCommand(CMD_SET_VOLUME, 30);
      sendCommand(CMD_PLAY_W_INDEX, 4);
      break;
  }
}

void sendCommand(int8_t command, int16_t dat)
{
  delay(20);
  Send_buf[0] = 0x7e;   //
  Send_buf[1] = 0xff;   //
  Send_buf[2] = 0x06;   // Len
  Send_buf[3] = command;//
  Send_buf[4] = 0x00;   // 0x00 NO, 0x01 feedback
  Send_buf[5] = (int8_t)(dat >> 8);  //datah
  Send_buf[6] = (int8_t)(dat);       //datal
  Send_buf[7] = 0xef;   //
  Serial.print("Sending: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    mp3.write(Send_buf[i]) ;
    Serial.print(sbyte2hex(Send_buf[i]));
  }
  Serial.println();
}

void writeUsername(String username)
{
  if (!SD.begin(4))
  {
    Serial.println("SD card initialization failed");
    return;
  }

  File usernameFile;

  if (!SD.exists("user.txt"))
  {
    Serial.println("creating user.txt");
    usernameFile = SD.open("user.txt", FILE_WRITE);
    usernameFile.close();
  }

  usernameFile = SD.open("user.txt", FILE_WRITE | O_TRUNC);
  Serial.println("Writing username");
  Serial.println(username);
  usernameFile.print(username);
  usernameFile.close();
}

void writeIP(IPAddress ipAddress)
{
  if (!SD.begin(4))
  {
    Serial.println("SD card initialization failed");
    return;
  }

  File ipFile;

  if (!SD.exists("ipAddr.txt"))
  {
    Serial.println("creating ipAddr.txt");
    ipFile = SD.open("ipAddr.txt", FILE_WRITE);
    ipFile.close();
  }

  ipFile = SD.open("ipAddr.txt", FILE_WRITE | O_TRUNC);
  Serial.println("Writing ip");
  Serial.println(ipAddress);
  ipFile.print(ipAddress);
  ipFile.close();
}

void readSDCard()
{
  if (!SD.begin(4))
  {
    Serial.println("SD card initialization failed");
    return;
  }

  File ipAddressFile;
  File usernameFile;

  Serial.println("SD card initialized");

  // Find IP Address file
  if (SD.exists("ipAddr.txt"))
  {
    Serial.println("found ipAddr.txt");
    ipAddressFile = SD.open("ipAddr.txt", FILE_READ);
    String ipString = ipAddressFile.readString();
    Serial.println(ipString);
    strcpy( hueHubIP, ipString.c_str() );
    ipAddressFile.close();
  }
  else
  {
    Serial.println("creating ipAddr.txt");
    ipAddressFile = SD.open("ipAddr.txt", FILE_WRITE);
    ipAddressFile.close();
  }

  // Find Username file
  if (SD.exists("user.txt"))
  {
    Serial.println("found user.txt");
    usernameFile = SD.open("user.txt", FILE_READ);
    String usernameString = usernameFile.readString();
    Serial.println(usernameString);
    strcpy( hueUsername, usernameString.c_str() );
    usernameFile.close();
  }
  else
  {
    Serial.println("creating user.txt");
    usernameFile = SD.open("user.txt", FILE_WRITE);
    usernameFile.close();
  }
}



/********************************************************************************/
/*Function: sbyte2hex. Returns a byte data in HEX format.                 */
/*Parameter:- uint8_t b. Byte to convert to HEX.                                */
/*Return: String                                                                */


String sbyte2hex(uint8_t b)
{
  String shex;

  shex = "0X";

  if (b < 16) shex += "0";
  shex += String(b, HEX);
  shex += " ";
  return shex;
}
