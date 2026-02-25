# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(bad_pico_usb C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


# Add source files for the sequencer, commands, and UI components
add_executable(bad_pico_usb 
    src/bad_pico_usb.cpp
    src/usb_descriptors.c
    src/wifi_repl.c
    src/dhserver.c
)

# binary info (readabl by picotool)
pico_set_program_name(bad_pico_usb "Bad Pico USB")
pico_set_program_version(bad_pico_usb "0.1")
pico_set_program_description(bad_pico_usb "Generative MIDI Sequence")
pico_set_program_url(bad_pico_usb "https://github.com/woeps/bad-pico-usb")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(bad_pico_usb 1)
pico_enable_stdio_usb(bad_pico_usb 0)

# Add the standard library to the build
target_link_libraries(bad_pico_usb
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_uart
    hardware_adc
    hardware_gpio
    hardware_i2c
    hardware_dma
    tinyusb_device
    tinyusb_board
    pico_cyw43_arch_lwip_threadsafe_background
)

# Add the standard include files to the build
target_include_directories(bad_pico_usb PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
	#${CMAKE_CURRENT_LIST_DIR}/src/generated
)

target_compile_definitions(bad_pico_usb PRIVATE
    WIFI_SSID="BadPicoKB"
    WIFI_PASSWORD="badpico1"
    REPL_PORT=4242
)


pico_add_extra_outputs(bad_pico_usb)
