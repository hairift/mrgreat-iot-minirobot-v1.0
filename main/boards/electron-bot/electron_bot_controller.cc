/*
    Pengendali robot Electron Bot - versi protokol MCP
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "movements.h"
#include "sdkconfig.h"
#include "settings.h"

#define TAG "ElectronBotController"

struct ElectronBotActionParams {
    int action_type;
    int steps;
    int speed;
    int direction;
    int amount;
};

class ElectronBotController {
private:
    Otto electron_bot_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool is_action_in_progress_ = false;

    enum ActionType {
        // Aksi tangan 1-12
        ACTION_HAND_LEFT_UP = 1,      // Angkat tangan kiri
        ACTION_HAND_RIGHT_UP = 2,     // Angkat tangan kanan
        ACTION_HAND_BOTH_UP = 3,      // Angkat kedua tangan
        ACTION_HAND_LEFT_DOWN = 4,    // Turunkan tangan kiri
        ACTION_HAND_RIGHT_DOWN = 5,   // Turunkan tangan kanan
        ACTION_HAND_BOTH_DOWN = 6,    // Turunkan kedua tangan
        ACTION_HAND_LEFT_WAVE = 7,    // Lambaikan tangan kiri
        ACTION_HAND_RIGHT_WAVE = 8,   // Lambaikan tangan kanan
        ACTION_HAND_BOTH_WAVE = 9,    // Lambaikan kedua tangan
        ACTION_HAND_LEFT_FLAP = 10,   // Tepukkan tangan kiri
        ACTION_HAND_RIGHT_FLAP = 11,  // Tepukkan tangan kanan
        ACTION_HAND_BOTH_FLAP = 12,   // Tepukkan kedua tangan

        // Aksi badan 13-15
        ACTION_BODY_TURN_LEFT = 13,    // Putar ke kiri
        ACTION_BODY_TURN_RIGHT = 14,   // Putar ke kanan
        ACTION_BODY_TURN_CENTER = 15,  // Kembali ke tengah

        // Aksi kepala 16-20
        ACTION_HEAD_UP = 16,          // Angkat kepala
        ACTION_HEAD_DOWN = 17,        // Tundukkan kepala
        ACTION_HEAD_NOD_ONCE = 18,    // Mengangguk sekali
        ACTION_HEAD_CENTER = 19,      // Kembali ke tengah
        ACTION_HEAD_NOD_REPEAT = 20,  // Mengangguk berulang

        // Aksi sistem 21
        ACTION_HOME = 21  // Kembali ke posisi awal
    };

    static void ActionTask(void* arg) {
        ElectronBotController* controller = static_cast<ElectronBotController*>(arg);
        ElectronBotActionParams params;
        controller->electron_bot_.AttachServos();

        while (true) {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "Menjalankan aksi: %d", params.action_type);
                controller->is_action_in_progress_ = true;  // Menandai awal eksekusi aksi

                // Jalankan aksi yang sesuai
                if (params.action_type >= ACTION_HAND_LEFT_UP &&
                    params.action_type <= ACTION_HAND_BOTH_FLAP) {
                    // Aksi tangan
                    controller->electron_bot_.HandAction(params.action_type, params.steps,
                                                         params.amount, params.speed);
                } else if (params.action_type >= ACTION_BODY_TURN_LEFT &&
                           params.action_type <= ACTION_BODY_TURN_CENTER) {
                    // Aksi badan
                    int body_direction = params.action_type - ACTION_BODY_TURN_LEFT + 1;
                    controller->electron_bot_.BodyAction(body_direction, params.steps,
                                                         params.amount, params.speed);
                } else if (params.action_type >= ACTION_HEAD_UP &&
                           params.action_type <= ACTION_HEAD_NOD_REPEAT) {
                    // Aksi kepala
                    int head_action = params.action_type - ACTION_HEAD_UP + 1;
                    controller->electron_bot_.HeadAction(head_action, params.steps, params.amount,
                                                         params.speed);
                } else if (params.action_type == ACTION_HOME) {
                    // Aksi kembali ke posisi awal
                    controller->electron_bot_.Home(true);
                }
                controller->is_action_in_progress_ = false;  // Menandai aksi selesai dijalankan
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    void QueueAction(int action_type, int steps, int speed, int direction, int amount) {
        ESP_LOGI(TAG, "Kontrol aksi: jenis=%d, langkah=%d, kecepatan=%d, arah=%d, amplitudo=%d", action_type, steps,
                 speed, direction, amount);

        ElectronBotActionParams params = {action_type, steps, speed, direction, amount};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void StartActionTaskIfNeeded() {
        if (action_task_handle_ == nullptr) {
            xTaskCreate(ActionTask, "electron_bot_action", 1024 * 4, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }

    void LoadTrimsFromNVS() {
        Settings settings("electron_trims", false);

        int right_pitch = settings.GetInt("right_pitch", 0);
        int right_roll = settings.GetInt("right_roll", 0);
        int left_pitch = settings.GetInt("left_pitch", 0);
        int left_roll = settings.GetInt("left_roll", 0);
        int body = settings.GetInt("body", 0);
        int head = settings.GetInt("head", 0);
        electron_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);
    }

public:
    ElectronBotController() {
        electron_bot_.Init(Right_Pitch_Pin, Right_Roll_Pin, Left_Pitch_Pin, Left_Roll_Pin, Body_Pin,
                           Head_Pin);

        LoadTrimsFromNVS();
        action_queue_ = xQueueCreate(10, sizeof(ElectronBotActionParams));

        QueueAction(ACTION_HOME, 1, 1000, 0, 0);

        RegisterMcpTools();
        ESP_LOGI(TAG, "Pengendali Electron Bot sudah diinisialisasi dan alat MCP telah didaftarkan");
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "Mulai mendaftarkan alat MCP Electron Bot...");

        // Alat terpadu untuk aksi tangan
        mcp_server.AddTool(
            "self.electron.hand_action",
            "Kontrol aksi tangan. action: 1=angkat tangan, 2=turunkan tangan, 3=lambaikan tangan, "
            "4=tepuk tangan; hand: 1=tangan kiri, 2=tangan kanan, 3=kedua tangan; "
            "steps: jumlah pengulangan aksi (1-10); speed: kecepatan aksi (500-1500, makin kecil "
            "makin cepat); amount: amplitudo gerakan (10-50, hanya dipakai untuk aksi angkat tangan)",
            PropertyList({Property("action", kPropertyTypeInteger, 1, 1, 4),
                          Property("hand", kPropertyTypeInteger, 3, 1, 3),
                          Property("steps", kPropertyTypeInteger, 1, 1, 10),
                          Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                          Property("amount", kPropertyTypeInteger, 30, 10, 50)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int action_type = properties["action"].value<int>();
                int hand_type = properties["hand"].value<int>();
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int amount = properties["amount"].value<int>();

                // Tentukan aksi spesifik dari jenis aksi dan pilihan tangan
                int base_action;
                switch (action_type) {
                    case 1:
                        base_action = ACTION_HAND_LEFT_UP;
                        break;  // Angkat tangan
                    case 2:
                        base_action = ACTION_HAND_LEFT_DOWN;
                        amount = 0;
                        break;  // Turunkan tangan
                    case 3:
                        base_action = ACTION_HAND_LEFT_WAVE;
                        amount = 0;
                        break;  // Melambaikan tangan
                    case 4:
                        base_action = ACTION_HAND_LEFT_FLAP;
                        amount = 0;
                        break;  // Menepuk
                    default:
                        base_action = ACTION_HAND_LEFT_UP;
                }
                int action_id = base_action + (hand_type - 1);

                QueueAction(action_id, steps, speed, 0, amount);
                return true;
            });

        // Aksi badan
        mcp_server.AddTool(
            "self.electron.body_turn",
            "Putaran badan. steps: jumlah langkah putar (1-10); speed: kecepatan putar "
            "(500-1500, makin kecil makin cepat); direction: arah putar (1=kiri, 2=kanan, "
            "3=kembali ke tengah); angle: sudut putar (0-90 derajat)",
            PropertyList({Property("steps", kPropertyTypeInteger, 1, 1, 10),
                          Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                          Property("direction", kPropertyTypeInteger, 1, 1, 3),
                          Property("angle", kPropertyTypeInteger, 45, 0, 90)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int direction = properties["direction"].value<int>();
                int amount = properties["angle"].value<int>();

                int action;
                switch (direction) {
                    case 1:
                        action = ACTION_BODY_TURN_LEFT;
                        break;
                    case 2:
                        action = ACTION_BODY_TURN_RIGHT;
                        break;
                    case 3:
                        action = ACTION_BODY_TURN_CENTER;
                        break;
                    default:
                        action = ACTION_BODY_TURN_LEFT;
                }

                QueueAction(action, steps, speed, 0, amount);
                return true;
            });

        // Aksi kepala
        mcp_server.AddTool("self.electron.head_move",
                           "Gerakan kepala. action: 1=angkat kepala, 2=tundukkan kepala, "
                           "3=angguk, 4=kembali ke tengah, 5=angguk berulang; steps: jumlah "
                           "pengulangan aksi (1-10); speed: kecepatan aksi (500-1500, makin kecil "
                           "makin cepat); angle: sudut gerak kepala (1-15 derajat)",
                           PropertyList({Property("action", kPropertyTypeInteger, 3, 1, 5),
                                         Property("steps", kPropertyTypeInteger, 1, 1, 10),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("angle", kPropertyTypeInteger, 5, 1, 15)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int action_num = properties["action"].value<int>();
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amount = properties["angle"].value<int>();
                               int action = ACTION_HEAD_UP + (action_num - 1);
                               QueueAction(action, steps, speed, 0, amount);
                               return true;
                           });

        // Alat sistem
        mcp_server.AddTool("self.electron.stop", "Segera hentikan", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               // Kosongkan antrean tanpa menghentikan tugas utama
                               xQueueReset(action_queue_);
                               is_action_in_progress_ = false;
                               QueueAction(ACTION_HOME, 1, 1000, 0, 0);
                               return true;
                           });

        mcp_server.AddTool("self.electron.get_status", "Mengambil status robot, mengembalikan moving atau idle",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        // Alat kalibrasi satu servo
        mcp_server.AddTool(
            "self.electron.set_trim",
            "Kalibrasi posisi satu servo. Atur nilai trim servo tertentu untuk menyesuaikan "
            "postur awal Electron Bot, dan hasilnya akan disimpan permanen. "
            "servo_type: jenis servo (right_pitch: putar lengan kanan, right_roll: dorong-tarik "
            "lengan kanan, left_pitch: putar lengan kiri, left_roll: dorong-tarik lengan kiri, "
            "body: badan, head: kepala); trim_value: nilai trim (-30 sampai 30 derajat)",
            PropertyList({Property("servo_type", kPropertyTypeString, "right_pitch"),
                          Property("trim_value", kPropertyTypeInteger, 0, -30, 30)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Atur trim servo: %s = %d derajat", servo_type.c_str(), trim_value);

                // Ambil semua nilai trim yang tersimpan saat ini
                Settings settings("electron_trims", true);
                int right_pitch = settings.GetInt("right_pitch", 0);
                int right_roll = settings.GetInt("right_roll", 0);
                int left_pitch = settings.GetInt("left_pitch", 0);
                int left_roll = settings.GetInt("left_roll", 0);
                int body = settings.GetInt("body", 0);
                int head = settings.GetInt("head", 0);

                // Perbarui nilai trim untuk servo yang dipilih
                if (servo_type == "right_pitch") {
                    right_pitch = trim_value;
                    settings.SetInt("right_pitch", right_pitch);
                } else if (servo_type == "right_roll") {
                    right_roll = trim_value;
                    settings.SetInt("right_roll", right_roll);
                } else if (servo_type == "left_pitch") {
                    left_pitch = trim_value;
                    settings.SetInt("left_pitch", left_pitch);
                } else if (servo_type == "left_roll") {
                    left_roll = trim_value;
                    settings.SetInt("left_roll", left_roll);
                } else if (servo_type == "body") {
                    body = trim_value;
                    settings.SetInt("body", body);
                } else if (servo_type == "head") {
                    head = trim_value;
                    settings.SetInt("head", head);
                } else {
                    return "Kesalahan: jenis servo tidak valid, gunakan: right_pitch, right_roll, left_pitch, "
                           "left_roll, body, head";
                }

                electron_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);

                QueueAction(ACTION_HOME, 1, 500, 0, 0);

                return "Trim servo " + servo_type + " diatur menjadi " + std::to_string(trim_value) +
                       " derajat dan sudah disimpan permanen";
            });

        mcp_server.AddTool("self.electron.get_trims", "Mengambil pengaturan trim servo saat ini", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               Settings settings("electron_trims", false);

                               int right_pitch = settings.GetInt("right_pitch", 0);
                               int right_roll = settings.GetInt("right_roll", 0);
                               int left_pitch = settings.GetInt("left_pitch", 0);
                               int left_roll = settings.GetInt("left_roll", 0);
                               int body = settings.GetInt("body", 0);
                               int head = settings.GetInt("head", 0);

                               std::string result =
                                   "{\"right_pitch\":" + std::to_string(right_pitch) +
                                   ",\"right_roll\":" + std::to_string(right_roll) +
                                   ",\"left_pitch\":" + std::to_string(left_pitch) +
                                   ",\"left_roll\":" + std::to_string(left_roll) +
                                   ",\"body\":" + std::to_string(body) +
                                   ",\"head\":" + std::to_string(head) + "}";

                               ESP_LOGI(TAG, "Mengambil pengaturan trim: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.battery.get_level", "Mengambil level baterai dan status pengisian robot", PropertyList(),
                           [](const PropertyList& properties) -> ReturnValue {
                               auto& board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status =
                                   "{\"level\":" + std::to_string(level) +
                                   ",\"charging\":" + (charging ? "true" : "false") + "}";
                               return status;
                           });

        ESP_LOGI(TAG, "Pendaftaran alat MCP Electron Bot selesai");
    }

    ~ElectronBotController() {
        if (action_task_handle_ != nullptr) {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static ElectronBotController* g_electron_controller = nullptr;

void InitializeElectronBotController() {
    if (g_electron_controller == nullptr) {
        g_electron_controller = new ElectronBotController();
        ESP_LOGI(TAG, "Pengendali Electron Bot sudah diinisialisasi dan alat MCP telah didaftarkan");
    }
}
