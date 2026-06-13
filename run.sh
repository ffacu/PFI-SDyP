#!/bin/bash

# ==========================================
# Configuracion del experimento
# ==========================================
IMAGENES=("img_800.jpg" "img_2000.jpg" "img_5000.jpg")
# Formato de configuración: "Filtro Niveles" ("3 3" = Light | "5 9" = Heavy)
CONFIGURACIONES=("3 3" "5 9") 
ITERACIONES=10
ARCHIVO_CSV="metricas.csv"


# OpenMP: Usa los 32 nucleos de 1 solo nodo
HILOS_OMP=32
# MPI: Usa los 128 nucleos del cluster entero (4 nodos * 32 cores)
PROCESOS_MPI_PURO=128
# Híbrido: 4 procesos MPI (1 por nodo físico) y 32 hilos por cada uno
NODO_HIB=4
HILOS_HIB=32

echo "============================================================"
echo " INICIANDO BENCHMARK "
echo " Imágenes: 800, 2000, 5000 | Iteraciones por caso: $ITERACIONES"
echo "============================================================"

# Crear el encabezado del CSV
echo "Imagen,Filtro,Niveles,Programa,Iter1,Iter2,Iter3,Iter4,Iter5,Iter6,Iter7,Iter8,Iter9,Iter10" > $ARCHIVO_CSV

# Recorrer cada imagen
for IMG in "${IMAGENES[@]}"; do
    
    # Validar que exista
    if [ ! -f "$IMG" ]; then
        echo "ADVERTENCIA: No se encontro la imagen $IMG. Saltando..."
        continue
    fi

    # Recorrer cada configuracion (Light y Heavy)
    for CONFIG in "${CONFIGURACIONES[@]}"; do
        # Extraer filtro y posterizado del string (ej: "3 3" -> f=3, p=3)
        read F P <<< "$CONFIG"
        
        echo ""
        echo ">> TEST: Imagen=${IMG} | Filtro=${F}x${F} | Posterizado=${P} niveles"
        
        # ---------------------------------------------------------
        # 1. SECUENCIAL (Enviado al nodo1 para no saturar el master)
        # ---------------------------------------------------------
        echo -n "   [1/4] Secuencial... "
        FILA_CSV="$IMG,$F,$P,Secuencial"
        for ((i=1; i<=ITERACIONES; i++)); do
            SALIDA=$(mpirun -np 1 --host nodo1 ./bin/secuencial -i "$IMG" -o "out_sec.jpg" -f $F -p $P)
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo de procesamiento" | awk '{print $4}')
            FILA_CSV="$FILA_CSV,$TIEMPO"
        done
        echo "$FILA_CSV" >> $ARCHIVO_CSV
        echo "[OK]"

        # ---------------------------------------------------------
        # 2. OPENMP (Enviado al nodo1 para usar sus 32 cores)
        # ---------------------------------------------------------
        echo -n "   [2/4] OpenMP ($HILOS_OMP hilos)... "
        FILA_CSV="$IMG,$F,$P,OpenMP"
        for ((i=1; i<=ITERACIONES; i++)); do
            SALIDA=$(mpirun -np 1 --host nodo1 ./bin/omp -i "$IMG" -o "out_omp.jpg" -f $F -p $P -t $HILOS_OMP)
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo de procesamiento" | awk '{print $4}')
            FILA_CSV="$FILA_CSV,$TIEMPO"
        done
        echo "$FILA_CSV" >> $ARCHIVO_CSV
        echo "[OK]"

        # ---------------------------------------------------------
        # 3. MPI PURO (Distribuido usando el machinefile)
        # ---------------------------------------------------------
        echo -n "   [3/4] MPI Puro ($PROCESOS_MPI_PURO procesos)... "
        FILA_CSV="$IMG,$F,$P,MPI"
        for ((i=1; i<=ITERACIONES; i++)); do
            SALIDA=$(mpirun --hostfile machinefile -np $PROCESOS_MPI_PURO ./bin/mpi -i "$IMG" -o "out_mpi.jpg" -f $F -p $P)
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo de procesamiento" | awk '{print $4}')
            FILA_CSV="$FILA_CSV,$TIEMPO"
        done
        echo "$FILA_CSV" >> $ARCHIVO_CSV
        echo "[OK]"

        # ---------------------------------------------------------
        # 4. HÍBRIDO (Distribuido usando el machinefile)
        # ---------------------------------------------------------
        echo -n "   [4/4] Híbrido ($NODO_HIB Nodos x $HILOS_HIB Hilos)... "
        FILA_CSV="$IMG,$F,$P,Hibrido"
        for ((i=1; i<=ITERACIONES; i++)); do
            SALIDA=$(mpirun --hostfile machinefile -np $NODO_HIB ./bin/hibrido -i "$IMG" -o "out_hib.jpg" -f $F -p $P -t $HILOS_HIB)
            TIEMPO=$(echo "$SALIDA" | grep "Tiempo de procesamiento" | awk '{print $4}')
            FILA_CSV="$FILA_CSV,$TIEMPO"
        done
        echo "$FILA_CSV" >> $ARCHIVO_CSV
        echo "[OK]"

    done
done

echo ""
echo "======================================"
echo " BENCHMARK FINALIZADO."
echo " Archivo exportado: $ARCHIVO_CSV"
echo "======================================"