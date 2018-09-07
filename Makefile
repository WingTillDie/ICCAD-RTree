SRC = $(wildcard *.cc)
OBJ = $(SRC:.cc=.o)

CXXFLAGS = $(CXXFLAGS-$@) -O3 -ftree-parallelize-loops=7
#Add -g in debugging build
LDFLAGS = -lm -fopenmp

TARGET = cada025

all: $(TARGET)

$(TARGET): $(OBJ)
#	$(CC) -o $@ $^ $(LDFLAGS)
	g++ -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET)

CFLAGS-asm += -S
asm: 
#	$(CC) $(CFLAGS) $(SRC)
	g++ $(CFLAGS) $(SRC)
