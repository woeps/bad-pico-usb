#include <array>
#include <cctype>
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
constexpr size_t kTagMaxLen = 64;

struct TextMessage {
    char text[WIFI_REPL_LINE_MAX];
};

struct ErrorMessage {
    char text[WIFI_REPL_LINE_MAX];
};

static queue_t s_text_queue;
static queue_t s_error_queue;

struct KeyName {
    const char *name;
    uint8_t keycode;
    uint8_t modifier;
};

static constexpr KeyName key_names[] = {
    // Modifiers (keycode 0 = modifier-only)
    {"ctrl",        0, KEYBOARD_MODIFIER_LEFTCTRL},
    {"control",     0, KEYBOARD_MODIFIER_LEFTCTRL},
    {"alt",         0, KEYBOARD_MODIFIER_LEFTALT},
    {"shift",       0, KEYBOARD_MODIFIER_LEFTSHIFT},
    {"super",       0, KEYBOARD_MODIFIER_LEFTGUI},
    {"win",         0, KEYBOARD_MODIFIER_LEFTGUI},
    {"gui",         0, KEYBOARD_MODIFIER_LEFTGUI},
    {"cmd",         0, KEYBOARD_MODIFIER_LEFTGUI},

    // Special keys
    {"enter",       HID_KEY_ENTER,        0},
    {"return",      HID_KEY_ENTER,        0},
    {"tab",         HID_KEY_TAB,          0},
    {"esc",         HID_KEY_ESCAPE,       0},
    {"escape",      HID_KEY_ESCAPE,       0},
    {"backspace",   HID_KEY_BACKSPACE,    0},
    {"delete",      HID_KEY_DELETE,       0},
    {"del",         HID_KEY_DELETE,       0},
    {"space",       HID_KEY_SPACE,        0},

    // Arrow keys
    {"up",          HID_KEY_ARROW_UP,     0},
    {"down",        HID_KEY_ARROW_DOWN,   0},
    {"left",        HID_KEY_ARROW_LEFT,   0},
    {"right",       HID_KEY_ARROW_RIGHT,  0},

    // Navigation
    {"home",        HID_KEY_HOME,         0},
    {"end",         HID_KEY_END,          0},
    {"pageup",      HID_KEY_PAGE_UP,      0},
    {"pagedown",    HID_KEY_PAGE_DOWN,    0},
    {"insert",      HID_KEY_INSERT,       0},
    {"capslock",    HID_KEY_CAPS_LOCK,    0},
    {"printscreen", HID_KEY_PRINT_SCREEN, 0},

    // Function keys
    {"f1",  HID_KEY_F1,  0},
    {"f2",  HID_KEY_F2,  0},
    {"f3",  HID_KEY_F3,  0},
    {"f4",  HID_KEY_F4,  0},
    {"f5",  HID_KEY_F5,  0},
    {"f6",  HID_KEY_F6,  0},
    {"f7",  HID_KEY_F7,  0},
    {"f8",  HID_KEY_F8,  0},
    {"f9",  HID_KEY_F9,  0},
    {"f10", HID_KEY_F10, 0},
    {"f11", HID_KEY_F11, 0},
    {"f12", HID_KEY_F12, 0},
};

static constexpr size_t kKeyNameCount = sizeof(key_names) / sizeof(key_names[0]);

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
        case '\n': keycode = HID_KEY_ENTER;    return true;
        default: break;
    }

    return false;
}

void send_combo(uint8_t modifier, uint8_t keycode) {
    while (!tud_hid_ready()) {
        tud_task();
        sleep_ms(2);
    }

    std::array<uint8_t, 6> keys{};
    if (keycode) {
        keys[0] = keycode;
    }

    tud_hid_keyboard_report(kReportId, modifier, keys.data());

    while (!tud_hid_ready()) {
        tud_task();
        sleep_ms(2);
    }

    tud_hid_keyboard_report(kReportId, 0, nullptr);
    sleep_ms(5);
}

void send_key(char c) {
    uint8_t keycode;
    uint8_t modifier;
    if (!char_to_key(c, keycode, modifier)) {
        return;
    }
    send_combo(modifier, keycode);
}

void report_error(const char *fmt, const char *arg) {
    ErrorMessage err{};
    snprintf(err.text, sizeof(err.text), fmt, arg);
    queue_try_add(&s_error_queue, &err);
}

bool strcasecmp_const(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

bool lookup_key_name(const char *name, uint8_t &keycode, uint8_t &modifier) {
    for (size_t i = 0; i < kKeyNameCount; ++i) {
        if (strcasecmp_const(name, key_names[i].name)) {
            keycode = key_names[i].keycode;
            modifier = key_names[i].modifier;
            return true;
        }
    }

    // Single character key name (e.g. "a", "z", "5")
    if (name[0] != '\0' && name[1] == '\0') {
        return char_to_key(name[0], keycode, modifier);
    }

    return false;
}

void send_tag(const char *tag) {
    // Parse tag content: split on '+', accumulate modifiers, last non-modifier is the key
    char buf[kTagMaxLen];
    strncpy(buf, tag, kTagMaxLen - 1);
    buf[kTagMaxLen - 1] = '\0';

    uint8_t combined_modifier = 0;
    uint8_t final_keycode = 0;
    bool has_keycode = false;

    char *saveptr = nullptr;
    char *token = strtok_r(buf, "+", &saveptr);
    while (token) {
        // Trim leading/trailing whitespace
        while (*token == ' ') ++token;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') { *end = '\0'; --end; }

        uint8_t kc, mod;
        if (!lookup_key_name(token, kc, mod)) {
            report_error("unknown key: %s\r\n", token);
            return;
        }

        if (kc == 0 && mod != 0) {
            // Pure modifier
            combined_modifier |= mod;
        } else {
            if (has_keycode) {
                report_error("multiple non-modifier keys in combo: %s\r\n", tag);
                return;
            }
            combined_modifier |= mod;
            final_keycode = kc;
            has_keycode = true;
        }

        token = strtok_r(nullptr, "+", &saveptr);
    }

    send_combo(combined_modifier, final_keycode);
}

void send_text(const char *text) {
    const char *p = text;
    while (*p != '\0') {
        if (*p == '\\' && *(p + 1) == '<') {
            // Escaped '<' — send literal '<'
            send_key('<');
            p += 2;
        } else if (*p == '<') {
            // Tag start — find closing '>'
            const char *start = p + 1;
            const char *end = strchr(start, '>');
            if (!end) {
                // No closing '>' — send '<' literally
                send_key('<');
                ++p;
                continue;
            }
            size_t len = static_cast<size_t>(end - start);
            if (len == 0 || len >= kTagMaxLen) {
                report_error("invalid tag: <%.*s>\r\n", text);
                p = end + 1;
                continue;
            }
            char tag[kTagMaxLen];
            memcpy(tag, start, len);
            tag[len] = '\0';
            send_tag(tag);
            p = end + 1;
        } else {
            send_key(*p);
            ++p;
        }
    }
}

void on_repl_line(const char *line, size_t /*len*/) {
    TextMessage msg{};
    strncpy(msg.text, line, WIFI_REPL_LINE_MAX - 1);
    queue_try_add(&s_text_queue, &msg);
}

void core1_entry() {
    wifi_repl_init(on_repl_line, &s_error_queue);
    while (true) {
        wifi_repl_poll_errors();
        sleep_ms(10);
    }
}

}  // namespace

int main() {
    stdio_init_all();
    board_init();
    tusb_init();

    queue_init(&s_text_queue, sizeof(TextMessage), 8);
    queue_init(&s_error_queue, sizeof(ErrorMessage), 8);

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
