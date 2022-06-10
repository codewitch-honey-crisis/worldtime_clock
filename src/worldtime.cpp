#include <Arduino.h>
#ifdef ESP32
#include <pgmspace.h>
#include <HTTPClient.h>
#else
#include <avr/pgmspace.h>
#endif
#include <worldtime.hpp>
#ifdef ESP32
time_t worldtime::now(int8_t utc_offset) {
    constexpr static const char* url = "http://worldtimeapi.org/api/timezone/Etc/UTC";
    HTTPClient client;
    client.begin(url);
    if(0>=client.GET()) {
        return time_t(0);
    }
    time_t result = parse(client.getStream());
    client.end();
    return (time_t)result+(utc_offset*3600);
}

#endif
time_t worldtime::parse(Stream& stm) {
    if(!stm.available()) {
        return (time_t)0;
    }
    if(!stm.find("unixtime\":")) {
        return (time_t)0;
    }
    int ch = stm.read();
    long long lt = 0;
    while(ch>='0' && ch<='9') {
        lt*=10;
        lt+=(ch-'0');
        ch=stm.read();
    }
    return (time_t)lt;
}