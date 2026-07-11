cmake_minimum_required(VERSION 3.13)

# Incluye el SDK de Pico (debes tener pico_sdk_import.cmake en esta carpeta,
# se copia desde <ruta-al-pico-sdk>/external/pico_sdk_import.cmake)
include(pico_sdk_import.cmake)

project(pico_keyboard C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializa el SDK
pico_sdk_init()

add_executable(pico_keyboard
    src/main.c
    src/usb_descriptors.c
)

target_include_directories(pico_keyboard PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Librerías necesarias: stdlib, gpio, y pila USB (tinyusb) en modo device
target_link_libraries(pico_keyboard PUBLIC
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

# Genera .uf2, .hex, etc.
pico_add_extra_outputs(pico_keyboard)

# No usamos stdio por USB (ya usamos USB para HID), solo por UART si se quiere debug
pico_enable_stdio_uart(pico_keyboard 1)
pico_enable_stdio_usb(pico_keyboard 0)
