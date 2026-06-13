#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>
#include <sys/time.h>
#include "image_io.h"
#include "filters.h"
#include "posterize.h"


/*
 * Intercambio de halos (ghost rows) entre procesos MPI vecinos.
 * 
 * Cada proceso envía sus filas frontera reales a los vecinos y recibe
 * las filas frontera de los vecinos en sus zonas fantasma (ghost zones).
 * Se usa MPI_Sendrecv para evitar deadlocks.
 * MPI_PROC_NULL se utiliza para los bordes del dominio (primer y último rango).
 *
 * Parámetros:
 *   extended_buf : Buffer extendido (ghost_top + local_rows + ghost_bottom) * width
 *   local_rows   : Cantidad de filas reales (sin halos)
 *   width        : Ancho de la imagen
 *   halo_size    : Cantidad de filas de halo (radio del filtro)
 *   ghost_top    : Cantidad de filas fantasma en la parte superior (0 para rank 0)
 *   ghost_bottom : Cantidad de filas fantasma en la parte inferior (0 para el último rank)
 *   rank         : Rango del proceso actual
 *   size         : Cantidad total de procesos
 */
void halo_exchange(unsigned char *extended_buf, int local_rows, int width,
                   int halo_size, int ghost_top, int ghost_bottom,
                   int rank, int size) {
    
    int up_neighbor   = (rank > 0)        ? rank - 1 : MPI_PROC_NULL;
    int down_neighbor = (rank < size - 1) ? rank + 1 : MPI_PROC_NULL;
    
    int halo_bytes = halo_size * width;
    
    /*
     * Paso 1: Intercambio hacia abajo
     * - Enviamos nuestras últimas halo_size filas reales al vecino de abajo
     * - Recibimos del vecino de abajo sus primeras halo_size filas reales
     *   en nuestra zona fantasma inferior
     */
    MPI_Sendrecv(
        /* enviar */ extended_buf + (ghost_top + local_rows - halo_size) * width,
        halo_bytes, MPI_UNSIGNED_CHAR, down_neighbor, 0,
        /* recibir */ extended_buf + (ghost_top + local_rows) * width,
        ghost_bottom * width, MPI_UNSIGNED_CHAR, down_neighbor, 1,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );
    
    /*
     * Paso 2: Intercambio hacia arriba
     * - Enviamos nuestras primeras halo_size filas reales al vecino de arriba
     * - Recibimos del vecino de arriba sus últimas halo_size filas reales
     *   en nuestra zona fantasma superior
     */
    MPI_Sendrecv(
        /* enviar */ extended_buf + ghost_top * width,
        halo_bytes, MPI_UNSIGNED_CHAR, up_neighbor, 1,
        /* recibir */ extended_buf,
        ghost_top * width, MPI_UNSIGNED_CHAR, up_neighbor, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );
}


