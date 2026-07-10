CC = gcc
CFLAGS = -Wall -g
LIBS = -lncurses

SOURCES = editor.c rope.c undo.c trie.c hash.c search.c
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

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all ./$(TARGET)

benchmark: rope.o benchmark.c
	$(CC) $(CFLAGS) -o benchmark benchmark.c rope.o
	./benchmark

.PHONY: all clean run valgrind benchmark
