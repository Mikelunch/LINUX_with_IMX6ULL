/*
喇叭驱动实验
*/
#include "bsp_clk.h"
#include "delay.h"
#include "led.h"
#include "beep.h"

int main(void){
    clk_init();
    led_init();
    init_beep();
    while(1){
        int n = 1000;
        while(n--){
            delay_1ms(); 
        }
        led_reverse();
    }
    return 0;
}