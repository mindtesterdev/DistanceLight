/*

  This is the code to my ultrasonic parking guide (ie a glorified tennisball on a string).
  Parts are based on the Ping sensor example. 
  
   - Expects HC-SR04 Ranging Detector Mod Distance Sensor Trigger and Echo pins connected to pins 3 and 6.
   - Expects one standard RGB LED (the red green and blue #defines are for those pins on the LED.)
   - Expects one momentary normally open button that throws HIGH to buttonPin when pressed.
   
   LED will be green at >target distance + 150CM (5 Ft.)
   LED gradients yellow to red from Target Distance+150CM to Target Distance
   At target distance LED flashes red 5 times then goes to solid red.
   Pressing the button and holding for 3 seconds will save the currently measured distance as the target distance.
   The LED will turn blue while you're holding it, then flash green 3 times when the setting is saved to the first 4 bytes of the EEPROM.
   Continuing to hold the button for another 3 seconds will cause the LED to turn red and the EEPROM will be written out all zeros.
   If you put this on a hackduino / fresh ATMEGA chip with a programmer you may need to clear EEPROM with above procedure to get the sensor to behave properly (I had some strange results with a new chip).
  
  The serial is only enabled for debugging, you could remove all of that if you weren't going to make any changes to make the compiled sketch smaller.
  
  Distributed under GPLv3
  
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
      writeColor(R,G,0);     
      
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
  
  stdDev = T /sig;
  stdDev = sqrt(stdDev);
  sig=0;
  T = 0;
  
  for (int i = 0; i<numReadings; i++)
  {   
     if(distances[i] <= mean + stdDev & distances[i] >= mean-stdDev)
        {
          sig++;
          T+= distances[i];
        }
  }  
 
  RetVal = T/sig;
  
  if(RetVal == -1)
    RetVal = average; //If we fail the calculation, just return the existing average to keep everything smoothed.
    
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

