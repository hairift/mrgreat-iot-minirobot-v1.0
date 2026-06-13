#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// I2S simpleks (mikrofon dan speaker terpisah)
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

// Mikrofon INMP441
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6

// Speaker MAX98357
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

#else

#define AUDIO_I2S_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_7

#endif


#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_NC
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

// Tombol sakelar taktil harian untuk membangunkan AI atau mengganti status percakapan.
// Sambungkan satu kaki tombol ke GPIO1 dan kaki sisi seberangnya ke GND.
// Untuk tombol taktil 4 kaki, jangan memakai kiri atas + kiri bawah atau kanan atas + kanan bawah
// jika kedua kaki itu sudah terhubung permanen. Pakai sisi berseberangan, misalnya kiri atas + kanan atas.
// GPIO2 dipakai untuk ADC baterai, jadi jangan dipakai lagi sebagai tombol.
#define ASR_BUTTON_GPIO         GPIO_NUM_1

// USB bawaan pada ESP32-S3:
// D- = GPIO19 dan D+ = GPIO20 dipakai untuk data USB.
// Jalur ini bukan pembaca baterai, bukan tombol umum, dan tidak dapat menyalakan board tanpa jalur daya.

// OLED SSD1306 0.96" (I2C)
// SDA=G10, SCK(SCL)=G9
#define DISPLAY_SDA_PIN GPIO_NUM_10
#define DISPLAY_SCL_PIN GPIO_NUM_9
#define DISPLAY_WIDTH   128

#if CONFIG_OLED_SSD1306_128X32
#define DISPLAY_HEIGHT  32
#elif CONFIG_OLED_SSD1306_128X64
#define DISPLAY_HEIGHT  64
#elif CONFIG_OLED_SH1106_128X64
#define DISPLAY_HEIGHT  64
#define SH1106
#else
#error "OLED display type is not selected"
#endif

#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y true


// Uji MCP: kendalikan lampu
#define LAMP_GPIO GPIO_NUM_18

// Pin kendali servo (servo mikro SG90 x3)
#define SERVO_HEAD_GPIO     GPIO_NUM_17   // kepala
#define SERVO_RARM_GPIO     GPIO_NUM_38   // tangan kanan
#define SERVO_LARM_GPIO     GPIO_NUM_39   // tangan kiri

// Pembacaan baterai:
// Pakai pembagi tegangan 100 kOhm + 100 kOhm, lalu kapasitor 104 (100 nF)
// dari titik ADC ke GND. Jalur ADC mengikuti rangkaian terbaru di GPIO2.
// Untuk membaca baterai asli, masukan pembagi sebaiknya dari baterai/TP4056 OUT+
// atau MT3608 IN+, bukan dari keluaran boost 5V.
#define BATTERY_ADC_GPIO    GPIO_NUM_2
#define BATTERY_ADC_UNIT    ADC_UNIT_1
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_1
#define BATTERY_CHARGING_GPIO GPIO_NUM_NC
#define BATTERY_R_UPPER     100000.0f    // resistor atas (ohm)
#define BATTERY_R_LOWER     100000.0f    // resistor bawah (ohm)

#endif // _BOARD_CONFIG_H_
