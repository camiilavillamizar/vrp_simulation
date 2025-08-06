# === Configuration ===
CC = mpicc
CFLAGS = -Wall -fopenmp -Iinclude
SRC = $(wildcard src/*.c)
BIN = sim
OBJ = $(SRC:.c=.o)
OUTLINE_DIR = output

# === Build Target ===
$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) -lm -ljson-c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# === Clean Object Files and Binary ===
clean:
	rm -f src/*.o $(BIN)

# === Run with MPI ===
run: $(BIN)
	mpirun --allow-run-as-root -np 4 ./$(BIN)


