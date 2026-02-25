# bad-pico-usb

This project makes raspberry pi pico act as a "bad" usb device.

## Current Behavior

- Enumerates as a USB HID keyboard as soon as the Pico is plugged into a host.
- Waits for 30 seconds after the host has mounted the device.
- Types the string `hello world` automatically into the currently focused text input.

## Planned Features

- provide a wifi hotspot to connect to the pi pico
- provide a way to trigger payload execution, once connected to the wifi
- provide a way to trigger payload execution, once a specific bluetooth device gets visible
- first supported payload is sending key presses to the operating system
    - either pre-prepaired list of keypresses
    - or custom one-shot list of keypresses

## Technichal Details

- use the pico sdk
