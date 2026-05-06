#include "eda_dog_movements.h"

#include <algorithm>
#include <cmath>

#include "oscillator.h"

static const char *TAG = "EDARobotDogMovements";

#define LEG_HOME_POSITION 90

EDARobotDog::EDARobotDog() {
  is_dog_resting_ = false;
  // Inisialisasi semua pin servo ke -1 (belum terhubung)
  for (int i = 0; i < SERVO_COUNT; i++) {
    servo_pins_[i] = -1;
    servo_trim_[i] = 0;
  }
}

EDARobotDog::~EDARobotDog() { DetachServos(); }

unsigned long IRAM_ATTR millis() {
  return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void EDARobotDog::Init(int left_front_leg, int left_rear_leg, int right_front_leg,
                  int right_rear_leg) {
  servo_pins_[LEFT_FRONT_LEG] = left_front_leg;
  servo_pins_[LEFT_REAR_LEG] = left_rear_leg;
  servo_pins_[RIGHT_FRONT_LEG] = right_front_leg;
  servo_pins_[RIGHT_REAR_LEG] = right_rear_leg;

  AttachServos();
  is_dog_resting_ = false;
}

///////////////////////////////////////////////////////////////////
//-- FUNGSI PASANG DAN LEPAS ------------------------------------//
///////////////////////////////////////////////////////////////////
void EDARobotDog::AttachServos() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      servo_[i].Attach(servo_pins_[i]);
    }
  }
}

void EDARobotDog::DetachServos() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      servo_[i].Detach();
    }
  }
}

///////////////////////////////////////////////////////////////////
//-- TRIM OSILATOR ----------------------------------------------//
///////////////////////////////////////////////////////////////////
void EDARobotDog::SetTrims(int left_front_leg, int left_rear_leg,
                      int right_front_leg, int right_rear_leg) {
  servo_trim_[LEFT_FRONT_LEG] = left_front_leg;
  servo_trim_[LEFT_REAR_LEG] = left_rear_leg;
  servo_trim_[RIGHT_FRONT_LEG] = right_front_leg;
  servo_trim_[RIGHT_REAR_LEG] = right_rear_leg;

  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      servo_[i].SetTrim(servo_trim_[i]);
    }
  }
}

///////////////////////////////////////////////////////////////////
//-- FUNGSI GERAK DASAR -----------------------------------------//
///////////////////////////////////////////////////////////////////
void EDARobotDog::MoveServos(int time, int servo_target[]) {
  if (GetRestState() == true) {
    SetRestState(false);
  }

  final_time_ = millis() + time;
  if (time > 10) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (servo_pins_[i] != -1) {
        increment_[i] =
            (servo_target[i] - servo_[i].GetPosition()) / (time / 10.0);
      }
    }

    for (int iteration = 1; millis() < final_time_; iteration++) {
      partial_time_ = millis() + 10;
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          servo_[i].SetPosition(servo_[i].GetPosition() + increment_[i]);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  } else {
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (servo_pins_[i] != -1) {
        servo_[i].SetPosition(servo_target[i]);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(time));
  }

  // Penyesuaian akhir ke posisi target.
  bool f = true;
  int adjustment_count = 0;
  while (f && adjustment_count < 10) {
    f = false;
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (servo_pins_[i] != -1 && servo_target[i] != servo_[i].GetPosition()) {
        f = true;
        break;
      }
    }
    if (f) {
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          servo_[i].SetPosition(servo_target[i]);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      adjustment_count++;
    }
  };
}

void EDARobotDog::MoveSingle(int position, int servo_number) {
  if (position > 180)
    position = 90;
  if (position < 0)
    position = 90;

  if (GetRestState() == true) {
    SetRestState(false);
  }

  if (servo_number >= 0 && servo_number < SERVO_COUNT &&
      servo_pins_[servo_number] != -1) {
    servo_[servo_number].SetPosition(position);
  }
}

void EDARobotDog::OscillateServos(int amplitude[SERVO_COUNT],
                             int offset[SERVO_COUNT], int period,
                             double phase_diff[SERVO_COUNT], float cycle = 1) {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      servo_[i].SetO(offset[i]);
      servo_[i].SetA(amplitude[i]);
      servo_[i].SetT(period);
      servo_[i].SetPh(phase_diff[i]);
    }
  }

  double ref = millis();
  double end_time = period * cycle + ref;

  while (millis() < end_time) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (servo_pins_[i] != -1) {
        servo_[i].Refresh();
      }
    }
    vTaskDelay(5);
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}

