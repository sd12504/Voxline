#include "MonitorState.h"

void VoxlineState::MonitorState::setEqBandSolo(int band) noexcept
{
    if (band < 0 || band > 5)
    {
        clear();
        return;
    }

    currentEqBand.store(band, std::memory_order_relaxed);
    currentMode.store(MonitorMode::eqBandSolo, std::memory_order_release);
}

void VoxlineState::MonitorState::setDeEssListen(bool enabled) noexcept
{
    if (enabled)
    {
        currentEqBand.store(-1, std::memory_order_relaxed);
        currentMode.store(MonitorMode::deEssListenS,
                          std::memory_order_release);
    }
    else if (mode() == MonitorMode::deEssListenS)
    {
        clear();
    }
}

void VoxlineState::MonitorState::clear() noexcept
{
    currentEqBand.store(-1, std::memory_order_relaxed);
    currentMode.store(MonitorMode::none, std::memory_order_release);
}

VoxlineState::MonitorMode
VoxlineState::MonitorState::mode() const noexcept
{
    return currentMode.load(std::memory_order_acquire);
}

int VoxlineState::MonitorState::eqBand() const noexcept
{
    return mode() == MonitorMode::eqBandSolo
               ? currentEqBand.load(std::memory_order_relaxed)
               : -1;
}
