/* Relay on-off based on analogut input
   ------------------------------------

   turns on and off a relay. The amount of time the relay will be on and off depends on
   the value obtained by analogRead().

   Created 22 February 2020
   copyright 2020 Lajos Olah <lajos.olah.jr@gmail.com>

*/

const int COMPRESSOR_ON = LOW;
const int COMPRESSOR_OFF = HIGH;
const int relayPin = 3;                                 // relay output
const int potPin = 2;                                   // potentiometer input
const float analogueValueMax = 1023.0;                  // maximum potentiometer value
const unsigned long timeFrame = 60000.0;                // 1 minutes in milliseconds
const unsigned long minimalCompressorOnOff = 5000;      // seconds while compressor is on
int state;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relayPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  unsigned long compressorOnOff;               // seconds while compressor is on or off

  state = COMPRESSOR_ON;
  compressorOnOff = calculateCompressorOnOff(state);
  printCompressorValues(compressorOnOff, state);
  setCompressor(state, compressorOnOff);

  state = COMPRESSOR_OFF;
  compressorOnOff = calculateCompressorOnOff(state);
  printCompressorValues(compressorOnOff, state);
  setCompressor(state, compressorOnOff);
}

unsigned long calculateCompressorOnOff(int state) {
  int analogueValue = analogueValueMax - analogRead(potPin);

  unsigned long sleepValue = (float) timeFrame * ((float)analogueValue / analogueValueMax);

  if (state == COMPRESSOR_OFF) {
    sleepValue = timeFrame - sleepValue;
  }

  if (sleepValue == 0) {
    sleepValue = minimalCompressorOnOff;
  }

  return sleepValue / 1000;
}

void setCompressor(int state, unsigned long compressorOnOff) {
  digitalWrite(relayPin, state);
  Serial.print("Setting compressor ");

  switch (state) {
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

    localCompressorOnOff = calculateCompressorOnOff(state);
    if (localCompressorOnOff != compressorOnOff) {
      Serial.println("Timing changed");
      printCompressorValues(localCompressorOnOff, state);
      compressorOnOff = localCompressorOnOff;
    }
  }
}

void printCompressorValues(unsigned long compressorOnOff, int state) {
  Serial.print("Compressor on value ");
  Serial.print(state == COMPRESSOR_ON ? compressorOnOff : timeFrame / 1000 - compressorOnOff);
  Serial.println(" s");
  Serial.print("Compressor off value ");
  Serial.print(state == COMPRESSOR_ON ? timeFrame / 1000 - compressorOnOff : compressorOnOff);
  Serial.println(" s");
}
