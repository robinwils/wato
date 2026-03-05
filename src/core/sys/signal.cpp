#include "core/sys/signal.hpp"

#include <bx/bx.h>
#include <signal.h>

std::atomic_bool gShutdownRequested{false};

static void shutdownHandler(int) { gShutdownRequested.store(true); }

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#include <execinfo.h>
#include <unistd.h>

static void crashHandler(int)
{
    void* array[50];
    int   size = backtrace(array, 50);
    // Writes directly to stderr, no malloc. Use addr2line to demangle.
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    _exit(1);
}

void installSignalHandlers()
{
    signal(SIGSEGV, crashHandler);
    signal(SIGTERM, shutdownHandler);
    signal(SIGINT, shutdownHandler);
}
#else
void installSignalHandlers()
{
    signal(SIGTERM, shutdownHandler);
    signal(SIGINT, shutdownHandler);
}
#endif
