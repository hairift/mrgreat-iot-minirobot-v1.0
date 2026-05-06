#ifndef PRESS_TO_TALK_MCP_TOOL_H
#define PRESS_TO_TALK_MCP_TOOL_H

#include "mcp_server.h"
#include "settings.h"

// Kelas alat MCP yang dapat dipakai ulang untuk mode tekan-bicara.
class PressToTalkMcpTool {
private:
    bool press_to_talk_enabled_;

public:
    PressToTalkMcpTool();

    // Inisialisasi alat dan daftarkan ke server MCP.
    void Initialize();

    // Ambil status mode tekan-bicara saat ini.
    bool IsPressToTalkEnabled() const;

private:
    // Callback untuk alat MCP.
    ReturnValue HandleSetPressToTalk(const PropertyList& properties);

    // Metode internal untuk mengatur mode tekan-bicara dan menyimpannya.
    void SetPressToTalkEnabled(bool enabled);
};

#endif // PRESS_TO_TALK_MCP_TOOL_H
