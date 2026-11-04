
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <Wire.h>

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to Digital 8
// Connect the GPS RX (receive) pin to Digital 7

// you can change the pin numbers to match your wiring:
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

//Analog input pin (center tap of potentiometer)
const int potPin = A0;

//address of the 7 segment led display
const byte s7sAddress = 0x71;
//state of input switch
int state = 0;
//last state of input switch
int lastState = 0;
//viewing mode (can be 0,1,2,3)
int mode = 0;

void setup()
{
  pinMode(4, INPUT_PULLUP); //mode switch will be connected to pin 4 on one side and gnd on the other
  delay(500);
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request antenna status from old and new modules; comment out both to keep quiet
  //GPS.sendCommand(F(PGCMD_ANTENNA));
  //GPS.sendCommand(F(CDCMD_ANTENNA));

  delay(1000);
  // Ask for firmware version
  mySerial.println(PMTK_Q_RELEASE);

    //begin communication with 7-segment display
  Wire.begin();
  //clear the display and set cursor to the left
  clearDisplayI2C();
}

uint32_t timer = millis();
void loop()                     // run over and over again
{
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if ((c) && (GPSECHO))
    Serial.write(c);

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // approximately every 200 milliseconds or so, print out the current stats
  if (millis() - timer > 200) {
    timer = millis(); // reset the timer
    int rawValue = analogRead(potPin);

    //set display brightness - default is 255 (full high)
    setBrightnessI2C(map(rawValue, 0, 1023, 0, 255));
    
    if (GPS.fix) 
    {
      determineState();
    }
    else
    {
      setDecimalsI2C(0b000000);
      s7sSendStringI2C("5HIT");
    }
  }
}

void determineState()
{
  //read pin 4 to get the curernt switch state
  state = digitalRead(4);
  //if the current state doesn't equal the last state AND the current state is low:
  if (state != lastState && state == LOW)
  {
    //increment the mode
    mode ++;
    if (mode > 3)
    {
      mode = 0; //set the mode back to 0 if all modes have been cycled
    }
  }
  //for debounce
  delay(10);
  //set the last known switch state to the current switch state
  lastState = state;
  //call the display generation method
  generateDisplay();
}

void generateDisplay()
{
  switch (mode) //mode is the variable the switch-case is looking at
  {
    case 0: //if mode equals 0
        displaySpeed(); //call the displaySpeed method
        break;
    case 1: //if mode equals 1
        displayAngle(); //call the displayAngle method
        break;
    case 2: //if mode equals 2
        displayTime(); //call the displayTime method
        break;
    case 3: //if mode equals 3
        displaySats(); //call the displaySats() method
        break;
  }
}


void displaySpeed()
{
  //blank all decimals and colons
  setDecimalsI2C(0b000000);
  //convert GPS speed from knots to mph and reduce to integer
  int mphNum = int(GPS.speed*1.15078);
  //turn the previously calculated number into a String
  String mph = String(mphNum);
  //placeholder value for the final output
  String output = "";

  //if speed is less than 10
  if (mphNum >= 0 && mphNum < 10)
  {
      //provide proper spacing to ensure both digits are in the middle of display
      output = "  " + mph +" ";
  }
  
  else
  {
      //same as above - provides proper spacing
      output = " " + mph + " ";
  }
  //output the speed to the 7-segment display
  s7sSendStringI2C(output);
}


void displayAngle()
{
  //receive angle from GPS and convert it to an int
  int angleNum = int(GPS.angle);
  //convert the integer angle to a string
  String angle = String(angleNum);
  //placeholder for final output
  String output = "";

  //angle between 0 and 9
  if (angleNum >0 && angleNum < 10)
  {
    output = "H  " + angle;
  }

  //angle between 10 and 99
  else if (angleNum >= 10 && angleNum < 100)
  {
    output = "H " + angle;
  }

  //angle greater than 100
  else if (angleNum >= 100) 
  {
    output = "H" + angle;
  }

  //send the formatted angle string to the display
  s7sSendStringI2C(output);
}

void displayTime()
{
  //retrieve the hour from GPS
  int hourNum = GPS.hour;
  //retrieve the minute from GPS
  int minNum = GPS.minute;
  //convert the hour and minute into string format  
  String hour; 
  String minutes; 

  //if hour is between 0 and 9
  if (hourNum < 10)
  {
    //add a 0 to the front of it
    hour = "0" + String(hourNum);
  }

  //otherwise keep it the same
  else
  {
    hour = String(hourNum);
  }

  //if the minutes are between 0 and 9
  if (minNum < 10)
  {
    //add a 0 to the front
    minutes = "0" + String(minNum);
  }

  //otherwise keep it the same
  else
  {
    minutes = String(minNum);
  }
 
  //combine the hours and minutes
  s7sSendStringI2C(hour + minutes);

  //turn the colon on
  setDecimalsI2C(0b00010000);

}


void displaySats()
{
  //holder value for string value of satellites
  String sats;

  //if num satellites is less than 10
  if (GPS.satellites < 10)
  {
    //add a 0 to the front of the string
    sats = "0" + String(GPS.satellites);
  }

  //if greater than 10
  else
  {
    //use the number of satellites
    sats = String(GPS.satellites);
  }

  //output the number of satellites to the displayu
  s7sSendStringI2C("5t" + sats);
}


// This custom function works somewhat like a serial.print.
//  You can send it an array of chars (string) and it'll print
//  the first 4 characters in the array
void s7sSendStringI2C(String toSend)
{
  Wire.beginTransmission(s7sAddress);
  for (int i = 0; i < 4; i ++)
  {
    Wire.write(toSend[i]);
  }
  Wire.endTransmission();
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplayI2C()
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x76);
  Wire.endTransmission();
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightnessI2C(byte value)
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x7A);
  Wire.write(value);
  Wire.endTransmission();
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimalsI2C(byte decimals)
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x77);
  Wire.write(decimals);
  Wire.endTransmission();
}
