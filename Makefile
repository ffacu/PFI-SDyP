CC = gcc
MPICC = mpicc

CFLAGS = -Wall -O3 -I./include
OMPFLAGS = -fopenmp
LDFLAGS = -lm

# ==========================================
# Directorios
# ==========================================
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# ==========================================
# Archivos Comunes
# ==========================================
COMMON_SRC = $(SRC_DIR)/image_io.c $(SRC_DIR)/filters.c $(SRC_DIR)/posterize.c
COMMON_OBJ = $(COMMON_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# ==========================================
# Targets
# ==========================================
.PHONY: all clean dirs secuencial omp mpi hibrido

all: dirs secuencial omp mpi hibrido

# Crea las carpetas necesarias si no existen
dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# ==========================================
# Compilación de Ejecutables
# ==========================================

# 1. Secuencial
secuencial: dirs $(COMMON_OBJ) $(OBJ_DIR)/main_sec.o
	$(CC) $(CFLAGS) $(COMMON_OBJ) $(OBJ_DIR)/main_sec.o -o $(BIN_DIR)/secuencial $(LDFLAGS)

# 2. Memoria Compartida
omp: dirs $(COMMON_OBJ) $(OBJ_DIR)/main_omp.o
	$(CC) $(CFLAGS) $(OMPFLAGS) $(COMMON_OBJ) $(OBJ_DIR)/main_omp.o -o $(BIN_DIR)/omp $(LDFLAGS)

# 3. Memoria Distribuida
mpi: dirs $(COMMON_OBJ) $(OBJ_DIR)/main_mpi.o
	$(MPICC) $(CFLAGS) $(COMMON_OBJ) $(OBJ_DIR)/main_mpi.o -o $(BIN_DIR)/mpi $(LDFLAGS)

# 4. Híbrida (MPI + OpenMP)
hibrido: dirs $(COMMON_OBJ) $(OBJ_DIR)/main_hibrido.o
	$(MPICC) $(CFLAGS) $(OMPFLAGS) $(COMMON_OBJ) $(OBJ_DIR)/main_hibrido.o -o $(BIN_DIR)/hibrido $(LDFLAGS)

# ==========================================
# Regla para compilar los archivos .c a objetos .o
# ==========================================
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OMPFLAGS) -c $< -o $@

# ==========================================
# Limpieza general
# ==========================================
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)