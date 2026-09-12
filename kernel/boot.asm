; boot.asm - Загрузочный код для ядра
; Точка входа, установка защищённого режима

[bits 32]
[global start]

extern kernel_main

start:
    ; Установка указателя стека
    mov esp, stack_top
    
    ; Вызов функции ядра
    call kernel_main
    
    ; Бесконечный цикл (если ядро вернётся)
    cli
.hang:
    hlt
    jmp .hang

; Резервирование места под стек (8KB)
section .bss
align 16
stack_bottom:
    resb 8192
stack_top:
