#include <array>
#include <cstdint>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

namespace {

constexpr uint8_t kReportId = 0;
constexpr uint32_t kDelayMs = 30'000;

bool char_to_key(char c, uint8_t &keycode, uint8_t &modifier) {
    modifier = 0;

    if (c >= 'a' && c <= 'z') {
        keycode = HID_KEY_A + (c - 'a');
        return true;
    }

    if (c >= 'A' && c <= 'Z') {
        keycode = HID_KEY_A + (c - 'A');
        modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        return true;
    }

    if (c == ' ') {
        keycode = HID_KEY_SPACE;
        return true;
    }

    if (c == '\n') {
        keycode = HID_KEY_ENTER;
        return true;
    }

    return false;
}

void send_key(char c) {
    uint8_t keycode;
    uint8_t modifier;
    if (!char_to_key(c, keycode, modifier)) {
        return;
    }

    while (!tud_hid_ready()) {
        tud_task();
        sleep_ms(2);
    }

    std::array<uint8_t, 6> keys{};
    keys[0] = keycode;

    tud_hid_keyboard_report(kReportId, modifier, keys.data());
    sleep_ms(5);

    tud_hid_keyboard_report(kReportId, 0, nullptr);
    sleep_ms(5);
}

void send_text(const char *text) {
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        send_key(*cursor);
    }
}

}  // namespace

int main() {
    stdio_init_all();
    board_init();
    tusb_init();

    while (!tud_mounted()) {
        tud_task();
        sleep_ms(10);
    }

    absolute_time_t start = get_absolute_time();
    while (absolute_time_diff_us(start, get_absolute_time()) < (kDelayMs * 1000)) {
        tud_task();
        sleep_ms(10);
    }

    send_text("hello world");

    while (true) {
        tud_task();
        sleep_ms(10);
    }
}

extern "C" {

uint16_t tud_hid_get_report_cb(uint8_t /*instance*/, uint8_t /*report_id*/, hid_report_type_t /*report_type*/, uint8_t* /*buffer*/, uint16_t /*reqlen*/) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t /*instance*/, uint8_t /*report_id*/, hid_report_type_t /*report_type*/, uint8_t const* /*buffer*/, uint16_t /*bufsize*/) {}

void tud_hid_report_complete_cb(uint8_t /*instance*/, uint8_t const* /*report*/, uint16_t /*len*/) {}

}
