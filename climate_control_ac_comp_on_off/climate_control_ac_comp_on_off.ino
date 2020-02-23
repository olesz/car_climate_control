/* AC compressor on-off via a relay based on analogut input
   --------------------------------------------------------

   Turns on and off AC compressor. The amount of time the relay
   will be on and off depends on the value obtained by analogRead().

   Created 22 February 2020
   Copyright 2020 Lajos Olah <lajos.olah.jr@gmail.com>

*/

#include <pt.h>

#define SEC_1 1000
#define MILLISEC_100 100

#define COUNTER_INIT 1
#define COMPRESSOR_ON 0
#define COMPRESSOR_OFF 1
#define RELAY_PIN 3                       // relay output
#define POTENTIOMETER_PIN 2               // potentiometer input
#define ANALOGUE_MAX_VALUE 1023.0         // maximum potentiometer value
#define TIME_FRAME 60000.0                // 1 minutes in milliseconds
#define MINIMAL_COMPRESSOR_ON_OFF 5000    // seconds while compressor is on
unsigned long compressorOn;               // seconds while compressor is on or off
unsigned long compressorOff;              // seconds while compressor is on or off

static struct pt ptReadAnalogueValue;
static struct pt ptDoCompressorOnOff;

/* This function reads analogut value from potentiometer */
static int readAnalogueValue(struct pt *pt) {
  static unsigned long timestamp = 0;
  static unsigned long localCompressorOn;
  static unsigned long localCompressorOff;
  
  PT_BEGIN(pt);
  
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - timestamp > MILLISEC_100);
    timestamp = millis(); // take a new timestamp
    localCompressorOn = calculateCompressorOnOff(COMPRESSOR_ON);
    localCompressorOff = calculateCompressorOnOff(COMPRESSOR_OFF);

    if (localCompressorOn != compressorOn || localCompressorOff != compressorOff) {
      compressorOn = localCompressorOn;
      compressorOff = localCompressorOff;
      Serial.println("Values changed");
      printCompressorValues(compressorOn, compressorOff);
    }
  }
  
  PT_END(pt);
}

/* This function does compressor on-off */
static int doCompressorOnOff(struct pt *pt) {
  static unsigned long timestamp = 0;
  static int state = COMPRESSOR_ON;
  static int counter = COUNTER_INIT;
  
  PT_BEGIN(pt);

  PT_WAIT_UNTIL(pt, millis() - timestamp > SEC_1);

  setCompressor(state);
  
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - timestamp > SEC_1);
    timestamp = millis(); // take a new timestamp
    unsigned long compressorOnOff = state == COMPRESSOR_ON ? compressorOn : compressorOff;

    if (counter > compressorOnOff) {
      counter = COUNTER_INIT;
      state = state == COMPRESSOR_ON ? COMPRESSOR_OFF : COMPRESSOR_ON;
      setCompressor(state);
    }

    Serial.print(counter);
    Serial.print(" (");
    Serial.print(state == COMPRESSOR_ON ? "on" : "off");
    Serial.println(")");
    counter++;
  }
  
  PT_END(pt);
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  PT_INIT(&ptReadAnalogueValue);
  PT_INIT(&ptDoCompressorOnOff);

  Serial.begin(9600);
}

void loop() {
  readAnalogueValue(&ptReadAnalogueValue);
  doCompressorOnOff(&ptDoCompressorOnOff);
}

unsigned long calculateCompressorOnOff(int state) {
  // potmeret turned to the left means low temp. -> max - actual value
  int analogueValue = ANALOGUE_MAX_VALUE - analogRead(POTENTIOMETER_PIN);

  unsigned long sleepValue = (float) TIME_FRAME * ((float)analogueValue / ANALOGUE_MAX_VALUE);

  if (state == COMPRESSOR_OFF) {
    sleepValue = TIME_FRAME - sleepValue;
  }

  if (sleepValue == 0) {
    sleepValue = MINIMAL_COMPRESSOR_ON_OFF;
  }

  return sleepValue / SEC_1;
}

void setCompressor(int state) {  
  Serial.print("Setting compressor ");

  switch (state) {
    case COMPRESSOR_ON:
      Serial.print("on ");
      break;
    case COMPRESSOR_OFF:
      Serial.print("off ");
      break;
  }
  Serial.print("(");
  Serial.print(state == COMPRESSOR_ON ? compressorOn : compressorOff);
  Serial.println("s)");
  digitalWrite(RELAY_PIN, state);
}

void printCompressorValues(unsigned long compressorOn, unsigned long compressorOff) {
  Serial.print("Compressor on value ");
  Serial.print(compressorOn);
  Serial.println(" s");
  Serial.print("Compressor off value ");
  Serial.print(compressorOff);
  Serial.println(" s");
}