void EDARobotDog::Execute(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT],
                     int period, double phase_diff[SERVO_COUNT],
                     float steps = 1.0) {
  if (GetRestState() == true) {
    SetRestState(false);
  }

  int cycles = (int)steps;

  //-- Jalankan siklus penuh
  if (cycles >= 1)
    for (int i = 0; i < cycles; i++)
      OscillateServos(amplitude, offset, period, phase_diff);

  //-- Jalankan sisa siklus yang belum penuh
  OscillateServos(amplitude, offset, period, phase_diff, (float)steps - cycles);
  vTaskDelay(pdMS_TO_TICKS(10));
}

///////////////////////////////////////////////////////////////////
//-- POSISI ISTIRAHAT ANJING ROBOT ------------------------------//
///////////////////////////////////////////////////////////////////
void EDARobotDog::Home() {
  if (is_dog_resting_ == false) { // Hanya kembali ke posisi istirahat jika perlu
    int homes[SERVO_COUNT] = {LEG_HOME_POSITION, LEG_HOME_POSITION,
                              LEG_HOME_POSITION, LEG_HOME_POSITION};
    MoveServos(500, homes);
    is_dog_resting_ = true;
  }
  vTaskDelay(pdMS_TO_TICKS(200));
}

bool EDARobotDog::GetRestState() { return is_dog_resting_; }

void EDARobotDog::SetRestState(bool state) { is_dog_resting_ = state; }

///////////////////////////////////////////////////////////////////
//-- GERAK DASAR KAKI -------------------------------------------//
///////////////////////////////////////////////////////////////////

void EDARobotDog::LiftLeftFrontLeg(int period, int height) {

  // Ambil posisi saat ini
  int current_pos[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      current_pos[i] = servo_[i].GetPosition();
    } else {
      current_pos[i] = LEG_HOME_POSITION;
    }
  }

  // Ulangi gerakan ayun 3 kali
  for (int num = 0; num < 3; num++) {
    // Gerakkan servo target ke posisi 0
    current_pos[LEFT_FRONT_LEG] = 0; // servo1
    MoveServos(100, current_pos);

    // Gerakkan servo target ke posisi 30
    current_pos[LEFT_FRONT_LEG] = 30; // servo1
    MoveServos(100, current_pos);
  }

  // Kembalikan servo target ke posisi 90
  current_pos[LEFT_FRONT_LEG] = 90; // servo1
  MoveServos(100, current_pos);
}

void EDARobotDog::LiftLeftRearLeg(int period, int height) {

  // Ambil posisi saat ini
  int current_pos[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      current_pos[i] = servo_[i].GetPosition();
    } else {
      current_pos[i] = LEG_HOME_POSITION;
    }
  }

  // Ulangi gerakan ayun 3 kali
  for (int num = 0; num < 3; num++) {
    // Gerakkan servo target ke posisi 180
    current_pos[LEFT_REAR_LEG] = 180; // servo1
    MoveServos(100, current_pos);

    // Gerakkan servo target ke posisi 150
    current_pos[LEFT_REAR_LEG] = 150; // servo1
    MoveServos(100, current_pos);
  }

  // Kembalikan servo target ke posisi 90
  current_pos[LEFT_REAR_LEG] = 90; // servo1
  MoveServos(100, current_pos);
}

void EDARobotDog::LiftRightFrontLeg(int period, int height) {
  
  // Ambil posisi saat ini
  int current_pos[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      current_pos[i] = servo_[i].GetPosition();
    } else {
      current_pos[i] = LEG_HOME_POSITION;
    }
  }

  // Ulangi gerakan ayun 3 kali
  for (int num = 0; num < 3; num++) {
    // Gerakkan servo target ke posisi 180
    current_pos[RIGHT_FRONT_LEG] = 180; // servo1
    MoveServos(100, current_pos);

    // Gerakkan servo target ke posisi 150
    current_pos[RIGHT_FRONT_LEG] = 150; // servo1
    MoveServos(100, current_pos);
  }

  // Kembalikan servo target ke posisi 90
  current_pos[RIGHT_FRONT_LEG] = 90; // servo1
  MoveServos(100, current_pos);
}

void EDARobotDog::LiftRightRearLeg(int period, int height) {

  // Ambil posisi saat ini
  int current_pos[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      current_pos[i] = servo_[i].GetPosition();
    } else {
      current_pos[i] = LEG_HOME_POSITION;
    }
  }

  // Ulangi gerakan ayun 3 kali
  for (int num = 0; num < 3; num++) {
    // Gerakkan servo target ke posisi 0
    current_pos[RIGHT_REAR_LEG] = 0; // servo1
    MoveServos(100, current_pos);

    // Gerakkan servo target ke posisi 30
    current_pos[RIGHT_REAR_LEG] = 30; // servo1
    MoveServos(100, current_pos);
  }

  // Kembalikan servo target ke posisi 90
  current_pos[RIGHT_FRONT_LEG] = 90; // servo1
  MoveServos(100, current_pos);
}

