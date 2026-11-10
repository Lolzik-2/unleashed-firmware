#ifndef U8G2_GLUE_H
#define U8G2_GLUE_H

#include <u8g2.h>

#ifdef __cplusplus
extern "C" {
#endif

// Функции для обработки задержек и GPIO (заглушка под STM32)
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

// Функция для обработки SPI (мы перехватываем данные)
uint8_t u8x8_hw_spi_stm32(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr);

// Инициализация виртуального дисплея Flipper (ST756x)
void u8g2_Setup_st756x_flipper(
    u8g2_t* u8g2,
    const u8g2_cb_t* rotation,
    u8x8_msg_cb byte_cb,
    u8x8_msg_cb gpio_and_delay_cb
);

// Наша кастомная функция: берет буфер Flipper'а и рисует его на цветном экране Cardputer
void draw_flipper_buffer_to_cardputer(u8g2_t* u8g2);

#ifdef __cplusplus
}
#endif

#endif // U8G2_GLUE_H
