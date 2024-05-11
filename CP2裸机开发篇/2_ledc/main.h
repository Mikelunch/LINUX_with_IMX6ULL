#ifndef __MAIN_H
#define __MAIN_H

//GPIO时钟使能寄存器
#define CCM_CCGR0 *((volatile unsigned int*)0x020c4068) //定义CCGRO寄存器地址
#define CCM_CCGR1 *((volatile unsigned int*)0x020c406C) //定义CCGR1
#define CCM_CCGR2 *((volatile unsigned int*)0x020c4070) //定义CCGR2
#define CCM_CCGR3 *((volatile unsigned int*)0x020c4074) //定义CCGR3
#define CCM_CCGR4 *((volatile unsigned int*)0x020c4078) //定义CCGR4
#define CCM_CCGR5 *((volatile unsigned int*)0x020c407C) //定义CCGR5
#define CCM_CCGR6 *((volatile unsigned int*)0x020c4080) //定义CCGR6

//IOMUX相关寄存器地址
#define SW_MUX_GPIO1_IO03 *((volatile unsigned int*)0x020e0068)
#define SW_PAD_GPIO1_IO03 *((volatile unsigned int*)0x020e02f4)

//GPIO1相关寄存器地址
#define GPIO1_DR *((volatile unsigned int*)0x0209c000) //GPIO数据寄存器
#define GPIO1_GDIR *((volatile unsigned int*)0x0209c004) //gpio方向控制寄存器
#define GPIO01_PSR *((volatile unsigned int*)0x0209c008) //
#define GPIO01_ICR1 *((volatile unsigned int*)0x0209c00C)
#define GPIO01_ICR2 *((volatile unsigned int*)0x0209c010)
#define GPIO01_IMR *((volatile unsigned int*)0x0209c014)
#define GPIO01_ISR *((volatile unsigned int*)0x0209c018)
#define GPIO01_EDGE_SEL *((volatile unsigned int*)0x0209c01C)


#endif // !__MAIN_H_
