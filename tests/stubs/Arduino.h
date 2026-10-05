#pragma once
#include <cstddef>
#include <cstdint>
constexpr int INPUT=0, ADC_11db=3;
extern uint32_t testMillis;
extern int testAdc;
inline uint32_t millis(){return testMillis;}
inline int analogRead(int){return testAdc;}
inline void pinMode(int,int){}
inline void analogReadResolution(int){}
inline void analogSetPinAttenuation(int,int){}

#include <string>
using String=std::string;
