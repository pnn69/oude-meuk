/*
 * adc.h
 *
 */
#ifndef ADC_H_
#define ADC_H_
#include <stdbool.h>

float map(float x, float in_min, float in_max, float out_min, float out_max);
float ReadVoltage(float reading);
void CalibrateV(void);
int read_adc_256(void);

float voltageFAN;
float voltageRH;
float voltageHEAT;
float PressHigh;
float PressLow;
bool PressLowAlarm;
bool PressHighAlarm;
bool WaterAlarm;
bool DayNight; // Day=true night=false
int ADC[15];
float ADCmV[8];
void adc_config(void);
void init_adc(void);
void adc_task(void *parameter);
float AdcFanRaw(void);
float AdcFan(void);
float AdcSamp2(void);
float AdcSampRaw(void);
void calibrateScaleX(int countl, int counth);
int scaleX(int avg);
int convert_adc256_to_mv(int val);

#endif /* ADC_H_ */