///////////////////////////////////////////////////////////////////
//-- GERAK LANGKAH ANJING ---------------------------------------//
///////////////////////////////////////////////////////////////////

void EDARobotDog::Turn(float steps, int period, int dir) {

  if (GetRestState() == true) {
    SetRestState(false);
  }

  for (int step = 0; step < (int)steps; step++) {
    if (dir == LEFT) {

      int current_pos[SERVO_COUNT];
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          current_pos[i] = servo_[i].GetPosition();
        } else {
          current_pos[i] = LEG_HOME_POSITION;
        }
      }

      current_pos[RIGHT_REAR_LEG] = 140; // servo3
      current_pos[LEFT_REAR_LEG] = 40;   // servo2
      MoveServos(100, current_pos);

      current_pos[RIGHT_FRONT_LEG] = 40; // servo4
      current_pos[LEFT_FRONT_LEG] = 140; // servo1
      MoveServos(100, current_pos);

      current_pos[RIGHT_REAR_LEG] = 90; // servo3
      current_pos[LEFT_REAR_LEG] = 90;  // servo2
      MoveServos(100, current_pos);

      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      MoveServos(100, current_pos);


      current_pos[RIGHT_FRONT_LEG] = 140; // servo4
      current_pos[LEFT_FRONT_LEG] = 40;   // servo1
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 40; // servo3
      current_pos[LEFT_REAR_LEG] = 140; // servo2
      MoveServos(100, current_pos);


      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 90; // servo3
      current_pos[LEFT_REAR_LEG] = 90;  // servo2
      MoveServos(100, current_pos);
      

    } else {

// Ambil posisi saat ini
      int current_pos[SERVO_COUNT];
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          current_pos[i] = servo_[i].GetPosition();
        } else {
          current_pos[i] = LEG_HOME_POSITION;
        }
      }


      current_pos[LEFT_REAR_LEG] = 140; // servo2
      current_pos[RIGHT_REAR_LEG] = 40; // servo3
      MoveServos(100, current_pos);

      current_pos[LEFT_FRONT_LEG] = 40;   // servo1
      current_pos[RIGHT_FRONT_LEG] = 140; // servo4
      MoveServos(100, current_pos);

      current_pos[LEFT_REAR_LEG] = 90;  // servo2
      current_pos[RIGHT_REAR_LEG] = 90; // servo3
      MoveServos(100, current_pos);

      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 140; // servo1
      current_pos[RIGHT_FRONT_LEG] = 40; // servo4
      MoveServos(100, current_pos);


      current_pos[LEFT_REAR_LEG] = 40;   // servo2
      current_pos[RIGHT_REAR_LEG] = 140; // servo3
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      MoveServos(100, current_pos);


      current_pos[LEFT_REAR_LEG] = 90;  // servo2
      current_pos[RIGHT_REAR_LEG] = 90; // servo3
      MoveServos(100, current_pos);

    }
  }
}

