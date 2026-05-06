#include "press_to_talk_mcp_tool.h"
#include <esp_log.h>

static const char* TAG = "PressToTalkMcpTool";

PressToTalkMcpTool::PressToTalkMcpTool()
    : press_to_talk_enabled_(false) {
}

void PressToTalkMcpTool::Initialize() {
    // Baca status saat ini dari pengaturan.
    Settings settings("vendor");
    press_to_talk_enabled_ = settings.GetInt("press_to_talk", 0) != 0;

    // Daftarkan alat MCP.
    auto& mcp_server = McpServer::GetInstance();
    mcp_server.AddTool("self.set_press_to_talk",
        "Beralih antara mode tekan untuk bicara dan mode klik untuk bicara.\n"
        "Nilai mode dapat berupa `press_to_talk` atau `click_to_talk`.",
        PropertyList({
            Property("mode", kPropertyTypeString)
        }),
        [this](const PropertyList& properties) -> ReturnValue {
            return HandleSetPressToTalk(properties);
        });

    ESP_LOGI(TAG, "PressToTalkMcpTool diinisialisasi, mode saat ini: %s",
        press_to_talk_enabled_ ? "press_to_talk" : "click_to_talk");
}

bool PressToTalkMcpTool::IsPressToTalkEnabled() const {
    return press_to_talk_enabled_;
}

ReturnValue PressToTalkMcpTool::HandleSetPressToTalk(const PropertyList& properties) {
    auto mode = properties["mode"].value<std::string>();

    if (mode == "press_to_talk") {
        SetPressToTalkEnabled(true);
        ESP_LOGI(TAG, "Beralih ke mode tekan untuk bicara");
        return true;
    } else if (mode == "click_to_talk") {
        SetPressToTalkEnabled(false);
        ESP_LOGI(TAG, "Beralih ke mode klik untuk bicara");
        return true;
    }

    throw std::runtime_error("Mode tidak valid: " + mode);
}

void PressToTalkMcpTool::SetPressToTalkEnabled(bool enabled) {
    press_to_talk_enabled_ = enabled;

    Settings settings("vendor", true);
    settings.SetInt("press_to_talk", enabled ? 1 : 0);
    ESP_LOGI(TAG, "Mode tekan untuk bicara aktif: %d", enabled);
}
