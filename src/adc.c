
/*
    adc stuff
    gain correction handled in the I2C file.
    This because of the multiplexer control handled there
*/
#include "adc.h"
#include "global.h"
#include <esp_adc_cal.h>
#include <esp_log.h>
#include <math.h>

#define TAG "ADC"

#define DEFAULT_VREF 1100 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  // Multisampling
static const adc_atten_t atten = ADC_ATTEN_DB_2_5;
// static const adc_atten_t atten = ADC_ATTEN_DB_11;
// static const adc_atten_t atten = ADC_ATTEN_DB_6;
static const adc_channel_t channelFAN = ADC_CHANNEL_3; // ADC1 CH2
static const adc_channel_t channelMUX = ADC_CHANNEL_3; // ADC1 CH3
const int vref = 2.495;
static esp_adc_cal_characteristics_t adc_chars;

void adc_config(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL_3, atten);
}

float AdcFanRaw(void) {
    return adc1_get_raw((adc1_channel_t)channelFAN); // result in mV
}
float AdcFan(void) {
    return adc1_get_raw((adc1_channel_t)channelFAN) * 422 * 3.5 / 4095; // result in mV
}
float AdcSamp2(void) {
    return adc1_get_raw((adc1_channel_t)channelMUX) * 347 * 3.5 / 4095; // result in mV
}

float AdcSampRaw(void) { return adc1_get_raw((adc1_channel_t)channelMUX); }

float map(float x, float in_min, float in_max, float out_min, float out_max) {
    x = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (x < out_min)
        return out_min;
    if (x > out_max)
        return out_max;
    return x;
}

float ReadVoltage(float reading) { // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
    if (reading < 1 || reading > 4095)
        return 0;
    return -0.000000000009824 * pow(reading, 3) + 0.000000016557283 * pow(reading, 2) + 0.000854596860691 * reading + 0.065440348345433;
    // return -0.000000000000016 * pow(reading,4) + 0.000000000118171 * pow(reading,3)- 0.000000301211691 * pow(reading,2)+ 0.001109019271794 * reading + 0.034143524634089;
    // return reading;
    // https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function/blob/master/ESP32_ADC_Read_Voltage_Accurate.ino
    // https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function
    // https://www.youtube.com/watch?v=RlKMJknsNpo&t=145s
} // Added an improved polynomial, use either, comment out as required

void CalibrateV(void) {
    char ch;
    ESP_LOGW(TAG, "Put 5V loop in P1 and hit a key");
    ch = fgetc(stdin);
    while (ch == 0xFF) {
        ch = fgetc(stdin);
    }
    ESP_LOGW(TAG, "Calibrating P1 to 5Volt");
    ESP_LOGI(TAG, "Wait");
    for (ch = 0; ch < 10; ch++) {
        vTaskDelay(100);
        ESP_LOGI(TAG, ".");
    }
    ESP_LOGI(TAG, "Old NVMVgain %f", NVMsystem.NVMVgain);
    NVMsystem.NVMVgain = 5000 / (float)ADC[0];
    ESP_LOGI(TAG, "New NVMVgain %f", NVMsystem.NVMVgain);
    ESP_LOGW(TAG, "Store new parametes? y\\n ...");
    ch = fgetc(stdin);
    while (ch == 0xFF) {
        ch = fgetc(stdin);
    }
    if (ch == 'y' || ch == 'Y') {
        store_struckt_name("NVMsystem", &NVMsystem, sizeof(NVMsystem));
    }
    ch = 0xFF;
    ESP_LOGI(TAG, "Calibration done!!!");
}

int read_adc_256() {
    const int OVERSAMPLE_COUNT = 8; // 2^n, 1..256
    static int buf[8];

    int64_t sum = 0;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
        buf[i] = adc1_get_raw(ADC1_CHANNEL_3); // gpio39, sensor vn
        sum += buf[i];
        // vTaskDelay(1); // 1ms apart, so 8ms sample time
    }
    int avg = sum * (256 / OVERSAMPLE_COUNT); // just a shift
    return avg;
}
// input is adc counts*256
// output is mv*10
// this is the voltage on the mux inputs
// it is divided/2 before it goes to the adc, but that is all calibrated away
static float A;
static float B;

void calibrateScaleX(int countl, int counth) {
    const float refl = 3811.8;          // 0,382
    const float refh = (24950.0) / 2.0; // 1,25
    A = (refh - refl) / (counth - countl);
    B = refh - A * counth;
    // ESP_LOGW(TAG, "Calibrate %d-%d A=%f, B=%f", countl, counth, A, B);
}

int scaleX(int avg) { return A * (float)avg + B; }

int convert_adc256_to_mv(int val) { return esp_adc_cal_raw_to_voltage((val + 128) / 256, &adc_chars); }
