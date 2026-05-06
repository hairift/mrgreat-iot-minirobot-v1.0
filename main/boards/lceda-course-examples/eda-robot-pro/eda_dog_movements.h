#ifndef __EDA_DOG_MOVEMENTS_H__
#define __EDA_DOG_MOVEMENTS_H__

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
#define SMALL 5
#define MEDIUM 15
#define BIG 30

// -- Batas perubahan servo bawaan, derajat per detik
#define SERVO_LIMIT_DEFAULT 240

// -- Indeks servo agar mudah diakses
#define LEFT_FRONT_LEG 0
#define LEFT_REAR_LEG 1
#define RIGHT_FRONT_LEG 2
#define RIGHT_REAR_LEG 3
#define SERVO_COUNT 4

class EDARobotDog {
public:
    EDARobotDog();
    ~EDARobotDog();

    //-- Inisialisasi anjing robot EDA
    void Init(int left_front_leg, int left_rear_leg, int right_front_leg, int right_rear_leg);
    
    //-- Fungsi pasang dan lepas
    void AttachServos();
    void DetachServos();

    //-- Trim osilator
    void SetTrims(int left_front_leg, int left_rear_leg, int right_front_leg, int right_rear_leg);

    //-- Fungsi gerak dasar
    void MoveServos(int time, int servo_target[]);
    void MoveSingle(int position, int servo_number);
    void OscillateServos(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT], int period,
                         double phase_diff[SERVO_COUNT], float cycle);

    //-- Posisi istirahat anjing robot
    void Home();
    bool GetRestState();
    void SetRestState(bool state);

    //-- Gerakan dasar kaki
    void LiftLeftFrontLeg(int period = 1000, int height = 45);   // Angkat kaki depan kiri
    void LiftLeftRearLeg(int period = 1000, int height = 45);    // Angkat kaki belakang kiri
    void LiftRightFrontLeg(int period = 1000, int height = 45);  // Angkat kaki depan kanan
    void LiftRightRearLeg(int period = 1000, int height = 45);   // Angkat kaki belakang kanan

    //-- Gerakan langkah anjing
    void Walk(float steps = 4, int period = 1000, int dir = FORWARD);
    void Turn(float steps = 4, int period = 2000, int dir = LEFT);
    void Sit(int period = 1500);
    void Stand(int period = 1500);
    void Stretch(int period = 2000);
    void Shake(int period = 1000);
    void Sleep();  // Gerakan tidur

    // -- Pembatas servo
    void EnableServoLimit(int speed_limit_degree_per_sec = SERVO_LIMIT_DEFAULT);
    void DisableServoLimit();

private:
    Oscillator servo_[SERVO_COUNT];

    int servo_pins_[SERVO_COUNT];
    int servo_trim_[SERVO_COUNT];

    unsigned long final_time_;
    unsigned long partial_time_;
    float increment_[SERVO_COUNT];

    bool is_dog_resting_;

    void Execute(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT], int period,
                 double phase_diff[SERVO_COUNT], float steps);
};

#endif  // __EDA_DOG_MOVEMENTS_H__
