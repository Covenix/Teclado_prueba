# Teclado ortolineal 5×10 para Raspberry Pi Pico (RP2040)

50 teclas, layout QWERTY completo + espacio + flechas.

## Conexionado

| | Col0(12) | Col1(11) | Col2(10) | Col3(9) | Col4(8) | Col5(7) | Col6(6) | Col7(5) | Col8(4) | Col9(3) |
|---|---|---|---|---|---|---|---|---|---|---|
| **Fila0 (17)** | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 |
| **Fila1 (16)** | Q | W | E | R | T | Y | U | I | O | P |
| **Fila2 (15)** | A | S | D | F | G | H | J | K | L | ENTER |
| **Fila3 (14)** | Z | X | C | V | B | N | M | , | . | ↑ |
| **Fila4 (13)** | CTRL | ALT | ESPACIO | ESPACIO | ESPACIO | ESPACIO | ESPACIO | ← | ↓ | → |

- Filas: GPIO 17, 16, 15, 14, 13 → salida, se ponen a nivel bajo una a una.
- Columnas: GPIO 12, 11, 10, 9, 8, 7, 6, 5, 4, 3 → entrada con pull-up interno.
- Cada tecla conecta su fila y columna correspondientes (diodo recomendado
  en serie con cada tecla, cátodo hacia la columna, para evitar "ghosting"
  si se pulsan varias teclas a la vez).

## Requisitos para compilar

1. Instalar el **Pico SDK** (incluye TinyUSB como submódulo):
   ```bash
   git clone -b master https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   export PICO_SDK_PATH=$(pwd)
   ```

2. Copiar el archivo `pico_sdk_import.cmake` a la raíz de este proyecto:
   ```bash
   cp $PICO_SDK_PATH/external/pico_sdk_import.cmake .
   ```

3. Compilar:
   ```bash
   mkdir build && cd build
   cmake -DPICO_BOARD=pico ..    # o pico2 si usas RP2350
   make -j4
   ```

4. Se generará `pico_keyboard.uf2`. Conecta la Pico en modo BOOTSEL
   (mantén pulsado el botón BOOTSEL al enchufarla) y copia el `.uf2`
   a la unidad USB que aparece (RPI-RP2).

## Notas

- El reporte HID es de 6 teclas simultáneas (6KRO), estándar para teclados
  USB básicos. Si necesitas más teclas simultáneas (NKRO), habría que
  cambiar el descriptor HID a un reporte tipo bitmap.
- El antirrebote (debounce) está fijado en 5 ms, ajustable en `DEBOUNCE_MS`
  dentro de `src/main.c`.
- El espacio ocupa 5 teclas físicas de la última fila; puedes reasignar
  cualquiera de esas posiciones a otra tecla editando el array `keymap`.
- Si compilas para una Pico 2 (RP2350), usa `-DPICO_BOARD=pico2` al
  invocar `cmake`; el resto del código es compatible sin cambios porque
  usa la API estándar de `pico_stdlib` y TinyUSB.
