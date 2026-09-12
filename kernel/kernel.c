// kernel.c - Основное ядро
// Минимальное ядро для загрузки через GRUB

#include <stdint.h>
#include <stddef.h>

// Адрес VGA буфера (текстовый режим 80x25)
#define VGA_BUFFER 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Цвета для текста
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Глобальные переменные для позиции курсора
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

// Создание символа с цветом
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | ((uint16_t) color << 8);
}

// Получение цвета
static inline uint8_t vga_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

// Инициализация терминала
static void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) VGA_BUFFER;
    
    // Очистка экрана
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

// Установка цвета
static void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

// Установка позиции курсора
static void terminal_setpos(size_t x, size_t y) {
    terminal_row = y;
    terminal_column = x;
}

// Вывод символа
static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

// Вывод символа с переводом строки
static void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
        return;
    }
    
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

// Вывод строки
static void terminal_writestring(const char* data) {
    while (*data != 0) {
        terminal_putchar(*data);
        data++;
    }
}

// Вывод числа (десятичное)
static void terminal_writenumber(int num) {
    if (num < 0) {
        terminal_putchar('-');
        num = -num;
    }
    
    char buffer[12];
    int i = 0;
    
    if (num == 0) {
        terminal_putchar('0');
        return;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Вывод в обратном порядке
    while (--i >= 0) {
        terminal_putchar(buffer[i]);
    }
}

// Точка входа в ядро
void kernel_main(void) {
    // Инициализация терминала
    terminal_initialize();
    
    // Приветственное сообщение
    terminal_setcolor(vga_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("========================================");
    terminal_setpos(0, 1);
    terminal_writestring("   ДОБРО ПОЖАЛОВАТЬ В МОЁ ЯДРО ОС!      ");
    terminal_setpos(0, 2);
    terminal_writestring("========================================");
    
    // Информация о системе
    terminal_setcolor(vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_setpos(0, 4);
    terminal_writestring("Ядро успешно загружено через GRUB!");
    
    terminal_setpos(0, 6);
    terminal_writestring("Режим: 32-bit Protected Mode");
    
    terminal_setpos(0, 7);
    terminal_writestring("VGA Text Mode: 80x25");
    
    terminal_setpos(0, 9);
    terminal_setcolor(vga_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Тест вывода чисел: ");
    terminal_setcolor(vga_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    terminal_writenumber(12345);
    
    terminal_setpos(0, 11);
    terminal_setcolor(vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("Система готова к работе.");
    
    terminal_setpos(0, 13);
    terminal_setcolor(vga_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
    terminal_writestring("Нажмите Reset для перезагрузки.");
    
    // Бесконечный цикл
    while (1) {
        __asm__ volatile ("hlt");
    }
}
