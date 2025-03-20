#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/allocator.h>
#include <bx/bx.h>
#include <bx/debug.h>
#include <bx/file.h>
#include <bx/math.h>
#include <bx/timer.h>

#include <iostream>

#include "bgfx/defines.h"
#include "components/physics.hpp"
#include "core/event_handler.hpp"
#include "core/game.hpp"

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#if defined(None)  // X11 defines this...
#undef None
#endif  // defined(None)

#include <imgui_helper.h>

#include <core/registry.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <input/input.hpp>
#include <renderer/bgfx_utils.hpp>
#include <renderer/physics.hpp>
#include <renderer/plane_primitive.hpp>
#include <systems/systems.hpp>

#include "entt/signal/dispatcher.hpp"

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
void signalHandler(int signum)
{
    void  *array[50];
    size_t size    = backtrace(array, 50);
    char **symbols = backtrace_symbols(array, size);

    fprintf(stderr, "Error: signal %d:\n", signum);

    for (size_t i = 0; i < size; ++i) {
        char *mangled   = symbols[i];
        char *demangled = nullptr;
        int   status    = 0;

        // Extract function name if possible
        char *begin = nullptr, *end = nullptr;
        for (char *p = mangled; *p; ++p) {
            if (*p == '(')
                begin = p;
            else if (*p == '+')
                end = p;
        }

        if (begin && end && begin < end) {
            *end      = '\0';  // Terminate at '+'
            demangled = abi::__cxa_demangle(begin + 1, nullptr, nullptr, &status);
            *end      = '+';  // Restore original symbol
        }

        // Print demangled or raw symbol
        if (status == 0 && demangled) {
            fprintf(stderr, "%s\n", demangled);
            free(demangled);
        } else {
            fprintf(stderr, "%s\n", symbols[i]);
        }
    }

    free(symbols);
    exit(1);
}
#else
void signalHandler(int signum) {}
#endif

int main()
{
    signal(SIGSEGV, signalHandler);

    Game game(1920, 1080);
    game.Init();

    return game.Run();
}
