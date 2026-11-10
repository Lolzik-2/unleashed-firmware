#include "u8g2_glue.h"
#include <Arduino.h>
#include <M5Cardputer.h> // Библиотека для экрана Cardputer

// Макрос для подавления предупреждений о неиспользуемых переменных
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

// --- ЗАГЛУШКИ ДЛЯ СИСТЕМНЫХ ФУНКЦИЙ FLIPPER ---
// Flipper использует свои функции задержки, мы подменяем их на Arduino
void furi_delay_ms(uint32_t ms) { delay(ms); }
void furi_delay_us(uint32_t us) { delayMicroseconds(us); }

// --- ОБРАБОТКА GPIO И ЗАДЕРЖЕК ---
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    UNUSED(u8x8);
    UNUSED(arg_ptr);
    switch(msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        // Инициализация пинов не нужна, M5Cardputer уже сделал это
        break;
    case U8X8_MSG_DELAY_MILLI:
        delay(arg_int);
        break;
    case U8X8_MSG_DELAY_10MICRO:
        delayMicroseconds(10);
        break;
    case U8X8_MSG_DELAY_100NANO:
        asm("nop");
        break;
    case U8X8_MSG_GPIO_RESET:
        // Сброс экрана управляется библиотекой M5, тут игнорируем
        break;
    default:
        return 0;
    }
    return 1;
}

// --- ОБРАБОТКА SPI (ПЕРЕХВАТ ДАННЫХ) ---
uint8_t u8x8_hw_spi_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    UNUSED(u8x8);
    switch(msg) {
    case U8X8_MSG_BYTE_SEND:
        // ВАЖНО: Flipper отправляет байты по SPI. Мы их игнорируем, 
        // так как u8g2 хранит полный буфер кадра в оперативной памяти.
        // Мы прочитаем этот буфер позже и нарисуем на экране Cardputer.
        break;
    case U8X8_MSG_BYTE_SET_DC:
        break;
    case U8X8_MSG_BYTE_INIT:
        break;
    case U8X8_MSG_BYTE_START_TRANSFER:
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        break;
    default:
        return 0;
    }
    return 1;
}

// --- КОМАНДЫ КОНТРОЛЛЕРА ST756X (ОСТАВЛЕНЫ БЕЗ ИЗМЕНЕНИЙ) ---
#define ST756X_CMD_ON_OFF           0b10101110
#define ST756X_CMD_SET_LINE         0b01000000
#define ST756X_CMD_SET_PAGE         0b10110000
#define ST756X_CMD_SET_COLUMN_MSB   0b00010000
#define ST756X_CMD_SET_COLUMN_LSB   0b00000000
#define ST756X_CMD_SEG_DIRECTION    0b10100000
#define ST756X_CMD_INVERSE_DISPLAY  0b10100110
#define ST756X_CMD_ALL_PIXEL_ON     0b10100100
#define ST756X_CMD_BIAS_SELECT      0b10100010
#define ST756X_CMD_R_M_W            0b11100000
#define ST756X_CMD_END              0b11101110
#define ST756X_CMD_RESET            0b11100010
#define ST756X_CMD_COM_DIRECTION    0b11000000
#define ST756X_CMD_POWER_CONTROL    0b00101000
#define ST756X_CMD_REGULATION_RATIO 0b00100000
#define ST756X_CMD_SET_EV           0b10000001
#define ST756X_CMD_SET_BOOSTER      0b11111000
#define ST756X_CMD_NOP              0b11100011

static const uint8_t u8x8_d_st756x_powersave0_seq[] = {
    U8X8_START_TRANSFER(), U8X8_C(ST756X_CMD_ALL_PIXEL_ON | 0b0), U8X8_C(ST756X_CMD_ON_OFF | 0b1), U8X8_END_TRANSFER(), U8X8_END()};
