# Información importante sobre arquitectura y funcionamiento

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
<!--  -->

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
<!--  -->

### 3. El Paso a Paso del Pipeline (El "Por Qué" de este orden)

El procesamiento de imágenes es un pipeline. El orden no es arbitrario; alterar un paso destruye el resultado.

1. **Lectura y Asignación Contigua (I/O & Memoria):** Se lee la imagen y se aloja en un arreglo unidimensional continuo en memoria (`malloc`). **Por qué:** Maximiza la "Localidad Espacial". El procesador carga bloques enteros de píxeles en su memoria caché, evitando tener que ir a buscar a la memoria RAM constantemente.
2. **Generación de la LUT:** Se preparan las tablas de color antes de procesar la imagen.
3. **Color $\rightarrow$ Grises:** Generamos una copia sin color para el análisis matemático.
4. **Borroneado (sobre imagen gris):** Suavizamos la imagen gris. Si detectamos bordes antes de borrar, capturaremos basura.
5. **Detección Sobel (sobre imagen gris borrosa):** Extraemos la "máscara" de bordes en blanco y negro.
6. **Posterizado (sobre imagen original a color):** Tomamos la foto original de alta resolución y aplanamos sus colores usando la LUT. **¿Por qué original y no la gris borrosa?** si posterizáramos la imagen borrosa, los colores quedarían manchados y perderían los detalles. La detección de bordes va por un "camino" y el color por otro.
7. **Fusión:** Juntamos los dos caminos. Tomamos la imagen a color posterizada (Paso 6) y le "estampamos" encima los píxeles negros que detectó Sobel (Paso 5).

---
<!--  -->

# Informacion acerca de la paralelizacion

## OpenMP

Para acelerar el pipeline de cartoonización en multiprocesadores simétricos de un solo nodo (SMP), el sistema utiliza OpenMP. OpenMP emplea un modelo de ejecución *fork-join*: el hilo maestro se ejecuta secuencialmente hasta que encuentra una región paralela, momento en el cual genera un equipo de hilos de trabajo que comparten el espacio de direcciones de memoria global del nodo.


### Paralelismo a Nivel de Bucle y Programación

Evaluar la matemática de la posterización o aplicar un núcleo de convolución a un píxel específico no depende inherentemente del valor recién calculado de ningún otro píxel.  Así, el algoritmo se basa en el paralelismo a nivel de bucle, utilizando la directiva #pragma omp parallel for aplicada al bucle más externo (iterando sobre la altura de la imagen).

OpenMP proporciona tres clases principales de programación:
- **Programación Dinámica (`schedule(dynamic)`):** Las iteraciones se distribuyen en tiempo de ejecución.  Cuando un hilo completa una pequeña porción de trabajo, consulta al programador interno de OpenMP para obtener otra tarea.  Aunque es excelente para cargas de trabajo altamente impredecibles (como el trazado de rayos recursivo), la programación dinámica introduce una inmensa sobrecarga de sincronización del sistema operativo.
- **Programación Guiada (`schedule(guided)`):** Similar a la dinámica, pero los tamaños de los bloques asignados disminuyen exponencialmente con el tiempo, intentando equilibrar la carga mientras se minimiza la sobrecarga en las etapas finales.
- **Programación Estática (`schedule(static)`):**  El espacio total de iteración se divide en grandes bloques contiguos de igual tamaño y se asigna permanentemente a hilos específicos en tiempo de compilación.

Para las operaciones que mapean sobre matrices rectangulares, la complejidad computacional por píxel es completamente determinista y uniforme.  Por lo tanto, la programación estática es la opción óptima, ya que esta elimina las consultas del programador en tiempo de ejecución, proporcionando la sobrecarga de sincronización de hilos más baja posible.  Además, al asignar bloques masivos y contiguos de filas a hilos individuales, la programación estática respeta los algoritmos de prefetching de caché L1/L2 del hardware, mejorando enormemente la localidad espacial y la utilización del ancho de banda de memoria.

### Prevención de False Sharing y Race Conditions

