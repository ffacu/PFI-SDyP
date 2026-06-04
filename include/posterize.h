#ifndef POSTERIZE_H
#define POSTERIZE_H

// Precalcula la lookup table para la cantidad de niveles deseada
void generate_lut(unsigned char lut[256], int levels);

// Aplica el posterizado a la imagen a color usando la LUT
void apply_posterize(unsigned char *img_in, unsigned char *img_out, int width, int height, unsigned char lut[256]);

#endif