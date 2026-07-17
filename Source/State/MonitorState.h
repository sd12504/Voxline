#pragma once

#include <atomic>

namespace VoxlineState
{
enum class MonitorMode
{
    none,
    eqBandSolo,
    deEssListenS
};

class MonitorState
{
public:
    void setEqBandSolo(int band) noexcept;
    void setDeEssListen(bool enabled) noexcept;
    void clear() noexcept;
    MonitorMode mode() const noexcept;
    int eqBand() const noexcept;

private:
    std::atomic<MonitorMode> currentMode {MonitorMode::none};
    std::atomic<int> currentEqBand {-1};
};
} // namespace VoxlineState