Cuando múltiples hilos escriben datos rápidamente en variables distintas que por casualidad residen en la misma línea de caché física de la CPU, los protocolos de coherencia de caché de hardware invalidan y recargan continuamente la línea de caché en diferentes núcleos.  Este fenómeno, conocido como *False Sharing*, degrada severamente la eficiencia paralela. En la implementación de la convolución, los hilos deben leer de una imagen fuente compartida y escribir en una imagen de destino compartida.

Debido a que la programación estática obliga a que los hilos operen en bloques de memoria completamente dispares y grandes (separados por megabytes de datos), sus punteros de escritura nunca ocupan la misma línea de caché.  En consecuencia, la compartición falsa se elimina por diseño, y los costosos bloqueos de sincronización de software (como `#pragma omp critical`) son innecesarios durante los bucles de procesamiento de píxeles.

---
<!--  -->

## MPI

Mientras que OpenMP satura la capacidad de procesamiento de una sola máquina, escalar la computación a través de múltiples computadoras autónomas requiere MPI, la cual opera en un espacio de memoria distribuido. Acá los procesos (rango) no pueden acceder a las variables de los demás; los datos deben ser empaquetados explícitamente, enrutados a través de la interfaz de red y desempaquetados por el nodo receptor.


### Descomposición de Dominio

El proceso raíz de MPI (Rango 0) utiliza `stb_image` para cargar la imagen completa en su memoria local.  Para paralelizar la carga de trabajo, este enorme arreglo de una dimensión (1D) debe dividirse lógicamente en subdominios y distribuirse a los rangos de los trabajadores. Una descomposición en bloques cartesianos 2D divide la imagen en una cuadrícula de cuadrados, lo que minimiza matemáticamente la relación perímetro-área de los subdominios, reduciendo teóricamente el volumen total de datos de frontera que deben ser comunicados.

Transmitir una columna vertical de una imagen requiere leer direcciones de memoria dispersas y no contiguas.  Aunque MPI proporciona tipos de datos derivados (`MPI_Type_vector` o `MPI_Type_create_subarray`) para manejar lecturas de columnas con paso, la sobrecarga de la CPU necesaria para empaquetar y desempaquetar continuamente estos bytes no contiguos desperdicia cualquier ahorro obtenido de la topología 2D. En consecuencia, la Descomposición por Filas 1D es superior.

Debido a que cada fila se almacena de manera contigua en la memoria, el proceso raíz puede transmitir particiones completas de la imagen utilizando punteros de memoria, logrando el máximo rendimiento de ancho de banda de red con un sobrecosto de empaquetado de software casi nulo.

Dado que las alturas de las imágenes rara vez son perfectamente divisibles por el número de nodos activos del clúster, la operación `MPI_Scatter` (que exige fragmentos exactamente iguales) no nos servira, por lo que deberemos utilizar `MPI_Scatterv` y `MPI_Gatherv`.  Estas funciones permiten que el proceso raíz dicte un entero `sendcounts` único y un índice de memoria de desplazamientos inicial para cada rango individual, gestionando adecuadamente las filas residuales de la división.


### El Mecanismo Halo Hexchange

El obstáculo fundamental en el filtrado espacial distribuido es resolver los cálculos en los límites de los subdominios.  Una operación de vecindario, como una convolución Sobel de 5x5, requiere leer datos de píxeles hasta un radio de dos píxeles alrededor de la coordenada objetivo.  Cuando un proceso MPI intenta calcular el gradiente para la fila más alta de su bloque de imagen asignado, los píxeles vecinos requeridos residen en la memoria física del proceso que gestiona la partición superior. Para resolver esta aislamiento de memoria, la arquitectura implementa zonas fantasma o halos.

Cuando cada proceso asigna su matriz de destino local, asigna filas adicionales y vacías en los límites extremos superior e inferior. El grosor del halo requerido está dictado por el radio del núcleo de convolución: Un núcleo de 3x3 requiere un grosor de halo de 1 fila. Un núcleo de 5x5 requiere un grosor de halo de 2 filas. Antes de ejecutar los bucles de procesamiento, todos los procesos MPI se detienen para llevar a cabo una fase de comunicación "halo hexchange".  Cada proceso transmite sus filas de frontera verdaderas a sus vecinos para poblar sus zonas fantasma, y simultáneamente recibe las filas de frontera verdaderas de sus vecinos para poblar sus propias zonas fantasma.

