#ifndef IMAGE_IO_H
#define IMAGE_IO_H

// API publica
unsigned char* load_image(const char* filename, int* width, int* height, int* channels);
void free_image(unsigned char* data);
void save_image(const char* filename, int width, int height, int channels, unsigned char* data);

#endif