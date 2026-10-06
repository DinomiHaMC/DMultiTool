#pragma once
#include <cstdint>
constexpr int OUTPUT=1,LOW=0;
inline uint32_t testToneMillis=0;inline int toneFrequency=0,toneStarts=0,toneStops=0,toneChannel=-1;
inline uint32_t millis(){return testToneMillis;}
inline void setToneChannel(int channel){toneChannel=channel;}
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
inline void tone(int,uint16_t frequency){toneFrequency=frequency;toneStarts++;}
inline void noTone(int){toneFrequency=0;toneStops++;}
