#include "Arduino.h"
#include "Analog.h"

Analog::Analog(uint8_t analogPin, uint8_t controllerNumber, uint8_t channel) // Constructor
{
  this->analogPin = analogPin;
  this->controllerNumber = controllerNumber;
  this->channel = channel;
}

Analog::~Analog() // Destructor
{
  free(avValues);            // free the sample buffer malloc'ed in Analog::average
  if (bankEnabled)           // if bank mode was used
    pinMode(bankPin, INPUT); // make make the bank switch pin a normal input again, without the internal pullup resistor.
}

void Analog::average(size_t length) // use the average of multiple samples of analog readings
{
  if (length == 0 || length == 1 || avLen) // the average of 0 or 1 samples is meaningless, if "av" already exists, don't allocate new memory
    return;
  avValues = (unsigned int *)malloc(length * sizeof(unsigned int)); // allocate memory for the sample buffer
  memset(avValues, 0, length * sizeof(unsigned int));               // set all values in the buffer to zero
  avLen = length;
}

void Analog::refresh() // read the analog value, update the average, map it to a MIDI value, check if it changed since last time, if so, send Control Change message over MIDI
{
  unsigned int input = analogRead(analogPin); // read the raw analog input value
  input = analogMap(input);                   // apply the analogMap function to the value (identity function f(x) = x by default)
  if (avLen)
  {                                // if averaging is enabled
    input = runningAverage(input); // update the running average with the new value
  }
  value = input >> 3;  // map from the 10-bit analog input value [0, 1023] to the 7-bit MIDI value [0, 127]
  if (value != oldVal) // if the value changed since last time
  {
    if (bankEnabled && !digitalRead(bankPin))                       // if the bank mode is enabled, and the bank switch is in the 'alternative' position (i.e. if the switch is on (LOW))
      USBMidiController.send(CC, altChannel + channelOffset, altController + addressOffset, value); // send a Control Change MIDI event with the 'alternative' channel and controller number
    else                                                            // if the bank mode is disabled, or the bank switch is in the normal position
      USBMidiController.send(CC, channel + channelOffset, controllerNumber + addressOffset, value); // send a Control Change MIDI event with the normal, original channel and controller number
    oldVal = value;
  }
}

void Analog::map(int (*fn)(int)) // change the function pointer for analogMap to a new function. It will be applied to the raw analog input value in Analog::refresh()
{
  analogMap = fn;
}

void Analog::bank(uint8_t bankPin, uint8_t altController, uint8_t altChannel) // Enable the bank mode. When bank switch is turned on, send alternative MIDI channel and controller numbers
{                                                                             // bankPin = digital pin with toggle switch connected
  bankEnabled = true;

  this->bankPin = bankPin;
  pinMode(bankPin, INPUT_PULLUP);
  this->altController = altController;
  this->altChannel = altChannel;
}

void Analog::detachBank() // Disable the bank mode
{
  if (bankEnabled) // only defined if bank mode is enabled
  {
    bankEnabled = false;
    pinMode(bankPin, INPUT); // make it a normal input again, without the internal pullup resistor.
  }
}

unsigned int Analog::runningAverage(unsigned int value) // http://playground.arduino.cc/Main/RunningAverage
{
  avSum -= avValues[avIndex];
  avValues[avIndex] = value;
  avSum += value;
  avIndex++;
  avIndex = avIndex % avLen;
  if (avCount < avLen)
    avCount++;
  return avSum / avCount;
}