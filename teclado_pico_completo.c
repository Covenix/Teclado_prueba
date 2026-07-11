/*
 * ============================================================
 *  Teclado ortolineal 5x10 (50 teclas) - Raspberry Pi Pico (RP2040)
 *  Archivo unico, listo para pegar en el editor de flashmypico.com
 * ============================================================
 *
 * Filas (salida)     -> GPIO 17, 16, 15, 14, 13
 * Columnas (entrada) -> GPIO 12, 11, 10, 9, 8, 7, 6, 5, 4, 3
 *
 * Layout QWERTY completo + espacio + flechas:
 *
 *   1    2    3    4    5    6    7    8    9    0
 *   Q    W    E    R    T    Y    U    I    O    P
 *   A    S    D    F    G    H    J    K    L    ENTER
 *   Z    X    C    V    B    N    M    ,    .    UP
 *   CTRL ALT  SPACE SPACE SPACE SPACE SPACE LEFT DOWN RIGHT
 */

/* --------------------------------------------------------------
 * 1) CONFIGURACION DE TINYUSB
 *    Estas macros deben definirse ANTES de incluir cualquier
 *    cabecera de TinyUSB. Como tusb_config.h usa guardas
 *    "#ifndef", definirlas aqui primero hace que se respeten
 *    estos valores en vez de los que traiga la plantilla por
 *    defecto del compilador online.
 * -------------------------------------------------------------- */
#define CFG_TUSB_MCU            OPT_MCU_RP2040
#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE

#define CFG_TUD_ENDPOINT0_SIZE  64

#define CFG_TUD_HID             1
#define CFG_TUD_CDC             0
#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

#define CFG_TUD_HID_EP_BUFSIZE  16

/* --------------------------------------------------------------
 * 2) INCLUDES
 * -------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

/* --------------------------------------------------------------
 * 3) DESCRIPTORES USB (identidad del dispositivo ante el PC)
 * -------------------------------------------------------------- */

/* --- Descriptor de dispositivo --- */
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,
    .idProduct          = 0x4004,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

/* --- Descriptor de reporte HID (teclado estandar 6KRO) --- */
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return desc_hid_report;
}

/* --- Descriptor de configuracion --- */
enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID   0x81

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                           TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                        sizeof(desc_hid_report), EPNUM_HID,
                        CFG_TUD_HID_EP_BUFSIZE, 5)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

/* --- Descriptores de cadenas (strings) --- */
char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },   // 0: Idioma (ingles US)
    "Hecho en casa",                  // 1: Fabricante
    "Teclado Ortolineal 5x10",        // 2: Producto
    "123456",                          // 3: Serial
};

static uint16_t _desc_str[32];

uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;

        const char *str = string_desc_arr[index];
        chr_count = (uint8_t) strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}

/* --------------------------------------------------------------
 * 4) CONFIGURACION DE LA MATRIZ DE TECLAS
 * -------------------------------------------------------------- */
#define NUM_FILAS 5
#define NUM_COLS  10

static const uint8_t pines_fila[NUM_FILAS] = {17, 16, 15, 14, 13};
static const uint8_t pines_col[NUM_COLS]   = {12, 11, 10, 9, 8, 7, 6, 5, 4, 3};

#define DEBOUNCE_MS 5

static bool estado_actual[NUM_FILAS][NUM_COLS];
static bool estado_anterior[NUM_FILAS][NUM_COLS];
static uint32_t ultimo_cambio[NUM_FILAS][NUM_COLS];

/* --- Mapa de teclas (HID keycodes de TinyUSB) --- */
static const uint8_t keymap[NUM_FILAS][NUM_COLS] = {
    { HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5,
      HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0 },

    { HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T,
      HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P },

    { HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G,
      HID_KEY_H, HID_KEY_J, HID_KEY_K, HID_KEY_L, HID_KEY_ENTER },

    { HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B,
      HID_KEY_N, HID_KEY_M, HID_KEY_COMMA, HID_KEY_PERIOD, HID_KEY_ARROW_UP },

    { HID_KEY_CONTROL_LEFT, HID_KEY_ALT_LEFT, HID_KEY_SPACE, HID_KEY_SPACE, HID_KEY_SPACE,
      HID_KEY_SPACE, HID_KEY_SPACE, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT },
};