### Protocolos de Comunicación y Prevención de Interbloqueos

Las implementaciones del intercambio de halo muy seguido "emparejan" las rutinas bloqueantes `MPI_Send` y `MPI_Recv` de manera secuencial.  Si cada proceso intenta enviar simultáneamente sin publicar un búfer de recepción correspondiente, los búferes del protocolo de red se saturan rápidamente, lo que resulta en un bloqueo del sistema.

La solución más robusta y estándar en la industria es utilizar la rutina `MPI_Sendrecv`.  Esta función publica simultáneamente una operación de envío y una operación de recepción, confiando en los threads asíncronos internos de la biblioteca MPI para gestionar el intercambio de datos, eliminando por completo el riesgo de bloqueos cíclicos.

Para una topología unidimensional por filas, el intercambio completo de halo requiere dos pasos simétricos:
1. **Intercambio Ascendente:** El rango $N$ envía sus filas reales más altas al rango $N-1$, y recibe filas fantasma del rango $N-1$ en su búfer fantasma superior.
2. **Intercambio Descendente:** El Rango $N$ envía sus filas reales más bajas al Rango $N+1$, y recibe filas fantasma del Rango $N+1$ en su búfer fantasma inferior. Para preservar la uniformidad del algoritmo sin escribir lógica compleja para los casos extremos de los primeros (Rango 0) y últimos procesos, la arquitectura configura los límites extremos para comunicarse con el destino especializado `MPI_PROC_NULL`.  Llamar a `MPI_Sendrecv` dirigido a `MPI_PROC_NULL` ejecuta una no-operación inmediata y exitosa, permitiendo que el bloque de código estándar funcione sin problemas en todo el clúster.


---
<!--  -->

## OpenMP + MPI

Las supercomputadoras modernas tienen una estructura jerárquica: constan de cientos de nodos físicos conectados por redes de alta velocidad, donde cada nodo alberga multiprocesadores simétricos (SMP) con decenas de núcleos de CPU físicos.

Implementar una aplicación MPI pura en esta arquitectura —instanciando un proceso MPI autónomo por núcleo físico— genera graves ineficiencias. La biblioteca MPI está diseñada para la comunicación en red. Cuando los procesos MPI que residen en la misma placa base física intercambian datos, la biblioteca suele almacenar innecesariamente la memoria en búferes mediante pilas de red virtualizadas o sistemas de sondeo de memoria compartida, lo que degrada artificialmente el ancho de banda de la memoria y consume grandes cantidades de RAM del sistema con búferes de contexto duplicados.

La solución definitiva es el modelo híbrido MPI + OpenMP. Este paradigma exige el lanzamiento de un único proceso MPI por nodo físico (o por zócalo de CPU). MPI se limita estrictamente a gestionar las comunicaciones masivas entre nodos (como `MPI_Scatterv` y los intercambios de halo). Una vez finalizada la sincronización de la red, el proceso MPI invoca OpenMP para generar hilos ligeros en los núcleos de la CPU local. Estos hilos ejecutan los complejos cálculos algorítmicos (posterización y convolución espacial) mediante memoria compartida, sin duplicar matrices ni invocar protocolos de red.

### Seguridad de subprocesos y selección de protocolo

La integración de multithreading con las comunicaciones de red invariablemente introduce riesgos. Si varios subprocesos de OpenMP intentan invocar funciones MPI simultáneamente, el estado interno de la biblioteca MPI se puede corromper, provocando fallos de segmentación.

Para mitigar este riesgo, haremos uso del protocolo `MPI_Init_thread`, que requiere que el desarrollador solicite explícitamente un nivel específico de seguridad de subprocesos. Los niveles disponibles son:

- `MPI_THREAD_SINGLE:` La aplicación no puede ser multihilo.
- `MPI_THREAD_FUNNELED`: La aplicación puede ser multihilo, pero solo el subproceso "maestro" original puede invocar rutinas MPI.
- `MPI_THREAD_SERIALIZED`: Varios subprocesos pueden invocar llamadas MPI, pero la aplicación debe garantizar, mediante bloqueos de software, que estas llamadas nunca se superpongan cronológicamente.
- `MPI_THREAD_MULTIPLE`: Cualquier hilo puede invocar llamadas MPI en cualquier momento, asumiendo la biblioteca MPI la responsabilidad total del bloqueo de mutex interno y la gestión de la cola.


