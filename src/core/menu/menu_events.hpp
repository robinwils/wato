#pragma once

#include <string>

#include "registry/registry.hpp"

struct LoginEvent {
    Registry*   Reg;
    std::string Account;
    std::string Password;
};
