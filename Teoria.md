
# Información importante sobre arquitectura y funcionamiento
---

### 1. Implicancia de los parámetros de ejecución (`-f` y `-p`)

Estos parámetros controlan la intensidad de los dos efectos principales que transforman una foto en un Cartoon.

* **Parámetro `-f` (Tamaño del Filtro de Borroneado):**
* **¿Qué hace visualmente?** Define qué tan suavizada quedará la imagen antes de buscarle los bordes.
* **Implicancia en 3 (3x3):** El programa mira un radio muy pequeño (el píxel central y sus 8 vecinos inmediatos). Suaviza poco. Si la foto tiene mucho ruido o texturas, el programa creerá que esos pequeños detalles son bordes y el cartoon quedará "sucio" con muchas líneas negras.
* **Implicancia en 5 (5x5):** El programa mira un bloque de 25 píxeles. Suaviza mucho más. Elimina las texturas finas, por lo que los únicos bordes que sobrevivirán serán los contornos fuertes (la silueta de una persona, el borde de un edificio).
* **Implicancia en Rendimiento:** Un filtro 3x3 requiere **9 lecturas/multiplicaciones** por cada píxel. Un filtro 5x5 requiere **25 operaciones** por píxel. Al usar 5x5, el paso de borroneado tardará casi 3 veces más. Esto es clave para cuando se mida el *Speedup* en la versión paralela.


* **Parámetro `-p` (Niveles de Posterizado):**
* **¿Qué hace visualmente?** Reduce la cantidad de colores disponibles para cada canal (Rojo, Verde, Azul).
* **Implicancia en 3 niveles:** Agrupa todos los colores posibles (0 a 255) en solo 3 "baldes" (ej. oscuro, medio, claro). El resultado es una imagen de colores súper planos, muy estilo cómic retro.
* **Implicancia en 9 niveles:** Permite 9 tonalidades por canal. El resultado conserva más detalles de sombras e iluminación.
* **Implicancia en Rendimiento:** Gracias a que implementamos una **Lookup Table (LUT)**, el tiempo de ejecución para `-p 3` o `-p 9` es **exactamente el mismo**. El algoritmo no calcula el color matemático píxel por píxel; simplemente busca el resultado en un arreglo de 256 posiciones precalculado. Esto es una optimización fundamental ($O(1)$ en complejidad de tiempo).



---

### 2. Los algoritmos utilizados y por qué se eligieron

Para lograr el efecto cartoon, necesitamos dos cosas: colores planos (posterizado) y líneas negras marcando los contornos (Sobel).

#### A. Conversión a escala de grises (Luminancia)

* **¿Cómo funciona?** Toma los canales RGB y los multiplica por constantes ($0.299R + 0.587G + 0.114B$).
* **¿Por qué se usa?** Una imagen RGB tiene 3 valores por píxel (Rojo, Verde, Azul). Para detectar bordes, el color es irrelevante; lo que importa es la intensidad de la luz. Transformar 3 canales a 1 canal reduce el volumen de datos a un tercio. No calculamos un promedio simple (R+G+B)/3 porque el ojo humano no percibe los colores con la misma intensidad. Somos mucho más sensibles al color verde. Por eso, utilizamos la fórmula estándar de luminancia (ITU-R BT.601).

#### B. Borroneado (Box Blur)

* **¿Cómo funciona?** Utiliza la **convolución**, que es una operación matemática. El algoritmo se para sobre un píxel, suma el valor de todos los píxeles vecinos dentro de su matriz (3x3 o 5x5) y calcula el promedio. Ese promedio reemplaza al píxel original.
* **¿Por qué se usa?** Funciona como un filtro pasa bajos. Elimina el ruido de alta frecuencia (granulado de la foto). Es un paso obligatorio antes de detectar bordes; de lo contrario, el detector encontraría "falsos bordes" en cada pequeño defecto de la foto.

#### C. Filtro de Sobel (Detección de Bordes)

* **¿Cómo funciona?** Es una aproximación del gradiente (la derivada) de la imagen. Usa dos matrices (Kernels), una que penaliza los cambios horizontales ($G_x$) y otra los verticales ($G_y$).
* **La optimización matemática:** La fórmula real de la magnitud del gradiente es $M = \sqrt{G_x^2 + G_y^2}$. Sin embargo, calcular raíces cuadradas a nivel CPU es *extremadamente lento*. En nuestro algoritmo usamos la aproximación absoluta: **$M = |G_x| + |G_y|$**. Visualmente el resultado es idéntico, pero el procesador lo calcula muchísimo más rápido.
* **Umbralado (Thresholding):** La magnitud nos da un número. Le decimos al programa: "Si este número supera el umbral (ej. 70), considéralo un borde y píntalo de negro absoluto (0). Si no, píntalo de blanco transparente (255)".

#### D. Posterización (mediante Lookup Table - LUT)

* **¿Cómo funciona?** Divide el rango continuo de colores (0 a 255) en escalones definidos por el usuario.
* **¿Por qué usamos LUT?** Porque en una foto de 5000x5000, hay 25 millones de píxeles (75 millones de canales RGB). Si aplicáramos divisiones y redondeos matemáticos en un bucle 75 millones de veces, arruinaríamos el rendimiento. La LUT calcula los 256 resultados posibles *una sola vez* al arrancar el programa, y luego solo hace lecturas en memoria.

---

### 3. El Paso a Paso del Pipeline (El "Por Qué" de este orden)

El procesamiento de imágenes es un pipeline. El orden no es arbitrario; alterar un paso destruye el resultado.

1. **Lectura y Asignación Contigua (I/O & Memoria):** Se lee la imagen y se aloja en un arreglo unidimensional continuo en memoria (`malloc`). **Por qué:** Maximiza la "Localidad Espacial". El procesador carga bloques enteros de píxeles en su memoria caché, evitando tener que ir a buscar a la memoria RAM constantemente.
2. **Generación de la LUT:** Se preparan las tablas de color antes de procesar la imagen.
3. **Color $\rightarrow$ Grises:** Generamos una copia sin color para el análisis matemático.
4. **Borroneado (sobre imagen gris):** Suavizamos la imagen gris. Si detectamos bordes antes de borrar, capturaremos basura.
5. **Detección Sobel (sobre imagen gris borrosa):** Extraemos la "máscara" de bordes en blanco y negro.
6. **Posterizado (sobre imagen original a color):** Tomamos la foto original de alta resolución y aplanamos sus colores usando la LUT. **¿Por qué original y no la gris borrosa?** si posterizáramos la imagen borrosa, los colores quedarían manchados y perderían los detalles. La detección de bordes va por un "camino" y el color por otro.
7. **Fusión:** Juntamos los dos caminos. Tomamos la imagen a color posterizada (Paso 6) y le "estampamos" encima los píxeles negros que detectó Sobel (Paso 5).