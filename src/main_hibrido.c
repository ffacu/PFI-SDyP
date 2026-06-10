#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>
#include <omp.h>
#include <sys/time.h>
#include "image_io.h"
#include "filters.h"
#include "posterize.h"


/*
 * Intercambio de halos (ghost rows) entre procesos MPI vecinos.
 * Reutiliza el mismo esquema que main_mpi.c.
 * Solo el hilo maestro invoca esta función (MPI_THREAD_FUNNELED).
 */
void halo_exchange(unsigned char *extended_buf, int local_rows, int width,
                   int halo_size, int ghost_top, int ghost_bottom,
                   int rank, int size) {
    
    int up_neighbor   = (rank > 0)        ? rank - 1 : MPI_PROC_NULL;
    int down_neighbor = (rank < size - 1) ? rank + 1 : MPI_PROC_NULL;
    
    int halo_bytes = halo_size * width;
    
    // Paso 1: Intercambio hacia abajo
    MPI_Sendrecv(
        extended_buf + (ghost_top + local_rows - halo_size) * width,
        halo_bytes, MPI_UNSIGNED_CHAR, down_neighbor, 0,
        extended_buf + (ghost_top + local_rows) * width,
        ghost_bottom * width, MPI_UNSIGNED_CHAR, down_neighbor, 1,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );
    
    // Paso 2: Intercambio hacia arriba
    MPI_Sendrecv(
        extended_buf + ghost_top * width,
        halo_bytes, MPI_UNSIGNED_CHAR, up_neighbor, 1,
        extended_buf,
        ghost_top * width, MPI_UNSIGNED_CHAR, up_neighbor, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );
}


