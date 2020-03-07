/* AC compressor on-off via a relay based on analogut input
   --------------------------------------------------------

   Turns on and off AC compressor. The amount of time the relay
   will be on and off depends on the value read from the potentiometer attahced.

   Duty cycle provided by G65 pressure sensor is also analyzed.
   Compressor/fan state is changed based on that.

   Created 02 March 2020
   Copyright 2020 Lajos Olah <lajos.olah.jr@gmail.com>

*/

#include <pt.h>
#include <movingAvg.h>

#define PRESSURE_SENSOR_PRESENT 0 // pressure sensor present or not
#define PRESSURE_SENSOR_PIN 2     // pressure sensor input - DIGITAL

#define POTENTIOMETER_PIN 2       // potentiometer input - ANALOGUE

#define COMPRESSOR_PIN 3          // compresor output - DIGITAL

#define FAN_PRESENT 0             // fan present or not
#define FAN_PIN 5                 // fan output - DIGITAL

#define MILLISEC_1000 1000
#define MILLISEC_100 100
#define MILLISEC_60000 60000.0

#define PRESSURE_SENSOR_DUTY_CYCLE_LENGTH 20000   // G65 pressure sensor sends 50hz signal -> 20000microsec is the cycle length
#define DUTY_CYCLE_PRESSURE_LOW_PERCENTAGE 15     // below this percentage compressor turned off (low pressure)
#define DUTY_CYCLE_PRESSURE_HIGH_PERCENTAGE 85    // above this percentage compressor turned off (high pressure)
#define DUTY_CYCLE_FAN_ON_PERCENTAGE 55           // above this percentage fan turned on
#define DUTY_CYCLE_MOVING_AVERAGE_COUNT 100       // 100*20ms=2sec: samples are kept and averaged for 2 seconds

#define COUNTER_INIT 1                    // initial value of cunter which counts the seconds in compressor on/off phase
#define STATE_ON 0                        // state on value
#define STATE_OFF 1                       // state off value
#define ANALOGUE_MAX_VALUE 1023.0         // maximum potentiometer value
#define MINIMAL_COMPRESSOR_ON_OFF 5000    // seconds while compressor is on

volatile unsigned long prev_time = 0;   // needed for interrupt handling

unsigned long compressorOn;   // seconds while compressor is on or off
unsigned long compressorOff;  // seconds while compressor is on or off

static struct pt ptReadAnalogueValue;
static struct pt ptDoCompressorOnOff;

movingAvg avgDutyCycle(DUTY_CYCLE_MOVING_AVERAGE_COUNT); 

/* This function reads analogut value from potentiometer */
static int readAnalogueValue(struct pt *pt) {
  static unsigned long timestamp = 0;
  static unsigned long localCompressorOn;
  static unsigned long localCompressorOff;
  
  PT_BEGIN(pt);
  
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - timestamp > MILLISEC_100);
    timestamp = millis(); // take a new timestamp
    localCompressorOn = calculateCompressorOnOff(STATE_ON);
    localCompressorOff = calculateCompressorOnOff(STATE_OFF);

    if (localCompressorOn != compressorOn || localCompressorOff != compressorOff) {
      compressorOn = localCompressorOn;
      compressorOff = localCompressorOff;
      printCompressorValues(compressorOn, compressorOff);
    }
  }
  
  PT_END(pt);
}

