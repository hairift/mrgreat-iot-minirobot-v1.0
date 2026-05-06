//--------------------------------------------------------------
//-- Oscillator.pde
//-- Membuat osilasi sinusoidal pada servo
//--------------------------------------------------------------
//-- (c) Juan Gonzalez-Gomez (Obijuan), Des 2011
//-- (c) txp666 untuk esp32, 202503
//-- Lisensi GPL
//--------------------------------------------------------------
#ifndef __OSCILLATOR_H__
#define __OSCILLATOR_H__

#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define M_PI 3.14159265358979323846

#ifndef DEG2RAD
#define DEG2RAD(g) ((g) * M_PI) / 180
#endif

#define SERVO_MIN_PULSEWIDTH_US 500           // Lebar pulsa minimum (mikrodetik)
#define SERVO_MAX_PULSEWIDTH_US 2500          // Lebar pulsa maksimum (mikrodetik)
#define SERVO_MIN_DEGREE -90                  // Sudut minimum
#define SERVO_MAX_DEGREE 90                   // Sudut maksimum
#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000  // 1 MHz, 1 us per tick
#define SERVO_TIMEBASE_PERIOD 20000           // 20000 tick, 20 ms

class Oscillator {
public:
    Oscillator(int trim = 0);
    ~Oscillator();
    void Attach(int pin, bool rev = false);
    void Detach();

    void SetA(unsigned int amplitude) { amplitude_ = amplitude; };
    void SetO(int offset) { offset_ = offset; };
    void SetPh(double Ph) { phase0_ = Ph; };
    void SetT(unsigned int period);
    void SetTrim(int trim) { trim_ = trim; };
    void SetLimiter(int diff_limit) { diff_limit_ = diff_limit; };
    void DisableLimiter() { diff_limit_ = 0; };
    int GetTrim() { return trim_; };
    void SetPosition(int position);
    void Stop() { stop_ = true; };
    void Play() { stop_ = false; };
    void Reset() { phase_ = 0; };
    void Refresh();
    int GetPosition() { return pos_; }

private:
    bool NextSample();
    void Write(int position);
    uint32_t AngleToCompare(int angle);

private:
    bool is_attached_;

    //-- Parameter osilator
    unsigned int amplitude_;  //-- Amplitudo (derajat)
    int offset_;              //-- Offset (derajat)
    unsigned int period_;     //-- Periode (milidetik)
    double phase0_;           //-- Fase (radian)

    //-- Variabel internal
    int pos_;                       //-- Posisi servo saat ini
    int pin_;                       //-- Pin tempat servo terhubung
    int trim_;                      //-- Offset kalibrasi
    double phase_;                  //-- Fase saat ini
    double inc_;                    //-- Pertambahan fase
    double number_samples_;         //-- Jumlah sampel
    unsigned int sampling_period_;  //-- Periode sampling (ms)

    long previous_millis_;
    long current_millis_;

    //-- Mode osilasi. Jika true, servo berhenti
    bool stop_;

    //-- Mode terbalik
    bool rev_;

    int diff_limit_;
    long previous_servo_command_millis_;

    ledc_channel_t ledc_channel_;
    ledc_mode_t ledc_speed_mode_;
};

#endif  // __OSCILLATOR_H__
