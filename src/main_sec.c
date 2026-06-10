#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include "image_io.h"
#include "filters.h"
#include "posterize.h"
#include <sys/time.h>


int main(int argc, char *argv[]) {
    
    char *input_file = NULL;
    char *output_file = "output.png";
    int filter_size = 3;
    int poster_levels = 9;
    int opt;

    /* 
      Parseo de argumentos: getopt lee argv buscando flags definidos en la cadena "i:o:f:p:"
      Los dos puntos (:) indican que el flag espera un valor (ej: -i foto.png)
    */
    while ((opt = getopt(argc, argv, "i:o:f:p:")) != -1) {
        switch (opt) {
            case 'i': input_file = optarg; break; 
            case 'o': output_file = optarg; break;
            case 'f': filter_size = atoi(optarg); break; 
            case 'p': poster_levels = atoi(optarg); break;
            default:
                fprintf(stderr, "Uso: %s -i <imagen> [-o <salida>] [-f <filtro 3|5>] [-p <niveles 3|9>]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (input_file == NULL) {
        fprintf(stderr, "Error: es obligatorio especificar una imagen de entrada (-i).\n");
        exit(EXIT_FAILURE);
    }
    if (filter_size != 3 && filter_size != 5) {
        fprintf(stderr, "Error: el tamaño del filtro (-f) debe ser 3 o 5.\n");
        exit(EXIT_FAILURE);
    }

    if (poster_levels != 3 && filter_size != 9) {
        fprintf(stderr, "Error: el nivel de posterizado (-p) debe ser 3 o 9.\n");
        exit(EXIT_FAILURE);
    }

    int width, height, channels;
    printf("Info: cargando imagen: %s ...\n", input_file);
    
    unsigned char *img_orig = load_image(input_file, &width, &height, &channels);
    if (img_orig == NULL) {
        fprintf(stderr, "Error: no se pudo cargar la imagen. Verifica la ruta o el formato.\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Info: imagen cargada exitosamente:  %d x %d pixeles.\n", width, height);

    int num_pixels = width * height;
    
    // Usamos arreglos unidimensionales continuos.
    // Las matrices intermedias (grises, borroneado, bordes) solo necesitan 1 canal (1 byte por pixel)
    // Evitamos multiplicar por 3, ahorrando RAM y acelerando la cache del procesador
    
    unsigned char *img_gray  = (unsigned char *)malloc(num_pixels);        // Escala de grises
    unsigned char *img_blur  = (unsigned char *)malloc(num_pixels);        // Resultado del borroneado
    unsigned char *img_edges = (unsigned char *)malloc(num_pixels);        // Resultado deteccion de bordes
    unsigned char *img_final = (unsigned char *)malloc(num_pixels * 3);    // Imagen final a color (RGB)

    if (!img_gray || !img_blur || !img_edges || !img_final) {
        fprintf(stderr, "Error: memoria insuficiente para alojar las matrices temporales.\n");
        exit(EXIT_FAILURE);
    }

    printf("Info: memoria reservada correctamente.\n");
    printf("Info: iniciando toma de tiempos.\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);

    printf("Info: convirtiendo a escala de grises ...\n");
    convert_to_grayscale(img_orig, img_gray, width, height);

    printf("Info: aplicando borroneado (filtro %dx%d) ...\n", filter_size, filter_size);
    apply_blur(img_gray, img_blur, width, height, filter_size);

    // Definimos un umbral para detectar los bordes (valores entre 50 y 150 andan ok)
    int sobel_threshold = 70; 
    printf("Info: detectando bordes (filtro de Sobel) ...\n");
    apply_sobel(img_blur, img_edges, width, height, sobel_threshold);

    printf("Info: aplicando posterizado (%d niveles) ...\n", poster_levels);
    
    // Generamos la LUT
    unsigned char lut[256];
    generate_lut(lut, poster_levels);
    
    // Aplicamos la LUT a la imagen original (que todavia tiene su color RGB intacto)
    apply_posterize(img_orig, img_final, width, height, lut);

    printf("Info: fusionando bordes y colores para crear el Cartoon ...\n");
    
    // Recorremos todos los pixeles uno por uno
    for (int i = 0; i < width * height; i++) {
        // img_edges tiene 1 solo canal. Si es 0, es un borde detectado por Sobel
        if (img_edges[i] == 0) {
            // img_final tiene 3 canales. Calculamos el indice base para este pixel
            int idx_rgb = i * 3;
            
            // Pintamos el pixel de negro (R=0, G=0, B=0)
            img_final[idx_rgb] = 0;
            img_final[idx_rgb + 1] = 0;
            img_final[idx_rgb + 2] = 0;
        }
        // Si no es 0, no hacemos nada porque img_final ya tiene el color posterizado ahi
    }

    gettimeofday(&end, NULL); 
    printf("Info: fin de toma de tiempos.\n");

    // Calculo del tiempo transcurrido en milisegundos
    double time_taken = (end.tv_sec - start.tv_sec) * 1000.0;      // Seg a ms
    time_taken += (end.tv_usec - start.tv_usec) / 1000.0;          // Microseg a ms

    printf("\n========================================\n");
    printf(" Tiempo de procesamiento: %.2f ms\n", time_taken);
    printf("========================================\n\n");

    save_image(output_file, width, height, 3, img_final);
    printf("Info: se ha guardado la imagen final en: %s\n", output_file);

    free(img_gray);
    free(img_blur);
    free(img_edges);
    free(img_final);
    free_image(img_orig);

    return 0;
}