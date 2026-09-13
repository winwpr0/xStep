# Grape Compiler Makefile
# Fast native compilation

CC = gcc
CFLAGS = -O3 -std=c11 -Wall -Wextra -march=native
TARGET = grape
SRC = grape_compiler.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) output.c output

test: $(TARGET)
	@echo "Creating test Grape program..."
	@echo 'int main { return 0; }' > test.grape
	./$(TARGET) test.grape test_output.c
	@echo "Test compilation successful!"

run: $(TARGET)
	./$(TARGET)

# Install to /usr/local/bin
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/grape
	chmod +x /usr/local/bin/grape
	@echo "Grape compiler installed to /usr/local/bin/grape"