void EDARobotDog::Walk(float steps, int period, int dir) {

  if (GetRestState() == true) {
    SetRestState(false);
  }


  for (int step = 0; step < (int)steps; step++) {
    if (dir == FORWARD) {

// Ambil posisi saat ini
      int current_pos[SERVO_COUNT];
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          current_pos[i] = servo_[i].GetPosition();
        } else {
          current_pos[i] = LEG_HOME_POSITION;
        }
      }

  
      current_pos[LEFT_FRONT_LEG] = 100;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 100; // servo4
      MoveServos(100, current_pos);

      
      current_pos[RIGHT_REAR_LEG] = 60; // servo3
      current_pos[LEFT_REAR_LEG] = 60;  // servo2
      MoveServos(100, current_pos);

   
      current_pos[LEFT_FRONT_LEG] = 140;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 140; // servo4
      MoveServos(100, current_pos);

    
      current_pos[RIGHT_REAR_LEG] = 40; // servo3
      current_pos[LEFT_REAR_LEG] = 40;  // servo2
      MoveServos(100, current_pos);

 

      current_pos[RIGHT_REAR_LEG] = 90;  // servo3
      current_pos[LEFT_REAR_LEG] = 90;   // servo2
      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 80;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 80; // servo4
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 120; // servo3
      current_pos[LEFT_REAR_LEG] = 120;  // servo2
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 140; // servo3
      current_pos[LEFT_REAR_LEG] = 140;  // servo2
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 90; // servo3
      current_pos[LEFT_REAR_LEG] = 90;  // servo2
      MoveServos(100, current_pos);
    } else {

      // Gerakkan servo terkait secara bertahap, lalu jeda 100 ms
      // Ambil posisi saat ini
      int current_pos[SERVO_COUNT];
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          current_pos[i] = servo_[i].GetPosition();
        } else {
          current_pos[i] = LEG_HOME_POSITION;
        }
      }


      current_pos[LEFT_FRONT_LEG] = 80;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 80; // servo4
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 120; // servo3
      current_pos[LEFT_REAR_LEG] = 120;  // servo2
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 40;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 40; // servo4
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 140; // servo3
      current_pos[LEFT_REAR_LEG] = 140;  // servo2
      MoveServos(100, current_pos);

      current_pos[RIGHT_REAR_LEG] = 90;  // servo3
      current_pos[LEFT_REAR_LEG] = 90;   // servo2
      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 100;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 100; // servo4
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 60; // servo3
      current_pos[LEFT_REAR_LEG] = 60;  // servo2
      MoveServos(100, current_pos);


      current_pos[LEFT_FRONT_LEG] = 90;  // servo1
      current_pos[RIGHT_FRONT_LEG] = 90; // servo4
      MoveServos(100, current_pos);

      current_pos[RIGHT_REAR_LEG] = 40; // servo3
      current_pos[LEFT_REAR_LEG] = 40;  // servo2
      MoveServos(100, current_pos);


      current_pos[RIGHT_REAR_LEG] = 90; // servo3
      current_pos[LEFT_REAR_LEG] = 90;  // servo2
      MoveServos(100, current_pos);
      
    }
  }
}

void EDARobotDog::Sit(int period) {


int current_pos[SERVO_COUNT];
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          current_pos[i] = servo_[i].GetPosition();
        } else {
          current_pos[i] = LEG_HOME_POSITION;
        }
      }


  current_pos[LEFT_REAR_LEG] = 0;  // servo2
  current_pos[RIGHT_REAR_LEG] = 180; // servo4
  MoveServos(100, current_pos);
}

void EDARobotDog::Stand(int period) {
  // Berdiri: semua kaki kembali ke posisi netral
  Home();
}

void EDARobotDog::Stretch(int period) {


  // Ambil posisi saat ini
int current_pos[SERVO_COUNT];
      for (int i = 0; i < SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
          current_pos[i] = servo_[i].GetPosition();
        } else {
          current_pos[i] = LEG_HOME_POSITION;
        }
      }


  current_pos[LEFT_FRONT_LEG] = 0;  // servo1
  current_pos[RIGHT_REAR_LEG] = 0;    // servo3
  current_pos[LEFT_REAR_LEG] = 180;     // servo2
  current_pos[RIGHT_FRONT_LEG] = 180; // servo4
  MoveServos(100, current_pos);
}

void EDARobotDog::Shake(int period) {
  // Ayun: tubuh bergerak ke kiri dan kanan, kaki depan kiri dan kaki belakang kanan berlawanan arah
  int A[SERVO_COUNT] = {20, 0, 20, 0}; // Hanya kaki depan yang berayun
  int O[SERVO_COUNT] = {0, LEG_HOME_POSITION, 0, LEG_HOME_POSITION};
  // Kaki depan kiri dan kaki depan kanan berayun berlawanan fase
  double phase_diff[SERVO_COUNT] = {DEG2RAD(180), 0, DEG2RAD(0), 0};

  Execute(A, O, period, phase_diff, 3);
}

void EDARobotDog::EnableServoLimit(int diff_limit) {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      servo_[i].SetLimiter(diff_limit);
    }
  }
}

void EDARobotDog::DisableServoLimit() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      servo_[i].DisableLimiter();
    }
  }
}

void EDARobotDog::Sleep() {

  int current_pos[SERVO_COUNT];
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (servo_pins_[i] != -1) {
      current_pos[i] = servo_[i].GetPosition();
    } else {
      current_pos[i] = LEG_HOME_POSITION;
    }
  }

  // Atur posisi tidur untuk keempat servo utama
  current_pos[LEFT_FRONT_LEG] = 0;   // servo1
  current_pos[RIGHT_REAR_LEG] = 180; // servo3
  current_pos[LEFT_REAR_LEG] = 180;  // servo2
  current_pos[RIGHT_FRONT_LEG] = 0;  // servo4
  MoveServos(100, current_pos);
}