int main(int argc, char *argv[]) {
    
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    char *input_file = NULL;
    char *output_file = "output.png";
    int filter_size = 3;
    int poster_levels = 9;
    int opt;

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

    if (poster_levels != 3 && poster_levels != 9) {
        fprintf(stderr, "Error: el nivel de posterizado (-p) debe ser 3 o 9.\n");
        exit(EXIT_FAILURE);
    }

    // Carga de imagen (solo Rank 0)
    int width = 0, height = 0, channels = 0;
    unsigned char *img_orig = NULL;

    if (rank == 0) {
        printf("Info [Rank 0]: cargando imagen: %s ...\n", input_file);
        img_orig = load_image(input_file, &width, &height, &channels);
        if (img_orig == NULL) {
            fprintf(stderr, "Error: no se pudo cargar la imagen.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Info [Rank 0]: imagen cargada: %d x %d pixeles, %d procesos MPI.\n", width, height, size);
    }

    // Broadcast de dimensiones a todos los rangos
    MPI_Bcast(&width,    1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height,   1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Descomposición del dominio: partición 1D por filas
    int base_rows = height / size;
    int remainder = height % size;
    
    // Arreglos de conteos y desplazamientos para Scatterv/Gatherv
    int *row_counts     = (int *)malloc(size * sizeof(int));
    int *sendcounts_rgb = (int *)malloc(size * sizeof(int));
    int *displs_rgb     = (int *)malloc(size * sizeof(int));
    int *sendcounts_1ch = (int *)malloc(size * sizeof(int));
    int *displs_1ch     = (int *)malloc(size * sizeof(int));
    
    int offset_rgb = 0;
    int offset_1ch = 0;
    for (int i = 0; i < size; i++) {
        row_counts[i] = base_rows + (i < remainder ? 1 : 0);
        
        sendcounts_rgb[i] = row_counts[i] * width * 3;
        displs_rgb[i]     = offset_rgb;
        offset_rgb       += sendcounts_rgb[i];
        
        sendcounts_1ch[i] = row_counts[i] * width;
        displs_1ch[i]     = offset_1ch;
        offset_1ch       += sendcounts_1ch[i];
    }
    
    int local_rows  = row_counts[rank];
    int local_pixels = local_rows * width;
    
    // Asignación de buffers locales
    unsigned char *local_rgb   = (unsigned char *)malloc(local_pixels * 3);
    unsigned char *local_gray  = (unsigned char *)malloc(local_pixels);
    unsigned char *local_edges = (unsigned char *)malloc(local_pixels);
    unsigned char *local_final = (unsigned char *)malloc(local_pixels * 3);
    
    if (!local_rgb || !local_gray || !local_edges || !local_final) {
        fprintf(stderr, "Error [Rank %d]: memoria insuficiente.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        printf("Info: iniciando toma de tiempos.\n");
    }

    // Distribución de la imagen RGB (Scatterv)
    struct timeval start, end;
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) gettimeofday(&start, NULL);
    
    MPI_Scatterv(img_orig, sendcounts_rgb, displs_rgb, MPI_UNSIGNED_CHAR,
                 local_rgb, local_pixels * 3, MPI_UNSIGNED_CHAR,
                 0, MPI_COMM_WORLD);

    // Conversión a escala de grises (sin halos)
    convert_to_grayscale(local_rgb, local_gray, width, local_rows);

    // Borroneado con halo exchange
    int blur_offset = filter_size / 2;
    
    // Calcular halos reales para este rango
    // Rank 0 no tiene halo superior, y último rank no tiene halo inferior
    // Esto preserva el comportamiento de la versión secuencial donde
    // las filas frontera de la imagen quedan en negro
    int blur_ghost_top    = (rank == 0)        ? 0 : blur_offset;
    int blur_ghost_bottom = (rank == size - 1) ? 0 : blur_offset;
    int blur_ext_h = blur_ghost_top + local_rows + blur_ghost_bottom;
    
    unsigned char *gray_ext = (unsigned char *)calloc(blur_ext_h * width, 1);
    unsigned char *blur_ext = (unsigned char *)calloc(blur_ext_h * width, 1);
    
    // Copiar datos grises locales al centro del buffer extendido
    memcpy(gray_ext + blur_ghost_top * width, local_gray, local_pixels);
    
    // Intercambio de halos para borroneado
    halo_exchange(gray_ext, local_rows, width,
                  blur_offset, blur_ghost_top, blur_ghost_bottom,
                  rank, size);
    
    // Aplicar borroneado sobre el buffer extendido
    apply_blur(gray_ext, blur_ext, width, blur_ext_h, filter_size);
    
    // Extraer las filas reales del resultado del borroneado
    unsigned char *local_blur = (unsigned char *)malloc(local_pixels);
    memcpy(local_blur, blur_ext + blur_ghost_top * width, local_pixels);

    int sobel_offset = 1;  // Sobel siempre usa kernel 3x3
    
    int sobel_ghost_top    = (rank == 0)        ? 0 : sobel_offset;
    int sobel_ghost_bottom = (rank == size - 1) ? 0 : sobel_offset;
    int sobel_ext_h = sobel_ghost_top + local_rows + sobel_ghost_bottom;
    
    unsigned char *blur_ext_s = (unsigned char *)calloc(sobel_ext_h * width, 1);
    unsigned char *edges_ext  = (unsigned char *)calloc(sobel_ext_h * width, 1);
    
    // Copiar datos borroneados locales al centro del buffer extendido
    memcpy(blur_ext_s + sobel_ghost_top * width, local_blur, local_pixels);
    
    // Intercambio de halos para Sobel
    halo_exchange(blur_ext_s, local_rows, width,
                  sobel_offset, sobel_ghost_top, sobel_ghost_bottom,
                  rank, size);
    
    int sobel_threshold = 70;
    apply_sobel(blur_ext_s, edges_ext, width, sobel_ext_h, sobel_threshold);
    
    // Extraer las filas reales del resultado de bordes
    memcpy(local_edges, edges_ext + sobel_ghost_top * width, local_pixels);

    // Posterizado (sin halos, aplicado a RGB original)
    unsigned char lut[256];
    generate_lut(lut, poster_levels);
    apply_posterize(local_rgb, local_final, width, local_rows, lut);

    // Fusión de bordes y colores posterizados
    for (int i = 0; i < local_pixels; i++) {
        if (local_edges[i] == 0) {
            int idx_rgb = i * 3;
            local_final[idx_rgb]     = 0;
            local_final[idx_rgb + 1] = 0;
            local_final[idx_rgb + 2] = 0;
        }
    }

    // Recolección del resultado en Rank 0 (Gatherv)
    unsigned char *img_final = NULL;
    if (rank == 0) {
        img_final = (unsigned char *)malloc(width * height * 3);
    }
    
    MPI_Gatherv(local_final, local_pixels * 3, MPI_UNSIGNED_CHAR,
                img_final, sendcounts_rgb, displs_rgb, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    // Guardar resultado y reportar tiempos
    if (rank == 0) {
        gettimeofday(&end, NULL);
        printf("Info: fin de toma de tiempos.\n");
        
        double time_taken = (end.tv_sec - start.tv_sec) * 1000.0;
        time_taken += (end.tv_usec - start.tv_usec) / 1000.0;
        
        printf("\n========================================\n");
        printf(" Tiempo de procesamiento: %.2f ms\n", time_taken);
        printf(" Procesos MPI: %d\n", size);
        printf("========================================\n\n");
        
        save_image(output_file, width, height, 3, img_final);
        printf("Info [Rank 0]: se ha guardado la imagen final en: %s\n", output_file);
        
        free(img_final);
        free_image(img_orig);
    }

    // Limpieza de memoria
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
    free(sendcounts_1ch);
    free(displs_1ch);
    
    MPI_Finalize();
    return 0;
}
