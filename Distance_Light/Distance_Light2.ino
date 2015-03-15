/*
 HC-SR04 Ping distance sensor]
 VCC to arduino 5v GND to arduino GND
 Echo to Arduino pin 13 Trig to Arduino pin 12
 Red POS to Arduino pin 11
 Green POS to Arduino pin 10
 560 ohm resistor to both LED NEG and GRD power rail
 More info at: http://goo.gl/kJ8Gl
 Original code improvements to the Ping sketch sourced from Trollmaker.com
 Some code and wiring inspired by http://en.wikiversity.org/wiki/User:Dstaub/robotcar
 */

#include <EEPROM.h>
#define trigPin 6
#define buttonPin 2
#define echoPin 3
#define red 11
#define green 10
#define blue 9
#define RANGE 600

const int numReadings = 11;
const int defaultTarget = 60;
long distances[numReadings];
int readIndex = 0;
long total = 0;
long average = 0;
long lastAverage;
long target = defaultTarget;
long lastReading = 0;
int flashes = 0;

void setup() {
  Serial.begin (115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  target = getTargetFromEEPROM();
  if(target == 0)
    target = defaultTarget;
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    distances[thisReading] = 0;  
}

long getDistance()
{
  long duration, distance;
  digitalWrite(trigPin, LOW);  // Added this line
  delayMicroseconds(2); // Added this line
  digitalWrite(trigPin, HIGH);
  //delayMicroseconds(500);// - Removed this line
  delayMicroseconds(100); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  if(distance <= RANGE & 
    distance > 0)
  {
    total= total - distances[readIndex];    
    distances[readIndex] = distance;
    total= total+distances[readIndex];
    readIndex = readIndex+1;
    if(readIndex >= numReadings)
      readIndex = 0;
   
    average = getCleanAverage(); //total/numReadings;
    int d = average - lastAverage;
    if(abs(d) >= 5)
    {
      lastAverage = average;
      lastReading = millis();
      //Serial.println(lastReading);
     int feet = (average/2.5)/12;
     int inches = (distance/2.5);
     inches = inches %12;
     Serial.print(feet);
     Serial.print("'");  
     Serial.print(inches);
     Serial.print("\" / Raw: ");
     Serial.print(distance/2.5);
     Serial.println(" inches");
    }
  }

  return distance;
}


void loop() {

  long distance = getDistance();


  if(distance <= RANGE & 
    distance > 0 &
    digitalRead(buttonPin) == LOW)
  {


    if((millis()-lastReading) >10000)
    {
      writeColor(0,0,0);
      delay(50);
    }
    else if(average >= target+150)
    {
      writeColor(0, 254,0);          
    }
    else if(average < target+150 
      & average > target
      )
    {
      flashes = 0;
      int D = average-target;
      int Top = target+150;
      
      int R = map(average, Top, target, 50, 100);
      int G = map(average, target, Top, 100, 254);
  
    /* Serial.print(average);
      Serial.print("-");
      Serial.print(target);
      Serial.print("=");
      Serial.print(D);
      Serial.print(", ");
      Serial.print(R);
      Serial.print(", ");
      Serial.println(G);*/
      writeColor(R,G,0);     
      //writeColor(150,254,0);
    }
    else if (average <=target)
    {
      if(flashes < 5)
      {
        writeColor(10, 0, 0);
        delay(100);
        writeColor(254, 0, 0);
        delay(100);          
        flashes++;
      }
      else 
        writeColor(254, 0, 0);
    }    


   
    
  }
  else if(digitalRead(buttonPin) == HIGH)
  {
    setTarget();
  }


  delay(20);
}

long getCleanAverage()
{
  long RetVal = 0;
  long V[numReadings];
  unsigned long T = 0;

  unsigned long mean; 
  unsigned long stdDev = 0;
  int sig = 0;

  for(int i = 0; i <numReadings; i++)
  {
    if(distances[i] > 0)
    {
      T += distances[i];
      sig++;
    }
  }
  mean = T/sig;
  for (int i = 0; i <numReadings; i++)
  {
    V[i] = distances[i] - mean;
    V[i] = V[i]*V[i];
  }
  T = 0;

  for (int i = 0; i <sig; i++)
  {    
    T+= V[i];
  }
  
   //Serial.print("T: ");
 // Serial.println(T);  

  stdDev = T /sig;
//  Serial.print("StdDev^2: ");
//  Serial.println(stdDev);
  stdDev = sqrt(stdDev);
//  Serial.print("StdDev: ");
//  Serial.println(stdDev);
  sig=0;
  T = 0;
  for (int i = 0; i<numReadings; i++)
  {
    // Serial.print(distances[i]);
    // Serial.print("/");
    // Serial.print(mean);
    // Serial.print( "/");
    // Serial.println(stdDev);
    
     if(distances[i] <= mean + stdDev & distances[i] >= mean-stdDev)
    {
      sig++;
      T+= distances[i];
    }
  }  
 
  RetVal = T/sig;
 /* Serial.print("Mean: ");
  Serial.print(mean);
  Serial.print(" / Corrected: ");
  Serial.println(RetVal);
  */
  if(RetVal == -1)
    RetVal = average;
  return RetVal;
}

void setTarget()
{
  long buttonPressed = millis();
  int greenFlashes = 0;
  int blueFlashes = 0;
  boolean EEPROMClear = false;

  while(digitalRead(buttonPin) == HIGH)
  {
    if(millis() - buttonPressed >= 3000)
    {
      if(greenFlashes == 0)
      {
        target = average;
        writeTargetToEEPROM(target);
        Serial.print("Setting Target Distance: ");
        Serial.println(target);
      }
      if(greenFlashes < 5)
      {
        writeColor(0, 0, 0);
        delay(100);
        writeColor(0, 254, 0);
        delay(100);          
        greenFlashes++;
      }
      else 
        writeColor(0, 254, 0);
    }
    else 
      writeColor(0,0,254);

    if(millis() - buttonPressed >= 6000 & !EEPROMClear)
    {
      clearEEPROM();
      EEPROMClear = true;
      target = defaultTarget;
      if(blueFlashes < 5)
      {
        writeColor(0, 0, 0);
        delay(100);
        writeColor(0, 0, 254);
        delay(100);          
        blueFlashes++;
      }
      else 
        writeColor(0, 0, 254);
    }
    else
      writeColor(0,0,254);

    delay(1);
  }
}

void writeColor(int Red, int Green, int Blue)
{ 
  analogWrite(red, Red);
  analogWrite(green, Green);
  analogWrite(blue, Blue);
  /* Serial.print(Red);
   Serial.print(", ");
   Serial.print(Green);    
   Serial.print(", ");
   Serial.println(Blue);*/
}


void clearEEPROM()
{
  Serial.print("Clearing EEPROM... ");

  writeColor(254, 0, 0);
  for (int i = 0; i < 1024; i++)
    EEPROM.write(i, 0);

  Serial.println("DONE.");

}

long getTargetFromEEPROM()
{
  byte buf[4];
  long retVal;

  for(int i = 0; i<4; i++)
    buf[i] = EEPROM.read(i);

  retVal = (unsigned long)(buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

  Serial.print("EEPROM Target: ");
  Serial.println(retVal);
  return retVal;
}

void writeTargetToEEPROM(long target)
{
  byte buf[4];  
  buf[0] = (byte) target;
  buf[1] = (byte) target >> 8;
  buf[2] = (byte) target >> 16;
  buf[3] = (byte) target >> 24;

  for(int i = 0; i<4; i++)
    EEPROM.write(i, buf[i]);

  Serial.print("Wrote EEPROM Target: ");
  Serial.println(target);
}