static const uint8_t u8x8_d_st756x_powersave1_seq[] = {
    U8X8_START_TRANSFER(), U8X8_C(ST756X_CMD_ON_OFF | 0b0), U8X8_C(ST756X_CMD_ALL_PIXEL_ON | 0b1), U8X8_END_TRANSFER(), U8X8_END()};
static const uint8_t u8x8_d_st756x_flip0_seq[] = {
    U8X8_START_TRANSFER(), U8X8_C(0x0a1), U8X8_C(0x0c0), U8X8_END_TRANSFER(), U8X8_END()};
static const uint8_t u8x8_d_st756x_flip1_seq[] = {
    U8X8_START_TRANSFER(), U8X8_C(0x0a0), U8X8_C(0x0c8), U8X8_END_TRANSFER(), U8X8_END()};

static const u8x8_display_info_t u8x8_st756x_128x64_display_info = {
    .chip_enable_level = 0, .chip_disable_level = 1, .post_chip_enable_wait_ns = 150,
    .pre_chip_disable_wait_ns = 50, .reset_pulse_width_ms = 1, .post_reset_wait_ms = 1,
    .sda_setup_time_ns = 50, .sck_pulse_width_ns = 120, .sck_clock_hz = 4000000UL,
    .spi_mode = 0, .i2c_bus_clock_100kHz = 4, .data_setup_time_ns = 40,
    .write_pulse_width_ns = 80, .tile_width = 16, .tile_height = 8,
    .default_x_offset = 0, .flipmode_x_offset = 4, .pixel_width = 128, .pixel_height = 64};

uint8_t u8x8_d_st756x_common(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    uint8_t x, c;
    uint8_t* ptr;

    switch(msg) {
    case U8X8_MSG_DISPLAY_DRAW_TILE:
        u8x8_cad_StartTransfer(u8x8);
        x = ((u8x8_tile_t*)arg_ptr)->x_pos;
        x *= 8;
        x += u8x8->x_offset;
        u8x8_cad_SendCmd(u8x8, 0x010 | (x >> 4));
        u8x8_cad_SendCmd(u8x8, 0x000 | (x & 15));
        u8x8_cad_SendCmd(u8x8, 0x0b0 | (((u8x8_tile_t*)arg_ptr)->y_pos));

        c = ((u8x8_tile_t*)arg_ptr)->cnt;
        c *= 8;
        ptr = ((u8x8_tile_t*)arg_ptr)->tile_ptr;
        if(c + x > 132u) {
            c = 132u;
            c -= x;
        }
        do {
            u8x8_cad_SendData(u8x8, c, ptr);
            arg_int--;
        } while(arg_int > 0);
        u8x8_cad_EndTransfer(u8x8);
        break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
        if(arg_int == 0) u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_powersave0_seq);
        else u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_powersave1_seq);
        break;
#ifdef U8X8_WITH_SET_CONTRAST
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_EV);
        u8x8_cad_SendArg(u8x8, arg_int >> 2);
        u8x8_cad_EndTransfer(u8x8);
        break;
#endif
    default:
        return 0;
    }
    return 1;
}

void u8x8_d_st756x_init(u8x8_t* u8x8, uint8_t contrast, uint8_t regulation_ratio, bool bias) {
    contrast = contrast & 0b00111111;
    regulation_ratio = regulation_ratio & 0b111;
    u8x8_cad_StartTransfer(u8x8);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_RESET);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_BIAS_SELECT | bias);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SEG_DIRECTION);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_COM_DIRECTION | 0b1000);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_LINE);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_REGULATION_RATIO | regulation_ratio);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_EV);
    u8x8_cad_SendArg(u8x8, contrast);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_POWER_CONTROL | 0b111);
    u8x8_cad_EndTransfer(u8x8);
}

void u8x8_d_st756x_set_contrast(u8x8_t* u8x8, int8_t contrast_offset) {
    uint8_t contrast = 32; // Заглушка, так как у Cardputer нет ST756x
    contrast += contrast_offset;
    contrast = contrast & 0b00111111;
    u8x8_cad_StartTransfer(u8x8);
    u8x8_cad_SendCmd(u8x8, ST756X_CMD_SET_EV);
    u8x8_cad_SendArg(u8x8, contrast);
    u8x8_cad_EndTransfer(u8x8);
}

