
#include "pitches.h"
#include <LiquidCrystal_SR.h>
#include <WiFiEsp.h>
#include <Wire.h>
//#include <SD.h>
//#include "SPI.h"
#include "wifi.h"

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(5, 6); // RX, TX
#endif

#define MAX_BUF_LEN 300

int status = WL_IDLE_STATUS; // the Wifi radio's status
int speakerOut = 7;



const char server[] = "kernel32.in";
const unsigned int port = 3000;
// PROGMEM char path[] = "GET /api/data.csv HTTP/1.1";
const char path[] = "GET /test HTTP/1.1";

const int chipSelect = 4;

// Initialize the Ethernet client object
WiFiEspClient client;

LiquidCrystal_SR
    lcd(2, 10, 9); // Pin 6 - Data Enable/ SER, Pin 5 - Clock/SCL, Pin 9 -SCK

void display(String first) {

  unsigned int start = 0;
  if (first.length() <= 32) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(first);
  } else {
    start = 0;
    while (start <= first.length()) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(first.substring(start, start + 15));
      start += 15;
      lcd.setCursor(0, 1);
      lcd.print(first.substring(start, start + 15));
      start += 15;

      delay(5000);
    }
  }
}
unsigned int start = 0;
void display(String first, String second) {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(first);

  if (second.length() <= 16) {
    lcd.setCursor(0, 1);
    lcd.print(second);
  } else {
    start = 0;
    while (start <= second.length() - 16) {
      lcd.setCursor(0, 1);

      lcd.print(second.substring(start, start + 15));
      start++;
      delay(500);
    }
  }

  // set the display to automatically scroll:
}

// notes in the melody:
int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3,
                NOTE_G3, 0,       NOTE_B3, NOTE_C4};

int startMelody[] = {NOTE_B4, NOTE_B3, NOTE_G3, NOTE_A3,
                     NOTE_F3, 0,       NOTE_B3, NOTE_C4};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {8, 8, 5, 4, 8, 4, 4, 4};

void playToneA() {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(speakerOut, startMelody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.10;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(speakerOut);
  }
}
void setup() {

  String WifiConnectStr = "WIFI CONNECT-";
  Serial.begin(9600);

  Serial.println(F("News Feeder V 1.0 build 100"));
  playToneA();
  lcd.begin(16, 2); // Initializing LCD
  // lcd.autoscroll();

  // initialize serial for debugging
  // initialize serial for ESP module
  Serial1.begin(9600);
  // see if the card is present and can be initialized:
  // if (!SD.begin(chipSelect)) {
  //   Serial.println(F("Card failed, or not present"));
  //   // don't do anything more:
  //   display(F("ERROR"),F("SD FAIL"));
  // //  while(1);
  // }

  //Serial.println(F("card initialized."));

  display(F("SETUP:..:"), F("WIFI INIT"));

  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    display(F("SETUP: "), F("WIFI FAILED INIT"));
    // don't continue
    while (true)
      ;
  }

  // attempt to connect to WiFi network
  int count = 0;
  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to WPA SSID: "));
    count++;
    display(F("SETUP"), WifiConnectStr + count);
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // status = WL_CONNECTED;
  }

  // you're connected now, so print out the data
  Serial.println(F("You're connected to the network"));

  display("SETUP", "SERVER CONNECT");
  Serial.println(F("Starting connection to server..."));
  // if you get a connection, report back via serial
  if (client.connect(server, port)) {
    Serial.println(F("Connected to server"));
    // Make a HTTP request
    client.println(path);
    client.println(F("Host: kernel32.in"));
    client.println(F("Connection: close"));
    client.println();
  }
}

