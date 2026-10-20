#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// AI04 V1.4 (davinci-ai04-c3) — pins read from the factory firmware.

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_8
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_5
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_7
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_4

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_10
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_0
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_1
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

#define LED_STRIP_GPIO          GPIO_NUM_2
#define LED_STRIP_COUNT         6

#define FUNCTION_BUTTON_GPIO    GPIO_NUM_13   // 功能
#define POWER_BUTTON_GPIO       GPIO_NUM_20   // 开机
#define POWER_HOLD_GPIO         GPIO_NUM_21   // keeps the battery power switch on
#define AUX_ENABLE_GPIO         GPIO_NUM_9    // driven high by the factory firmware

#define BATTERY_ADC_UNIT        ADC_UNIT_1
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_3
#define BATTERY_CHARGING_GPIO   GPIO_NUM_12

#endif // _BOARD_CONFIG_H_
