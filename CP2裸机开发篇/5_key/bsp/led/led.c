#include "led.h"
void led_init(){
    IOMUXC_SetPinMux(IOMUXC_GPIO1_IO03_GPIO1_IO03 , 0x0);
    IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO03_GPIO1_IO03 , 0x10B0);
    GPIO1->GDIR = 0x08;
    GPIO1->DR = (1 << 3);
}
void led_reverse(){
    GPIO1->DR ^= (1 << 3);
}