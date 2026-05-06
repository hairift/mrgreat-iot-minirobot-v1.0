#ifndef __MOVEMENTS_H__
#define __MOVEMENTS_H__

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oscillator.h"

//-- Konstanta
#define FORWARD 1
#define BACKWARD -1
#define LEFT 1
#define RIGHT -1
#define BOTH 0
#define SMALL 5
#define MEDIUM 15
#define BIG 30

// -- Batas perubahan servo bawaan, derajat per detik
#define SERVO_LIMIT_DEFAULT 240

// -- Indeks servo agar mudah diakses
#define RIGHT_PITCH 0
#define RIGHT_ROLL 1
#define LEFT_PITCH 2
#define LEFT_ROLL 3
#define BODY 4
#define HEAD 5
#define SERVO_COUNT 6

class Otto {
public:
    Otto();
    ~Otto();

    //-- Inisialisasi Otto
    void Init(int right_pitch, int right_roll, int left_pitch, int left_roll, int body, int head);
    //-- Fungsi pasang dan lepas
    void AttachServos();
    void DetachServos();

    //-- Trim osilator
    void SetTrims(int right_pitch, int right_roll, int left_pitch, int left_roll, int body,
                  int head);

    //-- Fungsi gerak dasar
    void MoveServos(int time, int servo_target[]);
    void MoveSingle(int position, int servo_number);
    void OscillateServos(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT], int period,
                         double phase_diff[SERVO_COUNT], float cycle);

    //-- Posisi istirahat Otto
    void Home(bool hands_down = true);
    bool GetRestState();
    void SetRestState(bool state);

// Aksi tangan
    void HandAction(int action, int times = 1, int amount = 30, int period = 1000);
    // action: 1=angkat tangan kiri, 2=angkat tangan kanan, 3=angkat kedua tangan, 4=turunkan tangan kiri,
    // 5=turunkan tangan kanan, 6=turunkan kedua tangan, 7=lambaikan tangan kiri, 8=lambaikan tangan kanan,
    // 9=lambaikan kedua tangan, 10=tepuk tangan kiri, 11=tepuk tangan kanan, 12=tepuk kedua tangan

    // Aksi badan
    void BodyAction(int action, int times = 1, int amount = 30, int period = 1000);
    // action: 1=putar kiri, 2=putar kanan

    // Aksi kepala
    void HeadAction(int action, int times = 1, int amount = 10, int period = 500);
    // action: 1=angkat kepala, 2=tundukkan kepala, 3=angguk, 4=kembali ke tengah, 5=angguk berulang

private:
    Oscillator servo_[SERVO_COUNT];

    int servo_pins_[SERVO_COUNT];
    int servo_trim_[SERVO_COUNT];
    int servo_initial_[SERVO_COUNT] = {180, 180, 0, 0, 90, 90};

    unsigned long final_time_;
    unsigned long partial_time_;
    float increment_[SERVO_COUNT];

    bool is_otto_resting_;

    void Execute(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT], int period,
                 double phase_diff[SERVO_COUNT], float steps);
};

#endif  // __MOVEMENTS_H__
