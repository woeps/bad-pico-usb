#include <array>
#include <cstdint>
#include <cstring>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "tusb.h"

#include "wifi_repl.h"

namespace {

constexpr uint8_t kReportId = 0;

struct TextMessage {
    char text[WIFI_REPL_LINE_MAX];
};

static queue_t s_text_queue;

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

    if (c >= '0' && c <= '9') {
        keycode = HID_KEY_0 + (c - '0');
        return true;
    }

    switch (c) {
        case '.': keycode = HID_KEY_PERIOD;    return true;
        case ',': keycode = HID_KEY_COMMA;     return true;
        case '-': keycode = HID_KEY_MINUS;     return true;
        case '=': keycode = HID_KEY_EQUAL;     return true;
        case '/': keycode = HID_KEY_SLASH;     return true;
        case ';': keycode = HID_KEY_SEMICOLON; return true;
        case '\'': keycode = HID_KEY_APOSTROPHE; return true;
        case '[': keycode = HID_KEY_BRACKET_LEFT;  return true;
        case ']': keycode = HID_KEY_BRACKET_RIGHT; return true;
        case '\\': keycode = HID_KEY_BACKSLASH; return true;
        case '`': keycode = HID_KEY_GRAVE;     return true;
        case '\t': keycode = HID_KEY_TAB;      return true;
        default: break;
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

void on_repl_line(const char *line, size_t /*len*/) {
    TextMessage msg{};
    strncpy(msg.text, line, WIFI_REPL_LINE_MAX - 1);
    queue_try_add(&s_text_queue, &msg);
}

void core1_entry() {
    wifi_repl_init(on_repl_line);
    while (true) {
        tight_loop_contents();
    }
}

}  // namespace

int main() {
    stdio_init_all();
    board_init();
    tusb_init();

    queue_init(&s_text_queue, sizeof(TextMessage), 8);

    multicore_launch_core1(core1_entry);

    while (!tud_mounted()) {
        tud_task();
        sleep_ms(10);
    }

    while (true) {
        tud_task();

        TextMessage msg;
        if (queue_try_remove(&s_text_queue, &msg)) {
            send_text(msg.text);
            send_key('\n');
        }

        sleep_ms(1);
    }
}

extern "C" {

uint16_t tud_hid_get_report_cb(uint8_t /*instance*/, uint8_t /*report_id*/, hid_report_type_t /*report_type*/, uint8_t* /*buffer*/, uint16_t /*reqlen*/) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t /*instance*/, uint8_t /*report_id*/, hid_report_type_t /*report_type*/, uint8_t const* /*buffer*/, uint16_t /*bufsize*/) {}

void tud_hid_report_complete_cb(uint8_t /*instance*/, uint8_t const* /*report*/, uint16_t /*len*/) {}

}
