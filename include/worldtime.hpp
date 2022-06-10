#pragma once
#include <Arduino.h>
struct worldtime final {
#ifdef ESP32
    static time_t now(int8_t utc_offset);
#endif
    static time_t parse(Stream& stm);
};