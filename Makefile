# === Configuration ===
CC = gcc
CFLAGS = -Iinclude -Wall
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
BIN = sim

# === Build Target ===
$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

# === Clean Object Files and Binary ===
clean:
	rm -f src/*.o $(BIN)

# === Run Program ===
run: $(BIN)
	./$(BIN)
