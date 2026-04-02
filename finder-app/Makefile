# Set the name of the final executable
TARGET := writer

# Set the list of source files
SRCS := writer.c

# Set the list of object files (matched with the list of source files)
OBJS := $(SRCS:.c=.o)

# Set the compiler to invoke.
# Use a switch for cross compilation.
# If CROSS_COMPILE argument is defined then CC := $(CROSS_COMPILE)gcc
# If CROSS_COMPILE argument is not defined then CC := gcc
CC := $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)

# Compiler flags
# If CFLAGS argument is not already defined then CFLAGS := -g -Wall
CFLAGS ?= -g -Wall

# Linker flags (can be added)
# LDFLAGS ?= 

# Extra libraries (can be added)
# LDLIBS ?= 

# Phony target declaration
.PHONY: all clean

# Default target declaration
all: $(TARGET)

# Linking rule
# TARGET depends on OBJS (OBJS should be built first and then run the recipe below)
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile rule
# Make the .o file from the corresponding .c file with the same name
# -c = compilation only, no linking
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target declaration
# Remove the generated executable and object file
clean:
	rm -f $(TARGET) *.o
