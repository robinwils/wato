#include <bx/bx.h>

#include "core/game.hpp"

#if defined(None)  // X11 defines this...
#undef None
#endif  // defined(None)

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
