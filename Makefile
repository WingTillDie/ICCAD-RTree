SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CFLAGS = $(CFLAGS-$@) -g -O3 -ftree-parallelize-loops=7
LDFLAGS = -lm -fopenmp

TARGET = cada025

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET)

CFLAGS-asm += -S
asm:
	$(CC) $(CFLAGS) $(SRC)