Si bien `MPI_THREAD_MULTIPLE` ofrece la mayor flexibilidad, garantizar la seguridad a este nivel requiere que la biblioteca MPI realice un bloqueo interno intensivo, lo que restringe fundamentalmente el rendimiento de la comunicación y degrada significativamente el rendimiento general del clúster.

Por lo tanto, el patrón de diseño óptimo para la canalización de caricaturización es el estilo "Master-Only" que utiliza `MPI_THREAD_FUNNELED`. Bajo esta arquitectura, la ejecución algorítmica se realiza completamente fuera de las regiones paralelas de OpenMP para coordinar la red. El hilo maestro gestiona de forma autónoma los intercambios de halo `MPI_Sendrecv`, actualizando las "zonas fantasma" locales. Una vez que la memoria localizada es totalmente coherente, el hilo maestro invoca `#pragma omp parallel for`, liberando los hilos de cómputo a través de la matriz sin riesgo de contención de red.

### Mitigación de bottelnecks en NUMA

Un problema de rendimiento significativo en arquitecturas híbridas que se ejecutan en nodos con múltiples sockets (por ejemplo, configuraciones duales AMD EPYC o Intel Xeon) es el efecto de NUMA (Non Uniform Memory Access).

En un sistema NUMA, los módulos de RAM físicos están conectados directamente a sockets de CPU específicos. Si un hilo que se ejecuta en la CPU 0 intenta acceder a una matriz ubicada físicamente en el banco de RAM conectado a la CPU 1, los datos deben atravesar la interconexión entre sockets (como Intel QPI o AMD Infinity Fabric), lo que genera importantes penalizaciones de latencia.

En el modelo híbrido con un solo nodo maestro, el hilo maestro suele asignar todos los búferes de memoria mediante rutinas `malloc` estándar antes de crear los hilos. La política de administración de memoria predeterminada del sistema operativo asigna estas asignaciones exclusivamente al nodo NUMA local del hilo maestro. En consecuencia, cuando OpenMP distribuye el cálculo, la mitad de los hilos activos tendrán latencia de memoria, limitando la escalabilidad del algoritmo.

Para evitar esto, se deben configurar variables de entorno OpenMP, como `OMP_PLACES=cores` y `OMP_PROC_BIND=spread`, que asignan los hilos a núcleos de hardware específicos. Además, aplicar una política de inicialización de memoria de "primer contacto" —donde los hilos dentro de una región paralela, en lugar del hilo principal, ponen a cero inicialmente las matrices asignadas— obliga al sistema operativo a distribuir las páginas de memoria física entre todos los nodos NUMA disponibles, alineando la distribución de memoria con la distribución de procesamiento y restaurando el ancho de banda máximo de la memoria.

**¿Por qué esto aplica a nuestro proyecto?**

En la implementación actual de *filters.c*, funciones como *apply_blur*
y *apply_sobel* realizan bucles de inicialización secuenciales para establecer los búferes en negro (`img_out[i] = 0;`) antes de ejecutar la lógica de filtrado. Si simplemente encapsula los bucles de cálculo subsiguientes con directivas OpenMP sin paralelizar estos bucles de inicialización, el hilo principal realizará el "primer acceso" a las páginas de memoria, asignándolas exclusivamente a su propio socket (nodo NUMA). De esta manera, los hilos de trabajo en otros sockets tendrán una alta latencia de acceso remoto a la memoria.

**¿Es necesario para el clúster?**

- **Para garantizar la precisión:** No. El programa generará resultados correctos incluso sin ésto.
- **Para mejorar el rendimiento y la escalabilidad:** Sí. Dado que se compila y ejecuta en un clúster (donde los nodos de cómputo tienen arquitecturas multisocket con un alto número de núcleos), ignorar la mitigación NUMA va a provocar que las curvas de speedup y eficiencia de OpenMP/Hybrid no escalen correctamente.