uint8_t u8x8_d_st756x_flipper(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    if(u8x8_d_st756x_common(u8x8, msg, arg_int, arg_ptr) == 0) {
        switch(msg) {
        case U8X8_MSG_DISPLAY_SETUP_MEMORY:
            u8x8_d_helper_display_setup_memory(u8x8, &u8x8_st756x_128x64_display_info);
            break;
        case U8X8_MSG_DISPLAY_INIT:
            u8x8_d_helper_display_init(u8x8);
            u8x8_d_st756x_init(u8x8, 32, 0b101, false); // Инициализируем как ERC версию
            break;
        case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
            if(arg_int == 0) {
                u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_flip1_seq);
                u8x8->x_offset = u8x8->display_info->default_x_offset;
            } else {
                u8x8_cad_SendSequence(u8x8, u8x8_d_st756x_flip0_seq);
                u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
            }
            break;
        default:
            return 0;
        }
    }
    return 1;
}

void u8g2_Setup_st756x_flipper(u8g2_t* u8g2, const u8g2_cb_t* rotation, u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb) {
    uint8_t tile_buf_height;
    uint8_t* buf;
    u8g2_SetupDisplay(u8g2, u8x8_d_st756x_flipper, u8x8_cad_001, byte_cb, gpio_and_delay_cb);
    buf = u8g2_m_16_8_f(&tile_buf_height);
    u8g2_SetupBuffer(u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, rotation);
}

void u8x8_d_st756x_set_inversion(u8x8_t* u8x8, bool arg) {
    if(arg) {
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, ST756X_CMD_INVERSE_DISPLAY | 0b01);
        u8x8_cad_EndTransfer(u8x8);
    } else {
        u8x8_cad_StartTransfer(u8x8);
        u8x8_cad_SendCmd(u8x8, ST756X_CMD_INVERSE_DISPLAY);
        u8x8_cad_EndTransfer(u8x8);
    }
}

// ==============================================================================
// НАША НОВАЯ ФУНКЦИЯ ДЛЯ ОТРИСОВКИ БУФЕРА FLIPPER НА ЭКРАН CARDputer
// ==============================================================================
void draw_flipper_buffer_to_cardputer(u8g2_t* u8g2) {
    // Получаем указатель на внутренний буфер u8g2 (128x64 пикселей)
    uint8_t* buffer = u8g2_GetBufferPtr(u8g2);
    if (!buffer) return;

    int screen_w = 128;
    int screen_h = 64;
    int scale = 2; // Увеличиваем в 2 раза, чтобы было видно на большом экране

    // Сдвиг для центрирования по экрану 240x135 (с учетом масштаба)
    int offset_x = (240 - (screen_w * scale)) / 2;
    int offset_y = (135 - (screen_h * scale)) / 2;

    // Начинаем пакетную отрисовку (быстрее, чем попиксельно)
    M5Cardputer.Display.startWrite();

    for (int y = 0; y < screen_h; y++) {
        for (int x = 0; x < screen_w; x++) {
            // Вычисляем байт в буфере u8g2
            int byte_index = (y / 8) * screen_w + x;
            int bit_index = y % 8;
            
            // Проверяем, закрашен ли пиксель (1 - белый, 0 - черный)
            bool is_white = (buffer[byte_index] >> bit_index) & 1;
            
            uint16_t color = is_white ? TFT_WHITE : TFT_BLACK;

            // Рисуем пиксель с учетом масштаба (2х2 блока)
            for (int dy = 0; dy < scale; dy++) {
                for (int dx = 0; dx < scale; dx++) {
                    M5Cardputer.Display.drawPixel(offset_x + (x * scale) + dx, offset_y + (y * scale) + dy, color);
                }
            }
        }
    }

    M5Cardputer.Display.endWrite();
}
