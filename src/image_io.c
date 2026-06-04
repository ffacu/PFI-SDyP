#include "image_io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

unsigned char* load_image(const char* filename, int* width, int* height, int* channels) {
    // El 3 al final fuerza a la libreria a devolvernos estrictamente RGB,
    // procesar 3 canales es más rapido que procesar 4 (RGBA)
    return stbi_load(filename, width, height, channels, 3);
}

void free_image(unsigned char* data) {
    stbi_image_free(data);
}

void save_image(const char* filename, int width, int height, int channels, unsigned char* data) {
    // stride_in_bytes es el ancho de la imagen multiplicado por los canales
    stbi_write_png(filename, width, height, channels, data, width * channels);
}