#include "filters.h"
#include <stdlib.h>


// Escala de grises (version paralela)
void convert_to_grayscale(unsigned char *img_in, unsigned char *img_out, int width, int height) {
    int num_pixels = width * height;
    
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < num_pixels; i++) {
        int idx_rgb = i * 3;
        unsigned char r = img_in[idx_rgb];
        unsigned char g = img_in[idx_rgb + 1];
        unsigned char b = img_in[idx_rgb + 2];
        
        // Formula estandar de luminancia
        img_out[i] = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
    }
}


// Borroneado (Box blur) (version paralela)
void apply_blur(unsigned char *img_in, unsigned char *img_out, int width, int height, int filter_size) {
    int offset = filter_size / 2;                 // Para 3x3 es 1, para 5x5 es 2
    int num_elements = filter_size * filter_size; // 9 o 25

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; i++) {
        img_out[i] = 0;
    }

    // Iteramos ignorando los bordes perimetrales (offset)
    #pragma omp parallel for schedule(static)
    for (int y = offset; y < height - offset; y++) {
        for (int x = offset; x < width - offset; x++) {
            
            int sum = 0;
            
            // Aplicamos la matriz de convolucion
            for (int ky = -offset; ky <= offset; ky++) {
                for (int kx = -offset; kx <= offset; kx++) {
                    int pixel_y = y + ky;
                    int pixel_x = x + kx;
                    int idx = pixel_y * width + pixel_x;
                    
                    sum += img_in[idx];
                }
            }
            
            // Calculamos el promedio y lo asignamos al pixel central
            int final_idx = y * width + x;
            img_out[final_idx] = (unsigned char)(sum / num_elements);
        }
    }
}


// Deteccion de bordes (Sobel + umbralado) (versión paralela)
void apply_sobel(unsigned char *img_in, unsigned char *img_out, int width, int height, int threshold) {
    
    // Kernels de Sobel estandar
    int Gx[3][3] = {
        {-1,  0,  1},
        {-2,  0,  2},
        {-1,  0,  1}
    };
    
    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < width * height; i++) {
        img_out[i] = 0; 
    }

    // Sobel siempre usa un radio de 1 (matriz 3x3)
    int offset = 1;

    #pragma omp parallel for schedule(static)
    for (int y = offset; y < height - offset; y++) {
        for (int x = offset; x < width - offset; x++) {
            
            int sumX = 0;
            int sumY = 0;
            
            // Aplicamos ambas matrices de convolucion
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel_y = y + ky;
                    int pixel_x = x + kx;
                    int idx = pixel_y * width + pixel_x;
                    
                    int pixel_val = img_in[idx];
                    
                    sumX += pixel_val * Gx[ky + 1][kx + 1];
                    sumY += pixel_val * Gy[ky + 1][kx + 1];
                }
            }
            
            // Magnitud aproximada (mucho mas rapido que usar sqrt y potencias)
            int magnitude = abs(sumX) + abs(sumY);
            
            int final_idx = y * width + x;
            
            // Operacion de umbralado (thresholding)
            if (magnitude > threshold) {
                img_out[final_idx] = 0;   // Borde (negro)
            } else {
                img_out[final_idx] = 255; // Fondo (blanco)
            }
        }
    }
}
