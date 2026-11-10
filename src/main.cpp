#include <Arduino.h>
#include <M5Cardputer.h>
#include <u8g2.h>
#include "u8g2_glue.h" 

u8g2_t u8g2; // Создаем объект экрана Flipper

void setup() {
    // 1. Включаем экран Cardputer
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // 2. Инициализируем u8g2 (виртуальный экран Flipper)
    // Передаем заглушки, которые мы написали для ESP32
    u8g2_Setup_st756x_flipper(&u8g2, U8G2_R0, u8x8_hw_spi_stm32, u8g2_gpio_and_delay_stm32);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);

    // 3. Рисуем тестовую картинку средствами Flipper (u8g2)
    u8g2_DrawFrame(&u8g2, 5, 5, 118, 54); // Рамка по краям
    u8g2_DrawBox(&u8g2, 20, 20, 30, 20);  // Квадрат
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 20, 50, "IT WORKS!");
    
    // 4. ВЫЗЫВАЕМ НАШУ ФУНКЦИЮ (перенос буфера на цветной экран Cardputer)
    draw_flipper_buffer_to_cardputer(&u8g2);
}

void loop() {
    // Ничего не делаем, экран уже нарисован
    delay(1000);
}
