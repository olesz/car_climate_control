/* AC compressor on-off via a relay based on analogut input
   --------------------------------------------------------

   Turns on and off AC compressor. The amount of time the relay
   will be on and off depends on the value obtained by analogRead().

   Created 22 February 2020
   copyright 2020 Lajos Olah <lajos.olah.jr@gmail.com>

*/

const int COMPRESSOR_ON = LOW;
const int COMPRESSOR_OFF = HIGH;
const int RELAY_PIN = 3;                                   // relay output
const int POTENTIOMETER_PIN = 2;                           // potentiometer input
const float ANALOGUE_MAX_VALUE = 1023.0;                   // maximum potentiometer value
const unsigned long TIME_FRAME = 60000.0;                  // 1 minutes in milliseconds
const unsigned long MINIMAL_COMPRESSOR_ON_OFF = 5000;      // seconds while compressor is on
int STATE;                                                 // state (on/off)

void setup() {
  pinMode(RELAY_PIN, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  unsigned long compressorOnOff;               // seconds while compressor is on or off

  STATE = COMPRESSOR_ON;
  compressorOnOff = calculateCompressorOnOff(STATE);
  printCompressorValues(compressorOnOff, STATE);
  setCompressor(STATE, compressorOnOff);

  STATE = COMPRESSOR_OFF;
  compressorOnOff = calculateCompressorOnOff(STATE);
  printCompressorValues(compressorOnOff, STATE);
  setCompressor(STATE, compressorOnOff);
}

unsigned long calculateCompressorOnOff(int STATE) {
  // potmeret torned to the left means low temp. -> max - actual value
  int analogueValue = ANALOGUE_MAX_VALUE - analogRead(POTENTIOMETER_PIN);

  unsigned long sleepValue = (float) TIME_FRAME * ((float)analogueValue / ANALOGUE_MAX_VALUE);

  if (STATE == COMPRESSOR_OFF) {
    sleepValue = TIME_FRAME - sleepValue;
  }

  if (sleepValue == 0) {
    sleepValue = MINIMAL_COMPRESSOR_ON_OFF;
  }

  return sleepValue / 1000;
}

void setCompressor(int STATE, unsigned long compressorOnOff) {  
  digitalWrite(RELAY_PIN, STATE);
  Serial.print("Setting compressor ");

  switch (STATE) {
    case COMPRESSOR_ON:
      Serial.print("on ");
      break;
    case COMPRESSOR_OFF:
      Serial.print("off ");
      break;
  }

  Serial.print(compressorOnOff);
  Serial.println(" s");

  wait(compressorOnOff);
}

void wait(unsigned long compressorOnOff) {
  unsigned long localCompressorOnOff;

  for (unsigned long i = 0; i <= compressorOnOff; i++) {
    delay(1000);
    Serial.println(i);

    localCompressorOnOff = calculateCompressorOnOff(STATE);
    if (localCompressorOnOff != compressorOnOff) {
      Serial.println("Timing changed");
      printCompressorValues(localCompressorOnOff, STATE);
      compressorOnOff = localCompressorOnOff;
    }
  }
}

void printCompressorValues(unsigned long compressorOnOff, int STATE) {
  Serial.print("Compressor on value ");
  Serial.print(STATE == COMPRESSOR_ON ? compressorOnOff : TIME_FRAME / 1000 - compressorOnOff);
  Serial.println(" s");
  Serial.print("Compressor off value ");
  Serial.print(STATE == COMPRESSOR_ON ? TIME_FRAME / 1000 - compressorOnOff : compressorOnOff);
  Serial.println(" s");
}
