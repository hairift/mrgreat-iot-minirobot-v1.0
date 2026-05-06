#include <cJSON.h>
#include <esp_log.h>

#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "display.h"
#include "mcp_server.h"
#include "sdkconfig.h"
#include "settings.h"

#define TAG "MCPController"

class MCPController {
public:
    MCPController() {
        RegisterMcpTools();
        ESP_LOGI(TAG, "Alat MCP berhasil didaftarkan");
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();
        ESP_LOGI(TAG, "Mulai mendaftarkan alat MCP...");

        mcp_server.AddTool(
            "self.AEC.set_mode",
            "Mengatur mode pemutusan percakapan AEC. Gunakan alat ini ketika pengguna ingin "
            "mengganti mode pemutusan percakapan, merasa percakapan terlalu mudah terputus, "
            "atau merasa pemutusan percakapan belum berjalan sebagaimana mestinya.\n"
            "Parameter:\n"
            "   `mode`: mode pemutusan percakapan. Nilai yang didukung hanya `kAecOff` "
            "(nonaktif) dan `kAecOnDeviceSide` (aktif).\n"
            "Nilai balik:\n"
            "   Informasi status untuk langsung dibacakan tanpa perlu konfirmasi tambahan.\n",
            PropertyList({Property("mode", kPropertyTypeString)}),
            [](const PropertyList& properties) -> ReturnValue {
                auto mode = properties["mode"].value<std::string>();
                auto& app = Application::GetInstance();
                vTaskDelay(pdMS_TO_TICKS(2000));
                if (mode == "kAecOff") {
                    app.SetAecMode(kAecOff);
                    return "{\"success\": true, \"message\": \"Mode pemutusan percakapan AEC "
                           "sudah dinonaktifkan\"}";
                }

                app.SetAecMode(kAecOnDeviceSide);
                return "{\"success\": true, \"message\": \"Mode pemutusan percakapan AEC sudah "
                       "diaktifkan\"}";
            });

        mcp_server.AddTool(
            "self.AEC.get_mode",
            "Mengambil status mode pemutusan percakapan AEC. Gunakan alat ini ketika pengguna "
            "ingin mengetahui mode AEC yang sedang aktif.\n"
            "Nilai balik:\n"
            "   Informasi status untuk langsung dibacakan tanpa perlu konfirmasi tambahan.\n",
            PropertyList(),
            [](const PropertyList&) -> ReturnValue {
                auto& app = Application::GetInstance();
                const bool is_currently_off = (app.GetAecMode() == kAecOff);
                if (is_currently_off) {
                    return "{\"success\": true, \"message\": \"Mode pemutusan percakapan AEC "
                           "sedang nonaktif\"}";
                }
                return "{\"success\": true, \"message\": \"Mode pemutusan percakapan AEC sedang "
                       "aktif\"}";
            });

        mcp_server.AddTool(
            "self.res.esp_restart",
            "Memulai ulang perangkat. Gunakan alat ini ketika pengguna meminta perangkat "
            "dinyalakan ulang.\n",
            PropertyList(),
            [](const PropertyList&) -> ReturnValue {
                vTaskDelay(pdMS_TO_TICKS(1000));
                // Nyalakan ulang perangkat
                esp_restart();
                return true;
            });

        ESP_LOGI(TAG, "Pendaftaran alat MCP selesai");
    }
};

static MCPController* g_mcp_controller = nullptr;

void InitializeMCPController() {
    if (g_mcp_controller == nullptr) {
        g_mcp_controller = new MCPController();
        ESP_LOGI(TAG, "Pengendali MCP berhasil diinisialisasi");
    }
}
