/*
按键实验
*/
#include "bsp_clk.h"
#include "delay.h"
#include "led.h"
#include "beep.h"
#include "key.h"

int main(void){
    clk_init();
    led_init();
    init_beep();
    key_init();
    while(1){
        int n = 10;
        while(n--){
            delay_1ms();
        }
        int st = key_read();
        if(st){
            led_reverse();
        }
    }
    
    return 0;
}