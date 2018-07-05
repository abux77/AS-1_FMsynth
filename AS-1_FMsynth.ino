/* 
_____/\\\\\\\\\________/\\\\\\\\\\\______________________/\\\_        
 ___/\\\\\\\\\\\\\____/\\\/////////\\\________________/\\\\\\\_       
  __/\\\/////////\\\__\//\\\______\///________________\/////\\\_      
   _\/\\\_______\/\\\___\////\\\__________/\\\\\\\\\\\_____\/\\\_     
    _\/\\\\\\\\\\\\\\\______\////\\\______\///////////______\/\\\_    
     _\/\\\/////////\\\_________\////\\\_____________________\/\\\_   
      _\/\\\_______\/\\\__/\\\______\//\\\____________________\/\\\_  
       _\/\\\_______\/\\\_\///\\\\\\\\\\\/_____________________\/\\\_ 
        _\///________\///____\///////////_______________________\///_ 

5 Knob FM Synth

POT 1 = OSCILLATOR FREQUENCY
POT 2 = MODULATOR FREQUENCY
POT 3 = FREQUENCY SHIFT
POT 4 = MODULATION INTENSITY 1x
POT 5 = MODULATION INTENSITY 10x
     ______________________________
   /     ____              ____     \
  |     /    \            /    \     |
  |    ( POT1 )          ( POT4 )    |
  |     \____/            \____/     |
  |               ____               |
  |              /    \              |
  |             ( POT3 )             |
  |              \____/              |
  |      ____              ____      |
  |     /    \            /    \     |
  |    ( POT2 )          ( POT5 )    |
  |     \____/            \____/     |
  |                                  |
   \ _______________________________/

Based on Mozzi example
Knob_LightLevel_x2_FMsynth
Tim Barrass 2013, CC by-nc-sa
http://sensorium.github.com/Mozzi/
  
Andrew Buckie 2018, CC by-nc-sa
*/

#include <MozziGuts.h>
#include <Oscil.h> // oscillator 
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <Smooth.h>
#include <AutoMap.h> // maps unpredictable inputs to a range
#include <mozzi_fixmath.h>
#include "SynthBoxConfig.h"

// Declare mapping variables
int CarrierFreq;
const int MinCarrierFreq = 22;
const int MaxCarrierFreq = 440;
float ModSpeed;
const int MinModSpeed = 1;
const int MaxModSpeed = 10000;
int FreqShift;
const int MinFreqShift = 1;
const int MaxFreqShift = 10;
int Intensity;
const int MinIntensity = 10;
const int MaxIntensity = 500;
int IntensityMultiplier;
const int MinIntensityMultiplier = 1;
const int MaxIntensityMultiplier = 10;

Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);

int mod_ratio = 5; // brightness (harmonics)
long fm_intensity; // carries control info from updateControl to updateAudio

// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);

void setup(){
  startMozzi();
  Serial.begin(115200); // For Debugging of values

  pinMode(DIP_1, INPUT_PULLUP);   // Configure D2 as input with pull-up resistor
  pinMode(DIP_2, INPUT_PULLUP);   // Configure D3 as input with pull-up resistor
  pinMode(DIP_3, INPUT_PULLUP);   // Configure D4 as input with pull-up resistor
  pinMode(DIP_4, INPUT_PULLUP);   // Configure D5 as input with pull-up resistor
  pinMode(DIP_5, INPUT_PULLUP);   // Configure D6 as input with pull-up resistor
  pinMode(ExpPlug, INPUT_PULLUP); // Configure D7 as input with pull-up resistor
 
}

void updateControl(){

  // Read digital inputs
  ExpPot1 = digitalRead(DIP_1);   // DIP switch 1 - 1 = Off / 0 = On
  ExpPot2 = digitalRead(DIP_2);   // DIP switch 2 - 1 = Off / 0 = On
  ExpPot3 = digitalRead(DIP_3);   // DIP switch 3 - 1 = Off / 0 = On
  ExpPot4 = digitalRead(DIP_4);   // DIP switch 4 - 1 = Off / 0 = On
  ExpPot5 = digitalRead(DIP_5);   // DIP switch 5 - 1 = Off / 0 = On
  ExpDetct = digitalRead(ExpPlug); // 1 = Expression pedal plugged in / 0 = Expression pedal 

  // Map to expression pedal if selected
  if (ExpPot1 == 0 & ExpDetct ==1){
    Pot1 = Exp;    
  }
  else{
    Pot1 = Pot1Default;
  }
  if (ExpPot2 == 0 & ExpDetct ==1){
    Pot2 = Exp;    
  }
  else{
    Pot2 = Pot2Default;
  }
  if (ExpPot3 == 0 & ExpDetct ==1){
    Pot3 = Exp;    
  }
  else{
    Pot3 = Pot3Default;
  }
  if (ExpPot4 == 0 & ExpDetct ==1){
    Pot4 = Exp;    
  }
  else{
    Pot4 = Pot4Default;
  }
  if (ExpPot5 == 0 & ExpDetct ==1){
    Pot5 = Exp;    
  }
  else{
    Pot5 = Pot5Default;
  }
  
  // Read all the analog inputs
  Pot1Val = mozziAnalogRead(Pot1); // value is 0-1023
  Pot2Val = mozziAnalogRead(Pot2); // value is 0-1023
  Pot3Val = mozziAnalogRead(Pot3); // value is 0-1023
  Pot4Val = mozziAnalogRead(Pot4); // value is 0-1023
  Pot5Val = mozziAnalogRead(Pot5); // value is 0-1023
  ExpVal  = mozziAnalogRead(Exp);  // value is 0-1023

  // Map Values
  CarrierFreq = map(Pot1Val, 0, 1023, MinCarrierFreq, MaxCarrierFreq);
  ModSpeed = (float)map(Pot2Val, 0, 1023, MinModSpeed, MaxModSpeed)/1000; // Use a float for the LFO sub 1hz frequencies
  FreqShift = map(Pot3Val, 0, 1023, MinFreqShift, MaxFreqShift);
  Intensity = map(Pot4Val, 0, 1023, MinIntensity, MaxIntensity);
  IntensityMultiplier = map(Pot5Val, 0, 1023, MinIntensityMultiplier, MaxIntensityMultiplier);

  // Print Debug values
  Serial.print("CarrierFreq = ");
  Serial.print(CarrierFreq);
  Serial.print("   ModSpeed = ");
  Serial.print(ModSpeed);
  Serial.print("   FreqShift = ");
  Serial.print(FreqShift);
  Serial.print("   Intensity = ");
  Serial.print(Intensity);
  Serial.print("   IntensityMultiplier = ");
  Serial.print(IntensityMultiplier);
  Serial.println(); // Carriage return
  
  //calculate the modulation frequency to stay in ratio
  int mod_freq = CarrierFreq * mod_ratio * FreqShift;
  
  // set the FM oscillator frequencies
  aCarrier.setFreq(CarrierFreq); 
  aModulator.setFreq(mod_freq);
  
 // calculate the fm_intensity
  fm_intensity = ((long)Intensity * IntensityMultiplier * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply
  kIntensityMod.setFreq(ModSpeed);
}

int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  return aCarrier.phMod(modulation);
}

void loop(){
  audioHook();
}
