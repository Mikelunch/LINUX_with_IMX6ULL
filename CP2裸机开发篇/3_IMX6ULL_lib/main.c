#include "imx6u.h"

void initialize(void) {
    /*使能时钟*/     
    CCM->CCGR0 = 0XFFFFFFFF;
    CCM->CCGR1 = 0XFFFFFFFF;
    CCM->CCGR2 = 0XFFFFFFFF;
    CCM->CCGR3 = 0XFFFFFFFF;
    CCM->CCGR4 = 0XFFFFFFFF;
    CCM->CCGR5 = 0XFFFFFFFF;
    CCM->CCGR6 = 0XFFFFFFFF;
}

void led_init(void){
    /*初始化led相关GPIO寄存器*/ 
    IOMUX_SW_MUX->GPIO1_IO03 = 0x5;
    IOMUX_SW_PAD->GPIO1_IO03 = 0x10B0;
    GPIO1->GDIR = 0X8;
    GPIO1->DR = 0X0;//默认初始低电平
}
void led_reverse(){
    GPIO1->DR ^= (1 <<3);
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