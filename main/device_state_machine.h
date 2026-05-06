#ifndef DEVICE_STATE_MACHINE_H
#define DEVICE_STATE_MACHINE_H

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>

#include "device_state.h"

/**
 * DeviceStateMachine - Mengelola perpindahan status perangkat dengan validasi.
 *
 * Kelas ini memastikan aturan perpindahan status tetap ketat dan menyediakan
 * mekanisme callback agar komponen lain bisa merespons perubahan status.
 */
class DeviceStateMachine {
public:
    DeviceStateMachine();
    ~DeviceStateMachine() = default;

    // Nonaktifkan copy constructor dan operator assignment
    DeviceStateMachine(const DeviceStateMachine&) = delete;
    DeviceStateMachine& operator=(const DeviceStateMachine&) = delete;

    /**
     * Ambil status perangkat saat ini.
     */
    DeviceState GetState() const { return current_state_.load(); }

    /**
     * Coba pindah ke status baru.
     * @param new_state Status tujuan.
     * @return true jika perpindahan berhasil, false jika transisinya tidak valid.
     */
    bool TransitionTo(DeviceState new_state);

    /**
     * Periksa apakah perpindahan ke status tujuan valid dari status saat ini.
     */
    bool CanTransitionTo(DeviceState target) const;

    /**
     * Tipe callback perubahan status.
     * Parameter: status_lama, status_baru
     */
    using StateCallback = std::function<void(DeviceState, DeviceState)>;

    /**
     * Tambahkan listener perubahan status.
     * Callback dipanggil dalam konteks pemanggil TransitionTo().
     * @return id listener yang dapat dipakai untuk penghapusan.
     */
    int AddStateChangeListener(StateCallback callback);

    /**
     * Hapus listener perubahan status berdasarkan id.
     */
    void RemoveStateChangeListener(int listener_id);

    /**
     * Ambil nama status dalam bentuk string untuk keperluan log.
     */
    static const char* GetStateName(DeviceState state);

private:
    std::atomic<DeviceState> current_state_{kDeviceStateUnknown};
    std::vector<std::pair<int, StateCallback>> listeners_;
    int next_listener_id_{0};
    std::mutex mutex_;

    /**
     * Periksa apakah perpindahan dari status asal ke status tujuan valid.
     */
    bool IsValidTransition(DeviceState from, DeviceState to) const;

    /**
     * Beri tahu callback bahwa status telah berubah.
     */
    void NotifyStateChange(DeviceState old_state, DeviceState new_state);
};

#endif // DEVICE_STATE_MACHINE_H