/* This function does compressor and fan on-off */
static int doCompressorOnOff(struct pt *pt) {
  static unsigned long timestamp = 0;
  static int counter = COUNTER_INIT;
  static int compressorState = STATE_ON;
  static int fanState = STATE_OFF;
  
  PT_BEGIN(pt);

  PT_WAIT_UNTIL(pt, millis() - timestamp > MILLISEC_1000);

  setCompressor(compressorState);
  
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - timestamp > MILLISEC_1000);
    timestamp = millis(); // take a new timestamp
    unsigned long compressorOnOff = compressorState == STATE_ON ? compressorOn : compressorOff;
    unsigned long avg = avgDutyCycle.getAvg();
    double avgPercentage = (double)(avg*100/PRESSURE_SENSOR_DUTY_CYCLE_LENGTH);

    if (avgPercentage > DUTY_CYCLE_FAN_ON_PERCENTAGE) {
      fanState = STATE_ON;
    } else {
      fanState = STATE_OFF;
    }

    setFan(fanState);

    if (PRESSURE_SENSOR_PRESENT != 0 && (avgPercentage < DUTY_CYCLE_PRESSURE_LOW_PERCENTAGE || avgPercentage > DUTY_CYCLE_PRESSURE_HIGH_PERCENTAGE)) {
      Serial.println("DC is out of range [" + String(DUTY_CYCLE_PRESSURE_LOW_PERCENTAGE) + "%-"
        + DUTY_CYCLE_PRESSURE_HIGH_PERCENTAGE + "%]: " + getDutyCyclePrintout(avg, avgPercentage, false));
      setCompressor(STATE_OFF);
      continue;
    }

    if (counter > compressorOnOff) {
      counter = COUNTER_INIT;
      compressorState = compressorState == STATE_ON ? STATE_OFF : STATE_ON;
      setCompressor(compressorState);
    }

    Serial.println(String(counter) + " (" + getStateString(compressorState) + " for " + String(compressorState == STATE_ON ? compressorOn : compressorOff)
      + "s" + getDutyCyclePrintout(avg, avgPercentage, true) + getFanPrintout(fanState) + ")");
    counter++;
  }
  
  PT_END(pt);
}

void rising()
{
  attachInterrupt(digitalPinToInterrupt(PRESSURE_SENSOR_PIN), falling, FALLING);
  prev_time = micros();
}
 
void falling() {
  attachInterrupt(digitalPinToInterrupt(PRESSURE_SENSOR_PIN), rising, RISING);
  avgDutyCycle.reading(micros()-prev_time);
}

void setup() {
  Serial.begin(9600);

  avgDutyCycle.begin();

  attachInterrupt(digitalPinToInterrupt(PRESSURE_SENSOR_PIN), rising, RISING);
  
  pinMode(COMPRESSOR_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PRESSURE_SENSOR_PIN, INPUT);
  
  PT_INIT(&ptReadAnalogueValue);
  PT_INIT(&ptDoCompressorOnOff);
}

void loop() {
  readAnalogueValue(&ptReadAnalogueValue);
  doCompressorOnOff(&ptDoCompressorOnOff);
}

unsigned long calculateCompressorOnOff(int state) {
  // potmeret turned to the left means low temp. -> max - actual value
  int analogueValue = ANALOGUE_MAX_VALUE - analogRead(POTENTIOMETER_PIN);

  unsigned long sleepValue = MILLISEC_60000 * ((float)analogueValue / ANALOGUE_MAX_VALUE);

  if (state == STATE_OFF) {
    sleepValue = MILLISEC_60000 - sleepValue;
  }

  if (sleepValue == 0) {
    sleepValue = MINIMAL_COMPRESSOR_ON_OFF;
  }

  return sleepValue / MILLISEC_1000;
}

void setCompressor(int state) {
  digitalWrite(COMPRESSOR_PIN, state);
}

void setFan(int state) {
  digitalWrite(FAN_PIN, state);
}

void printCompressorValues(unsigned long compressorOn, unsigned long compressorOff) {
  Serial.println("On value " + String(compressorOn) + "s, off value " + String(compressorOff) + "s");
}

String getDutyCyclePrintout(unsigned long ms, double percentage, boolean hyphenNeeded) {
  String result;
  if (PRESSURE_SENSOR_PRESENT == 0) {
    return "";
  } else {
    if (hyphenNeeded) {
      result = result + " - DC ";
    }
    result = result + String(percentage) + "%, " + String(ms) + "ms";
    return result;
  }
}

String getStateString(int state) {
  return String(state == STATE_ON ? "on" : "off");
}

String getFanPrintout(int fanState) {
  if (FAN_PRESENT == 0) {
    return "";
  } else {
    return " - fan " + getStateString(fanState);
  }
}
