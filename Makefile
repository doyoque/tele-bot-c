# variables
CC = gcc
CFLAGS = -Wall -Wextra -O2
SRCDIR = src
BINDIR = bin
TARGET = $(BINDIR)/main

# find all .c files in the source directory
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BINDIR)/%.o)

# default target
all: $(BINDIR) $(TARGET)

# create binary directory if it does not exist
$(BINDIR):
	mkdir -p $(BINDIR)

# compile the target program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# compile object files
$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean up
clean:
	rm -rf $(BINDIR)

.PHONY: all clean
