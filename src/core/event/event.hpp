#pragma once

template <typename EventType>
struct Event {
    Event()                         = default;
    Event(const Event &)            = default;
    Event(Event &&)                 = default;
    Event &operator=(const Event &) = default;
    Event &operator=(Event &&)      = default;
};
