CC = cc
CFLAGS = -O2 -Wall -Wextra -pedantic
TARGET = newofetch

all: $(TARGET)

$(TARGET): newofetch.c
	$(CC) $(CFLAGS) -o $(TARGET) newofetch.c

clean:
	rm -f $(TARGET)
