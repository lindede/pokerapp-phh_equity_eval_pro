#pragma once

#include <chrono>

namespace pokerstove
{

using SteadyClock = std::chrono::steady_clock;

inline double elapsed_seconds(SteadyClock::time_point start)
{
    return std::chrono::duration<double>(SteadyClock::now() - start).count();
}

}  // namespace pokerstove