int main(int argc, char *argv[]) {
    
    /* ========================================
     * Inicialización MPI con soporte de hilos
     * MPI_THREAD_FUNNELED: solo el hilo maestro puede llamar a MPI
     * ======================================== */
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    
    if (provided < MPI_THREAD_FUNNELED) {
        fprintf(stderr, "Error: la implementación MPI no soporta MPI_THREAD_FUNNELED.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    /* ========================================
     * Parseo de argumentos (todos los rangos)
     * ======================================== */
    char *input_file = NULL;
    char *output_file = "output.png";
    int filter_size = 3;
    int poster_levels = 9;
    int opt;

    optind = 1;
    while ((opt = getopt(argc, argv, "i:o:f:p:")) != -1) {
        switch (opt) {
            case 'i': input_file = optarg; break;
            case 'o': output_file = optarg; break;
            case 'f': filter_size = atoi(optarg); break;
            case 'p': poster_levels = atoi(optarg); break;
            default:
                if (rank == 0) {
                    fprintf(stderr, "Uso: %s -i <imagen> [-o <salida>] [-f <filtro 3|5>] [-p <niveles 3|9>]\n", argv[0]);
                }
                MPI_Finalize();
                exit(EXIT_FAILURE);
        }
    }

    if (input_file == NULL) {
        if (rank == 0) fprintf(stderr, "Error: es obligatorio especificar una imagen de entrada (-i).\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    if (filter_size != 3 && filter_size != 5) {
        if (rank == 0) fprintf(stderr, "Error: el tamaño del filtro (-f) debe ser 3 o 5.\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    /* ========================================
     * Carga de imagen (solo Rank 0)
     * ======================================== */
    int width = 0, height = 0, channels = 0;
    unsigned char *img_orig = NULL;

    if (rank == 0) {
        printf("Info [Rank 0]: cargando imagen: %s ...\n", input_file);
        img_orig = load_image(input_file, &width, &height, &channels);
        if (img_orig == NULL) {
            fprintf(stderr, "Error: no se pudo cargar la imagen.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Info [Rank 0]: imagen cargada: %d x %d pixeles.\n", width, height);
        printf("Info [Rank 0]: %d procesos MPI x %d hilos OpenMP = %d unidades de cómputo.\n",
               size, omp_get_max_threads(), size * omp_get_max_threads());
    }

    /* ========================================
     * Broadcast de dimensiones
     * ======================================== */
    MPI_Bcast(&width,    1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height,   1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* ========================================
     * Descomposición del dominio: partición 1D por filas
     * ======================================== */
    int base_rows = height / size;
    int remainder = height % size;
    
    int *row_counts     = (int *)malloc(size * sizeof(int));
    int *sendcounts_rgb = (int *)malloc(size * sizeof(int));
    int *displs_rgb     = (int *)malloc(size * sizeof(int));
    
    int offset_acc = 0;
    for (int i = 0; i < size; i++) {
        row_counts[i] = base_rows + (i < remainder ? 1 : 0);
        sendcounts_rgb[i] = row_counts[i] * width * 3;
        displs_rgb[i]     = offset_acc;
        offset_acc       += sendcounts_rgb[i];
    }
    
    int local_rows   = row_counts[rank];
    int local_pixels = local_rows * width;
    
    /* ========================================
     * Asignación de buffers locales
     * ======================================== */
    unsigned char *local_rgb   = (unsigned char *)malloc(local_pixels * 3);
    unsigned char *local_gray  = (unsigned char *)malloc(local_pixels);
    unsigned char *local_edges = (unsigned char *)malloc(local_pixels);
    unsigned char *local_final = (unsigned char *)malloc(local_pixels * 3);
    
    if (!local_rgb || !local_gray || !local_edges || !local_final) {
        fprintf(stderr, "Error [Rank %d]: memoria insuficiente.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* ========================================
     * Distribución de la imagen RGB (Scatterv)
     * ======================================== */
    struct timeval start, end;
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) gettimeofday(&start, NULL);
    
    MPI_Scatterv(img_orig, sendcounts_rgb, displs_rgb, MPI_UNSIGNED_CHAR,
                 local_rgb, local_pixels * 3, MPI_UNSIGNED_CHAR,
                 0, MPI_COMM_WORLD);

    /* ========================================
     * Paso 1: Conversión a escala de grises (OpenMP, sin halos)
     * Las funciones de filters_parallel.c ya contienen pragmas OpenMP
     * ======================================== */
    convert_to_grayscale(local_rgb, local_gray, width, local_rows);

    /* ========================================
     * Paso 2: Borroneado con halo exchange
     * Comunicación MPI solo desde el hilo maestro (FUNNELED)
     * Luego los hilos OpenMP procesan localmente
     * ======================================== */
    int blur_offset = filter_size / 2;
    
    int blur_ghost_top    = (rank == 0)        ? 0 : blur_offset;
    int blur_ghost_bottom = (rank == size - 1) ? 0 : blur_offset;
    int blur_ext_h = blur_ghost_top + local_rows + blur_ghost_bottom;
    
    unsigned char *gray_ext = (unsigned char *)calloc(blur_ext_h * width, 1);
    unsigned char *blur_ext = (unsigned char *)calloc(blur_ext_h * width, 1);
    
    memcpy(gray_ext + blur_ghost_top * width, local_gray, local_pixels);
    
    // Halo exchange (solo hilo maestro, fuera de región paralela)
    halo_exchange(gray_ext, local_rows, width,
                  blur_offset, blur_ghost_top, blur_ghost_bottom,
                  rank, size);
    
    // Borroneado con OpenMP (dentro de apply_blur de filters_parallel.c)
    apply_blur(gray_ext, blur_ext, width, blur_ext_h, filter_size);
    
    unsigned char *local_blur = (unsigned char *)malloc(local_pixels);
    memcpy(local_blur, blur_ext + blur_ghost_top * width, local_pixels);

    /* ========================================
     * Paso 3: Detección de bordes (Sobel) con halo exchange
     * ======================================== */
    int sobel_offset = 1;
    
    int sobel_ghost_top    = (rank == 0)        ? 0 : sobel_offset;
    int sobel_ghost_bottom = (rank == size - 1) ? 0 : sobel_offset;
    int sobel_ext_h = sobel_ghost_top + local_rows + sobel_ghost_bottom;
    
    unsigned char *blur_ext_s = (unsigned char *)calloc(sobel_ext_h * width, 1);
    unsigned char *edges_ext  = (unsigned char *)calloc(sobel_ext_h * width, 1);
    
    memcpy(blur_ext_s + sobel_ghost_top * width, local_blur, local_pixels);
    
    // Halo exchange (solo hilo maestro)
    halo_exchange(blur_ext_s, local_rows, width,
                  sobel_offset, sobel_ghost_top, sobel_ghost_bottom,
                  rank, size);
    
    int sobel_threshold = 70;
    // Sobel con OpenMP (dentro de apply_sobel de filters_parallel.c)
    apply_sobel(blur_ext_s, edges_ext, width, sobel_ext_h, sobel_threshold);
    
    memcpy(local_edges, edges_ext + sobel_ghost_top * width, local_pixels);

    /* ========================================
     * Paso 4: Posterizado con OpenMP (sin halos)
     * ======================================== */
    unsigned char lut[256];
    generate_lut(lut, poster_levels);
    // apply_posterize con OpenMP (dentro de posterize_parallel.c)
    apply_posterize(local_rgb, local_final, width, local_rows, lut);

    /* ========================================
     * Paso 5: Fusión de bordes y colores (OpenMP)
     * ======================================== */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < local_pixels; i++) {
        if (local_edges[i] == 0) {
            int idx_rgb = i * 3;
            local_final[idx_rgb]     = 0;
            local_final[idx_rgb + 1] = 0;
            local_final[idx_rgb + 2] = 0;
        }
    }

    /* ========================================
     * Recolección del resultado en Rank 0 (Gatherv)
     * ======================================== */
    unsigned char *img_final = NULL;
    if (rank == 0) {
        img_final = (unsigned char *)malloc(width * height * 3);
    }
    
    MPI_Gatherv(local_final, local_pixels * 3, MPI_UNSIGNED_CHAR,
                img_final, sendcounts_rgb, displs_rgb, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    /* ========================================
     * Finalización
     * ======================================== */
    if (rank == 0) {
        gettimeofday(&end, NULL);
        
        double time_taken = (end.tv_sec - start.tv_sec) * 1000.0;
        time_taken += (end.tv_usec - start.tv_usec) / 1000.0;
        
        printf("\n========================================\n");
        printf(" Tiempo de procesamiento: %.2f ms\n", time_taken);
        printf(" Procesos MPI: %d\n", size);
        printf(" Hilos OpenMP por proceso: %d\n", omp_get_max_threads());
        printf("========================================\n\n");
        
        save_image(output_file, width, height, 3, img_final);
        printf("Info [Rank 0]: se ha guardado la imagen final en: %s\n", output_file);
        
        free(img_final);
        free_image(img_orig);
    }

    /* ========================================
     * Limpieza de memoria
     * ======================================== */
    free(local_rgb);
    free(local_gray);
    free(local_blur);
    free(local_edges);
    free(local_final);
    free(gray_ext);
    free(blur_ext);
    free(blur_ext_s);
    free(edges_ext);
    free(row_counts);
    free(sendcounts_rgb);
    free(displs_rgb);
    
    MPI_Finalize();
    return 0;
}
