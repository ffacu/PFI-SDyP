# Procesamiento de imágenes realizando computación en cluster
---

## Compilar

Para compilar solo el programa secuencial
`make secuencial`

Para compilar solo el programa de memoria compartida
`make omp`

Para compilar solo el programa de memoria distribuida
`make mpi`

Para compilar solo el programa híbrido
`make hibrido`

Para compilar todos los archivos
`make`

---

## Parámetros de ejecución

Se debe poner la dirección donde se encuentra el binario compilado, y luego una serie de parámetros para facilitar las distintas combinaciones de prueba. Por ejemplo:

`./bin/secuencial -i mi_foto.png -o salida.png -f 5 -p 3`

Con esto le estamos pasando argumentos a la función getopt():
- -i mi_foto.bmp: es el archivo de Input
- -o salida.png: (Opcional) es el archivo de Output. Si no le pasás este dato, el código usa "output.png" por defecto.
- -f 5: Define el tamaño del filtro de convolución. Le indica al programa que debe usar una matriz de 5x5 para el borroneado y los bordes. El trabajo pide soportar 3 (3x3) y 5 (5x5).
- -p 3: Define los niveles del posterizado. El práctico pide probar con 3 y 9