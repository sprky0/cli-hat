CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra
LDFLAGS = -lm      # link math library

TARGET  = clihat
SOURCES = src/main.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
