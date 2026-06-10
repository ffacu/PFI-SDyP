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
# Archivos Objeto
# ==========================================

# Objetos secuenciales (usados por secuencial y mpi)
SEQ_OBJ = $(OBJ_DIR)/image_io.o $(OBJ_DIR)/filters.o $(OBJ_DIR)/posterize.o

# Objetos paralelos con OpenMP (usados por omp e hibrido)
PAR_OBJ = $(OBJ_DIR)/image_io.o $(OBJ_DIR)/filters_parallel.o $(OBJ_DIR)/posterize_parallel.o

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
secuencial: dirs $(SEQ_OBJ) $(OBJ_DIR)/main_sec.o
	$(CC) $(CFLAGS) $(SEQ_OBJ) $(OBJ_DIR)/main_sec.o -o $(BIN_DIR)/secuencial $(LDFLAGS)

# 2. Memoria Compartida (OpenMP)
omp: dirs $(PAR_OBJ) $(OBJ_DIR)/main_omp.o
	$(CC) $(CFLAGS) $(OMPFLAGS) $(PAR_OBJ) $(OBJ_DIR)/main_omp.o -o $(BIN_DIR)/omp $(LDFLAGS)

# 3. Memoria Distribuida (MPI)
mpi: dirs $(SEQ_OBJ) $(OBJ_DIR)/main_mpi.o
	$(MPICC) $(CFLAGS) $(SEQ_OBJ) $(OBJ_DIR)/main_mpi.o -o $(BIN_DIR)/mpi $(LDFLAGS)

# 4. Híbrida (MPI + OpenMP)
hibrido: dirs $(PAR_OBJ) $(OBJ_DIR)/main_hibrido.o
	$(MPICC) $(CFLAGS) $(OMPFLAGS) $(PAR_OBJ) $(OBJ_DIR)/main_hibrido.o -o $(BIN_DIR)/hibrido $(LDFLAGS)

# ==========================================
# Reglas de compilación de objetos
# ==========================================

# Regla genérica para archivos .c (incluye -fopenmp para los _parallel)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OMPFLAGS) -c $< -o $@

# Reglas específicas para archivos que requieren mpicc (necesitan <mpi.h>)
$(OBJ_DIR)/main_mpi.o: $(SRC_DIR)/main_mpi.c
	$(MPICC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/main_hibrido.o: $(SRC_DIR)/main_hibrido.c
	$(MPICC) $(CFLAGS) $(OMPFLAGS) -c $< -o $@

# ==========================================
# Limpieza general
# ==========================================
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)