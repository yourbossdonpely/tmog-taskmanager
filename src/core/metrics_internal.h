#pragma once

#include "core/metrics.h"
#include "core/metrics_parse.h"

namespace tmog::detail {

struct PlatformState
{
    bool hasCpu = false;
    CpuTimes cpu;

    bool hasNetwork = false;
    quint64 networkReceive = 0;
    quint64 networkTransmit = 0;

#if defined(Q_OS_WIN)
    quint64 winIdle = 0;
    quint64 winKernel = 0;
    quint64 winUser = 0;
#endif
};

MetricsSnapshot samplePlatform(PlatformState &state, double elapsedSeconds);

} // namespace tmog::detail
