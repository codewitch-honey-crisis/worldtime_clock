#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include <gfx_cpp14.hpp>
#include <ili9341.hpp>
#include <tft_io.hpp>
#include <worldtime.hpp>
using namespace arduino;
using namespace gfx;

// wifi
constexpr static const char* ssid = "Communism_Will_Win";
constexpr static const char* password = "mypalkarl";

// timezone
constexpr static const int8_t utc_offset = -8; // UTC

// synchronize with worldtime every 60 seconds
constexpr static const int sync_seconds = 60;

constexpr static const size16 clock_size = {120, 120};

constexpr static const uint8_t spi_host = VSPI;
constexpr static const int8_t lcd_pin_bl = 32;
constexpr static const int8_t lcd_pin_dc = 27;
constexpr static const int8_t lcd_pin_cs = 14;
constexpr static const int8_t spi_pin_mosi = 23;
constexpr static const int8_t spi_pin_clk = 18;
constexpr static const int8_t lcd_pin_rst = 33;
constexpr static const int8_t spi_pin_miso = 19;

using bus_t = tft_spi_ex<spi_host, lcd_pin_cs, spi_pin_mosi, spi_pin_miso, spi_pin_clk, SPI_MODE0, false, 320 * 240 * 2 + 8, 2>;
using lcd_t = ili9342c<lcd_pin_dc, lcd_pin_rst, lcd_pin_bl, bus_t, 1, true, 400, 200>;
using color_t = color<typename lcd_t::pixel_type>;

lcd_t lcd;

template <typename Destination>
void draw_clock(Destination& dst, tm& time, const ssize16& size) {
    using view_t = viewport<Destination>;
    srect16 b = size.bounds().normalize();
    uint16_t w = min(b.width(), b.height());
    srect16 sr(0, 0, w / 16, w / 5);
    sr.center_horizontal_inplace(b);
    view_t view(dst);
    view.center(spoint16(w / 2, w / 2));
    for (float rot = 0; rot < 360; rot += 45) {
        view.rotation(rot);
        spoint16 marker_points[] = {
            view.translate(spoint16(sr.x1, sr.y1)),
            view.translate(spoint16(sr.x2, sr.y1)),
            view.translate(spoint16(sr.x2, sr.y2)),
            view.translate(spoint16(sr.x1, sr.y2))};
        spath16 marker_path(4, marker_points);
        draw::filled_polygon(dst, marker_path, color<typename Destination::pixel_type>::gray);
    }
    sr = srect16(0, 0, w / 16, w / 2);
    sr.center_horizontal_inplace(b);
    view.center(spoint16(w / 2, w / 2));
    view.rotation((time.tm_sec / 60.0) * 360.0);
    spoint16 second_points[] = {
        view.translate(spoint16(sr.x1, sr.y1)),
        view.translate(spoint16(sr.x2, sr.y1)),
        view.translate(spoint16(sr.x2, sr.y2)),
        view.translate(spoint16(sr.x1, sr.y2))};
    spath16 second_path(4, second_points);

    view.rotation((time.tm_min / 60.0) * 360.0);
    spoint16 minute_points[] = {
        view.translate(spoint16(sr.x1, sr.y1)),
        view.translate(spoint16(sr.x2, sr.y1)),
        view.translate(spoint16(sr.x2, sr.y2)),
        view.translate(spoint16(sr.x1, sr.y2))};
    spath16 minute_path(4, minute_points);

    sr.y1 += w / 8;
    view.rotation(((time.tm_hour%12) / 12.0) * 360.0);
    spoint16 hour_points[] = {
        view.translate(spoint16(sr.x1, sr.y1)),
        view.translate(spoint16(sr.x2, sr.y1)),
        view.translate(spoint16(sr.x2, sr.y2)),
        view.translate(spoint16(sr.x1, sr.y2))};
    spath16 hour_path(4, hour_points);

    draw::filled_polygon(dst, minute_path, color<typename Destination::pixel_type>::black);

    draw::filled_polygon(dst, hour_path, color<typename Destination::pixel_type>::black);

    draw::filled_polygon(dst, second_path, color<typename Destination::pixel_type>::red);
}

uint32_t update_ts;
uint32_t sync_count;
time_t current_time;
srect16 clock_rect;
using clock_bmp_t = bitmap<typename lcd_t::pixel_type>;
uint8_t clock_bmp_buf[clock_bmp_t::sizeof_buffer(clock_size)];
clock_bmp_t clock_bmp(clock_size, clock_bmp_buf);
void setup() {
    Serial.begin(115200);
    lcd.fill(lcd.bounds(),color_t::white);
    Serial.print("Connecting");
    WiFi.begin(ssid, password);
    while (!WiFi.isConnected()) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println();
    Serial.println("Connected");
    clock_rect = srect16(spoint16::zero(), (ssize16)clock_size);
    clock_rect.center_inplace((srect16)lcd.bounds());
    sync_count = sync_seconds;
    current_time = worldtime::now(utc_offset);
    update_ts = millis();
}

void loop() {
    uint32_t ms = millis();
    if (ms - update_ts >= 1000) {
        update_ts = ms;
        ++current_time;
        tm* t = localtime(&current_time);
        Serial.println(asctime(t));
        draw::wait_all_async(lcd);
        draw::filled_rectangle(clock_bmp, clock_size.bounds(), color_t::white);
        draw_clock(clock_bmp, *t, (ssize16)clock_size);
        draw::bitmap_async(lcd, clock_rect, clock_bmp, clock_bmp.bounds());
        if (0 == --sync_count) {
            sync_count = sync_seconds;
            current_time = worldtime::now(utc_offset);
        }
    }
}