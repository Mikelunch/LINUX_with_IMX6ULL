#include "key.h"
#include "delay.h"

void key_init(void){
    IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18 , 0x0);
    IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18,0xf080);
    GPIO1->GDIR &= ~(1 << 18);//设置为输入模式
}

int GPIO_read(int pin) {
    return (((GPIO1->DR) >> pin) & 0x1);
}

int key_read() {
    int ret = 0;
    static unsigned char release = 1;
    if((release == 1) && (GPIO_read(18) == 0)){
        int n = 10;
        while(n--) {
            delay_1ms();
        }
        release = 0;
        if(GPIO_read(18) == 0){
            ret = 1;
        }
    }
    else if(GPIO_read(18) == 1){
        ret = 0;
        release = 1;
    }
    
    return ret;
}