# Makefile для сборки ядра ОС

# Компиляторы и инструменты
CC = i686-elf-gcc
LD = i686-elf-ld
ASM = nasm
GRUB_MKRESUE = grub-mkrescue

# Флаги компиляции
CFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-pie -no-pie -m32
LDFLAGS = -T linker.ld -nostdlib -m elf_i386
ASMFLAGS = -f elf32

# Файлы исходного кода
C_SOURCES = kernel.c
ASM_SOURCES = boot.asm

# Объектные файлы
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)
OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

# Целевой файл ядра
KERNEL = myos.kernel

# ISO образ
ISO = myos.iso

# Папка для GRUB
ISODIR = isodir

all: $(KERNEL)

# Сборка ядра
$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJECTS)

# Компиляция C файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ассемблирование ASM файлов
%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Создание загрузочного ISO образа
iso: $(KERNEL)
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/
	echo 'set timeout=0' > $(ISODIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISODIR)/boot/grub/grub.cfg
	echo '' >> $(ISODIR)/boot/grub/grub.cfg
	echo 'menuentry "My OS" {' >> $(ISODIR)/boot/grub/grub.cfg
	echo '    multiboot /boot/$(KERNEL)' >> $(ISODIR)/boot/grub/grub.cfg
	echo '    boot' >> $(ISODIR)/boot/grub/grub.cfg
	echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO) $(ISODIR)
	rm -rf $(ISODIR)
	@echo "ISO образ создан: $(ISO)"

# Запуск в эмуляторе QEMU
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

# Запуск из ISO образа
run-iso: iso
	qemu-system-i386 -cdrom $(ISO)

# Очистка
clean:
	rm -f $(OBJECTS) $(KERNEL) $(ISO)
	rm -rf $(ISODIR)

.PHONY: all clean run run-iso iso
