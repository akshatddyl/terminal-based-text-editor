CC = gcc
CFLAGS = -Wall -g
LIBS = -lncurses

SOURCES = editor.c rope.c undo.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = texteditor

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
