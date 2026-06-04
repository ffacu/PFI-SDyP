#ifndef FILTERS_H
#define FILTERS_H

// Convierte imagen RGB (3 canales) a grises (1 canal)
void convert_to_grayscale(unsigned char *img_in, unsigned char *img_out, int width, int height);

// Aplica borroneado (blur) sobre una imagen en escala de grises
void apply_blur(unsigned char *img_in, unsigned char *img_out, int width, int height, int filter_size);

// Aplica el filtro de Sobel para deteccion de bordes y un umbralado
void apply_sobel(unsigned char *img_in, unsigned char *img_out, int width, int height, int threshold);

#endif