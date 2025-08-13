CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g
TARGET=func_test
SOURCES=test.c tokens.c lexer.c
OUTPUT_DIR=out

all: $(OUTPUT_DIR)/$(TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(OUTPUT_DIR)/$(TARGET): $(SOURCES) $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

clean:
	rm -rf $(OUTPUT_DIR)

.PHONY: all clean