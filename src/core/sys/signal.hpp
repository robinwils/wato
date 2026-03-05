#pragma once

#include <atomic>

extern std::atomic_bool gShutdownRequested;

void installSignalHandlers();
