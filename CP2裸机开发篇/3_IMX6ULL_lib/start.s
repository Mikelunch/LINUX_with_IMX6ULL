.global _bss_start 
_bss_start:
    .word _bss_start
.global _bss_end
_bss_end:
    .word _bss_end
.global _start @全局变量
_start:
    @设置处理器进入SVC模式（超级系统用户模式）
    mrs r0, cpsr
    bic r0, r0, #0x1f @清除cpsr寄存器的bit0-bit4
    orr r0, r0, #0x13 @进入SVC模式
    msr cpsr, r0

    @手动清除bss段
    ldr r0 , _bss_start
    ldr r1 , _bss_end
    mov r2,#0
bss_loop:
    stmia r0! , {r2}
    cmp r0 , r1
    ble bss_loop
    
    @设置sp指针
    ldr sp, =0x80200000 @从0x80000000开始的2MB为栈空间 开发板上固定DDR的地址从0x80000开始
    b main @跳转到主函数区

    @设置完毕就可以跳转到.c文件开始执行
