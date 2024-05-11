SECTIONS{
    . = 0X87800000;
    .text :
    {
        start.o
        *(.text)
    }
    .rndata ALIGN(4) ; {*(.rndata)}
    .data ALIGN(4) : {*(.data)}
    __bss_start = .;
    .bss ALIGN(4) : {*(.bss) *(COMMON)}
    __bss_end = .;
}