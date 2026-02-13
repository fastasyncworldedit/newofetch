CC = cc
CFLAGS = -O2 -Wall -Wextra -pedantic
TARGET = newofetch
SRC = src/main.c src/sysinfo.c src/config.c src/format.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
