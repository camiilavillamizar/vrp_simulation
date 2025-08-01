# === Configuration ===
CC = mpicc
CFLAGS = -Iinclude -Wall
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = sim
OUTLINE_DIR = output

# === Build Target ===
$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# === Clean Object Files and Binary ===
clean:
	rm -f src/*.o $(BIN)

# === Run with MPI ===
run: $(BIN)
	mpirun -np 5 ./$(BIN)

# === Convert PPM to PNG ===
convert: $(OUTLINE_DIR)/map/initial_map.ppm
	magick $(OUTLINE_DIR)/map/initial_map.ppm $(OUTLINE_DIR)/map/initial_map.png