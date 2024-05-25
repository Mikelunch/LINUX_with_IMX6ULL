# 峰鸣器实验
主要进行了如下函数的编写

* ### 驱动峰鸣器
实现了`init_beep`驱动。代码如下
```
void init_beep(void){
    IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01 , 0X0);
    IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01, 0X10b0);
    GPIO5->GDIR |= (1<<1);
    GPIO5->DR |= (1<<1);//初始化关闭
}
```
值得注意的是**IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01管脚是复用管脚**，需要使用IOMUXC_SNVS_SNVS_TAMPER1_GPIO5_IO01。