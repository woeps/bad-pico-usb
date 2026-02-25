# bad-pico-usb

This project makes a Raspberry Pi **Pico W** act as a "bad" USB device with a wireless REPL interface.

## Current Behavior

- Enumerates as a USB HID keyboard when plugged into a USB host.
- Simultaneously starts a **Wi-Fi access point** (`BadPicoKB`, password: `badpico1`).
- Any device that connects to the AP and opens a TCP connection to `192.168.4.1:4242` gets a REPL prompt.
- Text entered at the REPL and confirmed with Enter is sent as USB HID keypresses to the USB host.

## Wi-Fi REPL Usage

1. Connect to Wi-Fi network `BadPicoKB` (password: `badpico1`).
2. Open a TCP connection to `192.168.4.1` port `4242`, e.g.:
   ```
   nc 192.168.4.1 4242
   ```
3. Type a line and press Enter — the Pico will type that text on the USB host. To send an Enter keypress, include `\n` at the end of the line.

### Supported Special Characters

Only the characters listed below are supported. All others are silently ignored.

| Character(s) | How to enter |
|--------------|--------------|
| `a`–`z` | Type directly |
| `A`–`Z` | Type directly (uppercase) |
| `0`–`9` | Type directly |
| `` . , - = / ; ' [ ] \ ` `` | Type directly |
| Space | Type directly |
| Tab | Type `\t` — interpreted as a Tab keypress |
| Enter | Type `\n` — interpreted as an Enter keypress |

## Configuration

WiFi credentials and REPL port are set as compile-time defines in `CMakeLists.txt`:

| Define          | Default       | Description              |
|-----------------|---------------|--------------------------|
| `WIFI_SSID`     | `BadPicoKB`   | Access point SSID        |
| `WIFI_PASSWORD` | `badpico1`    | Access point password    |
| `REPL_PORT`     | `4242`        | TCP port for REPL server |

## Hardware

Requires a **Raspberry Pi Pico W** (the original Pico has no Wi-Fi).

## Building

```sh
cmake -S . -B build -DPICO_BOARD=pico_w
cmake --build build --parallel
```

Flash `build/bad_pico_usb.uf2` onto the Pico W.

## Planned Features

- Trigger payload execution when a specific Bluetooth device becomes visible.
- Pre-prepared payload sequences stored in flash.

## Technical Details

- Pico SDK 2.1.1
- USB HID keyboard via TinyUSB on core 0
- Wi-Fi AP + lwIP TCP REPL server (`pico_cyw43_arch_lwip_threadsafe_background`) on core 1
- Inter-core communication via `pico_util/queue`