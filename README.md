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
3. Type a line and press Enter — the Pico will type that text as USB HID keypresses on the host.

### Supported Characters

Plain characters are typed directly. All unsupported characters are silently ignored.

| Character(s) | How to enter |
|--------------|--------------|
| `a`–`z` | Type directly |
| `A`–`Z` | Type directly (sends Shift + key) |
| `0`–`9` | Type directly |
| `` . , - = / ; ' [ ] \ ` `` | Type directly |
| Space | Type directly |
| `<` | Type `\<` (escaped, since `<` starts a tag) |

### Special Keys & Key Combos

Use `<tag>` notation to send special keys and key combinations. Tags are **case-insensitive**.

#### Modifier + key combos

Keys joined with `+` are pressed simultaneously:

```
<ctrl+c>        — Ctrl-C
<ctrl+alt+del>  — Ctrl-Alt-Delete
<super+l>       — GUI/Super + L (lock screen)
<alt+tab>       — Alt-Tab
<ctrl+shift+t>  — Ctrl-Shift-T
```

#### Standalone special keys

```
<enter>   <tab>     <esc>        <backspace>   <delete>
<space>   <up>      <down>       <left>        <right>
<home>    <end>     <pageup>     <pagedown>
<insert>  <capslock> <printscreen>
<f1> .. <f12>
<sleep:N>                        — wait N seconds (0-3600) before continuing
```

##### Sleep command

The `<sleep:N>` command pauses execution for N seconds before sending any further keystrokes. This is useful for timing-dependent operations or waiting for applications to respond.

- N must be an integer between 0 and 3600 (1 hour)
- Decimal values are not supported
- The USB connection remains active during sleep

Examples:
```
<sleep:3>                        — wait 3 seconds
<sleep:60>                       — wait 1 minute
open<sleep:2>notepad<enter>     — type "open", wait 2s, then open notepad
```

#### Standalone modifiers (press + release)

```
<ctrl>  <alt>  <shift>  <super>
```

#### Modifier aliases

| Name | Aliases |
|------|---------|
| Left Ctrl | `ctrl`, `control` |
| Left Alt | `alt` |
| Left Shift | `shift` |
| Left GUI / Super | `super`, `win`, `gui`, `cmd` |

#### Examples

```
hello world<enter>              — types "hello world" then presses Enter
<ctrl+a><ctrl+c>                — select all, then copy
<super+l>                       — lock screen (Windows/Linux)
<alt+f4>                        — close window
cd /tmp<enter>                  — types a shell command and presses Enter
```

#### Error handling

Unknown tag names produce an error message in the REPL session and are **not** forwarded as keypresses.

### Macros

Use `<<macro_name>>` to expand a predefined macro into a sequence of text and key combos. Macro names are **case-insensitive**.

#### Built-in macros

| Macro | Expands to | Effect |
|-------|-----------|--------|
| `<<openterminal>>` | `<ctrl+alt+t>` | Open terminal (Linux) |
| `<<selectall>>` | `<ctrl+a>` | Select all |
| `<<copyall>>` | `<ctrl+a><ctrl+c>` | Select all and copy |
| `<<hello>>` | `Hello, World!<enter>` | Type greeting and press Enter |

#### Adding macros

Macros are defined as a compile-time table in `src/bad_pico_usb.cpp`:

```cpp
static constexpr Macro macros[] = {
    {"openterminal", "<ctrl+alt+t>"},
    {"selectall",    "<ctrl+a>"},
    {"copyall",      "<ctrl+a><ctrl+c>"},
    {"hello",        "Hello, World!<enter>"},
};
```

Macro bodies follow the same syntax as regular input — they can contain plain text and `<tag>` key combos.

#### Examples

```
<<hello>>                       — types "Hello, World!" then presses Enter
<<selectall>>                   — selects all text
<<openterminal>>                — opens a terminal
type something<<enter>>         — ERROR: "enter" is not a macro; use <enter> for key tags
```

#### Error handling

Unknown macro names produce an error message in the REPL session and no keypresses are sent.

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