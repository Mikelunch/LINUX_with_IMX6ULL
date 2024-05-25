#include "beep.h"

void init_beep(void){
    IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01 , 0X0);
    IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0X10b0);
    GPIO5->GDIR |= (1<<1);
    GPIO5->DR |= (1<<1);//初始化关闭
}