void loop() {
  char stream_line[MAX_BUF_LEN];
  //char data[2][210];
  char *data[5];
  int data_version = -1;
  int data_stamp = -1;
  int data_count = -1;
  int data_index = 0;
  int item_size = -1;
  int item_timeout = -1;
  unsigned int indx = 0;

  int waittime = 0;
//  String line1;
  String line2;

  int i = 0;

  // if there are incoming bytes available
  // from the server, read them and print them

  int headerdata = 1;
  char prevchar = '\0';
  int dataread = 0;
  Serial.println(F("loop"));

  display(F("SETUP"), F("OTA UPDATE"));
  while (dataread == 0) {
    while (client.available()) {
      dataread = 1;
      char c = client.read();

      // if(c == '\r'){
      //   Serial.write('#');
      // }
      // if(c == '\n'){
      //   Serial.write('@');
      // }
      // the line is \r\n
      if (headerdata == 1 && c == '\r') {
        //    Serial.println("h1");
        if (prevchar == '\n') {
          //    Serial.println("h2");
          // start of data
          headerdata = 0;
          // skipp the /n
          client.read();
        }
      }

      prevchar = c;
      Serial.write(c);
      if (headerdata == 0) {

        if (c == '\n') {
          stream_line[i] = 0;
          //  Serial.println(F("DATA LINE"));
          //  Serial.println(stream_line);
          if (data_version == -1) {
            display(F("SETUP"), F("DATA UPDATE"));
            data_version = atoi(stream_line);
          } else if (data_stamp == -1) {
            data_stamp = atoi(stream_line);
          } else if (data_count == -1) {
            data_count = atoi(stream_line);

            //allcoate the rows for data .( latter)



          } else if(item_timeout == -1) {
            item_timeout = 0;
            // no action required now
          }  else if (item_size == -1) {
              item_size = atoi(stream_line);
          } else {

           // allocate the space first
          //   Serial.println(F("sream line"));
           data[data_index] = (char * ) malloc(item_size * sizeof(char *));
        //   Serial.println(stream_line);
        //   Serial.println(item_size);

            if (data_index < data_count) {
              strcpy(data[data_index], stream_line);
              //for (int temp =0 ; temp < i)
              data_index++;
            }
            item_timeout = -1 ;
            item_size = -1;
          }
          //reset the i ;
          i=0;
        } else {
          if (i < MAX_BUF_LEN) {
            stream_line[i] = c;
            i = i + 1;
          }
        }


      }
    }
  }
  stream_line[i] = 0;
  Serial.println(F("String to write is "));
  Serial.println(data_version);
  Serial.println(data_stamp);
  Serial.println(data_count);
  Serial.println(stream_line);

  if (!client.connected()) {
    Serial.println(F("Disconnecting from server..."));
    client.stop();
  }

  if (data_count <= 0) {
    display(F("NO DATA"));
  } else {
    // show the data .
    waittime = 3;
    while (1) {
      // exit condition required ///
    //  line1.remove(0);
      line2.remove(0);
      line2.concat(data[indx]);

      // Assume we have everything ..

      Serial.println(F("parsing done"));
      Serial.println(line2);
      Serial.println(waittime);
      //playToneA();
      display(line2);
      delay(waittime * 1000);

      indx++;
      if (indx == data_count) {
        indx = 0;
      }
    }
  }

  // File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // if (dataFile) {
  //   dataFile.println(json);
  //   dataFile.close();
  //   Serial.println(F("Written data"));
  //
  // }
  // else {
  //   Serial.println(F("error opening datalog.txt"));
  // }

  // Serial.println("after read");
  // Serial.println(json);
  /*  StaticJsonBuffer<300> jsonBuffer;
   JsonObject& root = jsonBuffer.parseObject(json);
   if (!root.success())
 {
   Serial.println("parseObject() failed");
    while (1);
 }
    String  displaydata = root["display"]["data"];
      displaydata = root.get<String>("display");
     //root.printTo(Serial);
   Serial.println("after displaydata");
   //Serial.println(root.get<String>("display"));
  Serial.println("print displaydata");
   // using C++11 syntax:
 for (auto kv : root) {
     Serial.println(kv.key);
     Serial.println(kv.value.as<String>());
 }
 */

  // delay(4000);
  // if the server's disconnected, stop the client
  while (1)
    ;
}

/*

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}*/