static uint8_t obtener_modificador(uint8_t keycode)
{
    switch (keycode) {
        case HID_KEY_CONTROL_LEFT:  return KEYBOARD_MODIFIER_LEFTCTRL;
        case HID_KEY_ALT_LEFT:      return KEYBOARD_MODIFIER_LEFTALT;
        default: return 0;
    }
}

/* --------------------------------------------------------------
 * 5) INICIALIZACION Y ESCANEO DE LA MATRIZ
 * -------------------------------------------------------------- */
static void inicializar_matriz(void)
{
    for (int f = 0; f < NUM_FILAS; f++) {
        gpio_init(pines_fila[f]);
        gpio_set_dir(pines_fila[f], GPIO_OUT);
        gpio_put(pines_fila[f], 1); // Reposo en alto (activo en bajo)
    }

    for (int c = 0; c < NUM_COLS; c++) {
        gpio_init(pines_col[c]);
        gpio_set_dir(pines_col[c], GPIO_IN);
        gpio_pull_up(pines_col[c]); // Columnas con pull-up interno
    }
}

static void escanear_matriz(void)
{
    uint32_t ahora = board_millis();

    for (int f = 0; f < NUM_FILAS; f++) {
        gpio_put(pines_fila[f], 0); // Activamos solo la fila actual
        sleep_us(5);

        for (int c = 0; c < NUM_COLS; c++) {
            bool presionada = !gpio_get(pines_col[c]);

            if (presionada != estado_anterior[f][c]) {
                ultimo_cambio[f][c] = ahora;
                estado_anterior[f][c] = presionada;
            }

            if ((ahora - ultimo_cambio[f][c]) > DEBOUNCE_MS) {
                estado_actual[f][c] = presionada;
            }
        }

        gpio_put(pines_fila[f], 1); // Desactivamos la fila
    }
}

/* --------------------------------------------------------------
 * 6) CONSTRUCCION Y ENVIO DEL REPORTE HID
 * -------------------------------------------------------------- */
static void enviar_reporte_teclado(void)
{
    if (!tud_hid_ready()) return;

    uint8_t modificadores = 0;
    uint8_t keycodes[6] = {0};
    uint8_t num_teclas = 0;

    for (int f = 0; f < NUM_FILAS; f++) {
        for (int c = 0; c < NUM_COLS; c++) {
            if (!estado_actual[f][c]) continue;

            uint8_t kc = keymap[f][c];
            uint8_t mod = obtener_modificador(kc);

            if (mod) {
                modificadores |= mod;
            } else if (num_teclas < 6) {
                bool ya_incluida = false;
                for (int i = 0; i < num_teclas; i++) {
                    if (keycodes[i] == kc) { ya_incluida = true; break; }
                }
                if (!ya_incluida) {
                    keycodes[num_teclas++] = kc;
                }
            }
        }
    }

    static uint8_t mod_anterior = 0;
    static uint8_t keys_anteriores[6] = {0};

    if (modificadores != mod_anterior || memcmp(keycodes, keys_anteriores, 6) != 0) {
        tud_hid_keyboard_report(0, modificadores, keycodes);
        mod_anterior = modificadores;
        memcpy(keys_anteriores, keycodes, 6);
    }
}

/* --------------------------------------------------------------
 * 7) CALLBACKS OBLIGATORIOS DE TINYUSB (aunque no se usen)
 * -------------------------------------------------------------- */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen)
{
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize)
{
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

/* --------------------------------------------------------------
 * 8) PROGRAMA PRINCIPAL
 * -------------------------------------------------------------- */
int main(void)
{
    board_init();
    tusb_init();
    inicializar_matriz();

    while (1) {
        tud_task();

        escanear_matriz();
        enviar_reporte_teclado();

        sleep_ms(1);
    }

    return 0;
}
