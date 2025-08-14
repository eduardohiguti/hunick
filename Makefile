# Compiler
CC = gcc

# --- Directories ---
BINDIR = bin
SRCDIR = src

# Source subdirectories
INCDIR = $(SRCDIR)/include
COREDIR = $(SRCDIR)/core
FRONTENDDIR = $(SRCDIR)/frontend
RUNTIMEDIR = $(SRCDIR)/runtime

# --- Compilation Flags ---
CFLAGS = -g -Wall -I$(INCDIR)

# --- Sources and Objects ---
VPATH = $(COREDIR):$(FRONTENDDIR):$(RUNTIMEDIR)

SOURCES := $(wildcard $(COREDIR)/*.c) \
           $(wildcard $(FRONTENDDIR)/*.c) \
           $(wildcard $(RUNTIMEDIR)/*.c)

SOURCES_BASENAME := $(notdir $(SOURCES))
OBJECTS := $(patsubst %.c, $(BINDIR)/%.o, $(SOURCES_BASENAME))

TARGET = $(BINDIR)/hunick

# --- Rules ---
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm
	@echo "Hunick successfully compiled to $(TARGET)"

# Compile .c to .o, only rebuild if source or headers change
$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create bin directory if missing
$(BINDIR):
	mkdir -p $(BINDIR)

# Clean compiled files
clean:
	@echo "Cleaning compiled files..."
	rm -rf $(BINDIR)

.PHONY: all clean

# --- Automatic header dependencies ---
# This will generate .d files for each .c file to track included headers
DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)

$(BINDIR)/%.d: %.c | $(BINDIR)
	$(CC) -M $(CFLAGS) $< > $@.tmp
	sed 's|^\(.*\)\.o:|\1.o:|' $@.tmp > $@
	rm -f $@.tmp