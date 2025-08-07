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
	@echo "Simulation completed. Starting frontend server..."
	python3 -m http.server 8080 &
	@echo "Open your browser at: http://localhost:8080/index.html"


