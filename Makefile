SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CFLAGS = -g -O0
LDFLAGS = -lm

TARGET = output

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET)
