#include "posterize.h"


// Generacion de Lookup Table
void generate_lut(unsigned char lut[256], int levels) {
    if (levels < 2) levels = 2; // Evitamos division por cero

    // Tamaño de cada balde
    int step = 256 / levels;

    for (int i = 0; i < 256; i++) {
        int bucket = i / step;
        
        // Prevencion de desbordamiento por redondeo en el ultimo nivel
        if (bucket >= levels) {
            bucket = levels - 1;
        }

        // Asignamos el valor central de ese balde
        lut[i] = (unsigned char)(bucket * (255.0 / (levels - 1)));
    }
}


// Aplicar posterizado
void apply_posterize(unsigned char *img_in, unsigned char *img_out, int width, int height, unsigned char lut[256]) {
    int total_elements = width * height * 3; // 3 canales (RGB)

    // Recorremos la memoria plana y reemplazamos cada canal de color (R, G, B)
    // consultando su nuevo valor en el arreglo de la LUT.
    // Acceso a memoria O(1)
    for (int i = 0; i < total_elements; i++) {
        img_out[i] = lut[img_in[i]];
    }
}