#include "main.h"
void initialize(void) {
    /*使能时钟*/    
    CCM_CCGR0 = 0XFFFFFFFF;
    CCM_CCGR1 = 0XFFFFFFFF;
    CCM_CCGR2 = 0XFFFFFFFF;
    CCM_CCGR3 = 0XFFFFFFFF;
    CCM_CCGR4 = 0XFFFFFFFF;
    CCM_CCGR5 = 0XFFFFFFFF;
    CCM_CCGR6 = 0XFFFFFFFF;  
}

void led_init(void){
    /*初始化led相关GPIO寄存器*/ 
    SW_MUX_GPIO1_IO03 = 0X5;
    SW_PAD_GPIO1_IO03 = 0X10B0;
    GPIO1_GDIR = 0X8;
    GPIO1_DR = 0X0;//默认初始低电平
}
void led_reverse(){
    GPIO1_DR ^= (1 << 3);
}
void delay_1ms(){
    int n = 0x7ff;
    while(n--){} //主频396M下
}  
int main(void){
    initialize();
    led_init();

    while(1){
        int n = 1000;
        while(n--){
            delay_1ms();
        }
        led_reverse();
    }

    return 0;
}

