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

# Información sobre la paralelización

## OpenMP

Para acelerar el pipeline de cartoonización en multiprocesadores simétricos de un solo nodo (SMP), el sistema usa OpenMP. OpenMP emplea un modelo de ejecución *fork-join*: el hilo maestro se ejecuta de forma secuencial hasta que encuentra una región paralela, momento en el que genera un equipo de hilos de trabajo que comparten el espacio de direcciones de memoria global del nodo.

### Planificación y Paralelismo a Nivel de Bucle

Evaluar la matemática de la posterización o aplicar un núcleo de convolución a un píxel específico no depende, por su naturaleza, del valor recién calculado de ningún otro píxel. Así, el algoritmo se basa en el paralelismo a nivel de bucle, utilizando la directiva `#pragma omp parallel for` aplicada al bucle más externo (iterando sobre la altura de la imagen).

OpenMP te da tres clases principales de planificación (*scheduling*):

* **Planificación Dinámica (`schedule(dynamic)`):** Las iteraciones se distribuyen en tiempo de ejecución. Cuando un hilo completa una pequeña porción de trabajo, le consulta al planificador interno de OpenMP para obtener otra tarea. Aunque es excelente para cargas de trabajo muy impredecibles (como el trazado de rayos recursivo), la planificación dinámica introduce una sobrecarga de sincronización tremenda en el sistema operativo.
* **Planificación Guiada (`schedule(guided)`):** Es similar a la dinámica, pero los tamaños de los bloques asignados van disminuyendo de forma exponencial con el tiempo, intentando balancear la carga mientras se minimiza la sobrecarga en las etapas finales.
* **Planificación Estática (`schedule(static)`):** El espacio total de iteración se divide en grandes bloques contiguos de igual tamaño y se asigna permanentemente a hilos específicos en tiempo de compilación.

Para las operaciones que se mapean sobre matrices rectangulares, la complejidad computacional por píxel es totalmente determinista y uniforme. Por lo tanto, la planificación estática es la opción óptima, ya que elimina las consultas al planificador en tiempo de ejecución, dándote la sobrecarga de sincronización de hilos más baja posible. Además, al asignar bloques masivos y contiguos de filas a hilos individuales, la planificación estática respeta los algoritmos de *prefetching* de caché L1/L2 del hardware, mejorando muchísimo la localidad espacial y el uso del ancho de banda de memoria.

### Prevención de False Sharing y Race Conditions

Cuando tenés múltiples hilos escribiendo datos rápido en variables distintas que por casualidad están en la misma línea de caché física de la CPU, los protocolos de coherencia de caché del hardware invalidan y recargan todo el tiempo la línea de caché en diferentes núcleos. Este fenómeno, conocido como *False Sharing*, te degrada severamente la eficiencia paralela. En la implementación de la convolución, los hilos tienen que leer de una imagen fuente compartida y escribir en una de destino compartida.

Como la planificación estática obliga a los hilos a operar en bloques de memoria totalmente dispares y grandes (separados por megabytes de datos), sus punteros de escritura jamás ocupan la misma línea de caché. Así, el *false sharing* se elimina por diseño, y los bloqueos costosos de sincronización de software (como `#pragma omp critical`) pasan a ser innecesarios en los bucles de procesamiento de píxeles.

---

## MPI

Mientras que OpenMP te satura la capacidad de procesamiento de una sola máquina, escalar el cómputo a través de varias computadoras autónomas requiere MPI, que opera en un espacio de memoria distribuido. Acá los procesos (*ranks*) no pueden acceder a las variables de los demás; los datos se tienen que empaquetar explícitamente, enrutar a través de la interfaz de red y desempaquetar en el nodo receptor.

### Descomposición de Dominio

El proceso raíz de MPI (Rank 0) usa `stb_image` para cargar la imagen entera en su memoria local. Para paralelizar la carga de trabajo, este tremendo arreglo unidimensional (1D) se tiene que dividir lógicamente en subdominios y distribuirse a los *ranks* de los trabajadores. Una descomposición en bloques cartesianos 2D divide la imagen en una cuadrícula de cuadrados, lo que minimiza matemáticamente la relación perímetro-área de los subdominios, bajando teóricamente el volumen total de datos de frontera que hay que comunicar.

Transmitir una columna vertical de una imagen requiere leer direcciones de memoria dispersas y no contiguas. Aunque MPI te da tipos de datos derivados (`MPI_Type_vector` o `MPI_Type_create_subarray`) para manejar lecturas de columnas con *stride*, la sobrecarga de la CPU necesaria para empaquetar y desempaquetar continuamente estos bytes no contiguos te termina desperdiciando cualquier ahorro de la topología 2D. Por eso, la Descomposición por Filas 1D es superior.

Como cada fila se almacena de manera contigua en la memoria, el proceso raíz puede transmitir particiones enteras de la imagen usando punteros de memoria, logrando el máximo rendimiento de ancho de banda de red con un costo de empaquetado de software casi nulo.

Como las alturas de las imágenes rara vez son perfectamente divisibles por el número de nodos activos del clúster, la operación `MPI_Scatter` (que te exige fragmentos exactamente iguales) no nos va a servir, así que vamos a tener que usar `MPI_Scatterv` y `MPI_Gatherv`. Estas funciones le permiten al proceso raíz dictar un entero `sendcounts` único y un índice de memoria de desplazamientos (*offsets*) inicial para cada *rank* individual, manejando como corresponde las filas residuales de la división.

### El Mecanismo Halo Exchange

El obstáculo fundamental en el filtrado espacial distribuido es resolver los cálculos en los límites de los subdominios. Una operación de vecindario, como una convolución Sobel de 5x5, requiere leer datos de píxeles hasta un radio de dos píxeles alrededor de la coordenada objetivo. Cuando un proceso MPI intenta calcular el gradiente para la fila más alta de su bloque de imagen asignado, los píxeles vecinos que necesita están en la memoria física del proceso que maneja la partición superior. Para resolver este aislamiento de memoria, la arquitectura implementa zonas fantasma o halos.

Cuando cada proceso aloja su matriz de destino local, reserva filas adicionales y vacías en los límites extremos superior e inferior. El grosor del halo que necesitás va a estar dictado por el radio del núcleo de convolución: un núcleo de 3x3 requiere un grosor de halo de 1 fila; un núcleo de 5x5 requiere un grosor de halo de 2 filas. Antes de ejecutar los bucles de procesamiento, todos los procesos MPI se frenan para armar una fase de comunicación "*halo exchange*". Cada proceso transmite sus filas de frontera reales a sus vecinos para llenar sus zonas fantasma y, al mismo tiempo, recibe las de sus vecinos para poblar las suyas propias.

### Protocolos de Comunicación y Prevención de Deadlocks

Las implementaciones del intercambio de halo muy seguido emparejan las rutinas bloqueantes `MPI_Send` y `MPI_Recv` de forma secuencial. Si cada proceso intenta mandar datos al mismo tiempo sin publicar un búfer de recepción correspondiente, los búferes del protocolo de red se saturan al toque, lo que te termina clavando el sistema en un *deadlock* (interbloqueo).

La solución más robusta y estándar en la industria es usar la rutina `MPI_Sendrecv`. Esta función publica de forma simultánea una operación de envío y una de recepción, confiando en los *threads* asincrónicos internos de la biblioteca MPI para manejar el intercambio de datos, borrando de un plumazo el riesgo de bloqueos cíclicos.

Para una topología unidimensional por filas, el intercambio completo de halo te pide dos pasos simétricos:

1. **Intercambio Ascendente:** El rank $N$ manda sus filas reales más altas al rank $N-1$, y recibe filas fantasma del rank $N-1$ en su búfer fantasma superior.
2. **Intercambio Descendente:** El rank $N$ manda sus filas reales más bajas al rank $N+1$, y recibe filas fantasma del rank $N+1$ en su búfer fantasma inferior.

Para mantener la uniformidad del algoritmo sin tener que andar escribiendo lógica compleja para los casos extremos de los primeros (Rank 0) y últimos procesos, la arquitectura configura los límites externos para comunicarse con el destino especializado `MPI_PROC_NULL`. Meter un `MPI_Sendrecv` dirigido a `MPI_PROC_NULL` ejecuta una no-operación inmediata y exitosa, dejando que el bloque de código estándar funcione sin dramas en todo el clúster.

---

## OpenMP + MPI

Las supercomputadoras modernas tienen una estructura jerárquica: consisten en cientos de nodos físicos conectados por redes de alta velocidad, donde cada nodo tiene multiprocesadores simétricos (SMP) con decenas de núcleos de CPU físicos.

Clavar una aplicación MPI pura en esta arquitectura —instanciando un proceso MPI autónomo por núcleo físico— te genera ineficiencias graves. La biblioteca MPI está pensada para la comunicación en red. Cuando los procesos MPI que están metidos en el mismo *mother* físico intercambian datos, la biblioteca suele meter la memoria en búferes innecesariamente mediante *stacks* de red virtualizadas o sistemas de sondeo de memoria compartida, lo que te tira abajo artificialmente el ancho de banda de la memoria y te devora la RAM del sistema con búferes de contexto duplicados.

La solución definitiva es el modelo híbrido MPI + OpenMP. Este paradigma te exige lanzar un único proceso MPI por nodo físico (o por socket de CPU). MPI se limita estrictamente a manejar las comunicaciones masivas entre nodos (como `MPI_Scatterv` y los intercambios de halo). Una vez que termina la sincronización de la red, el proceso MPI invoca a OpenMP para levantar hilos livianos en los núcleos de la CPU local. Estos hilos ejecutan los cálculos complejos del algoritmo (posterización y convolución espacial) mediante memoria compartida, sin andar duplicando matrices ni llamando a protocolos de red.

### Seguridad de Hilos y Selección de Protocolo

Combinar *multithreading* con comunicaciones de red te mete riesgos de cabeza. Si varios hilos de OpenMP intentar invocar funciones MPI al mismo tiempo, el estado interno de la biblioteca MPI se te puede corromper, provocando fallos de segmentación (*segmentation faults*).

Para mitigar este riesgo, vamos a usar el protocolo `MPI_Init_thread`, que te pide que como desarrollador solicites explícitamente un nivel específico de seguridad de hilos. Los niveles que tenés disponibles son:

* `MPI_THREAD_SINGLE`: La aplicación no puede usar multihilo.
* `MPI_THREAD_FUNNELED`: La aplicación puede tener varios hilos, pero solo el hilo "maestro" original puede llamar a las rutinas de MPI.
* `MPI_THREAD_SERIALIZED`: Varios hilos pueden invocar llamadas MPI, pero tu aplicación tiene que garantizar, mediante bloqueos por software, que estas llamadas jamás se pisen en el tiempo.
* `MPI_THREAD_MULTIPLE`: Cualquier hilo puede mandar llamadas MPI en cualquier momento, y la biblioteca MPI se hace cargo totalmente del bloqueo de mutex interno y del manejo de las colas.

Si bien `MPI_THREAD_MULTIPLE` te da la mayor flexibilidad, garantizar la seguridad a este nivel hace que la biblioteca MPI tenga que meter un bloqueo interno intensivo, lo que te frena de entrada el rendimiento de la comunicación y te tira abajo el rendimiento general del clúster.

Por lo tanto, el patrón de diseño óptimo para el pipeline de cartoonización es el estilo "Master-Only" usando `MPI_THREAD_FUNNELED`. Con esta arquitectura, las comunicaciones de red se coordinan totalmente afuera de las regiones paralelas de OpenMP. El hilo maestro maneja por su cuenta los intercambios de halo `MPI_Sendrecv`, actualizando las "zonas fantasma" locales. Una vez que la memoria localizada quedó totalmente coherente, el hilo maestro clava un `#pragma omp parallel for`, liberando los hilos de cómputo a lo largo de la matriz sin riesgo de tener contención en la red.

### Mitigación de Bottlenecks en NUMA

Un problema de rendimiento enorme en arquitecturas híbridas que corren en nodos con múltiples sockets (por ejemplo, configuraciones duales AMD EPYC o Intel Xeon) es el efecto NUMA (*Non-Uniform Memory Access*).

En un sistema NUMA, los módulos de RAM físicos están conectados directo a sockets de CPU específicos. Si un hilo que está corriendo en la CPU 0 quiere acceder a una matriz que está físicamente en el banco de RAM de la CPU 1, los datos tienen que cruzar la interconexión entre sockets (como Intel QPI o AMD Infinity Fabric), lo que te genera penalizaciones de latencia importantes.

En el modelo híbrido con un único proceso, el hilo maestro suele alojar todos los búferes de memoria metiendo rutinas `malloc` estándar antes de levantar los hilos. La política de gestión de memoria por defecto del sistema operativo le clava estas asignaciones exclusivamente al nodo NUMA local del hilo maestro. Como consecuencia, cuando OpenMP distribuye el cálculo, la mitad de los hilos activos van a sufrir latencia de memoria, limitándote la escalabilidad del algoritmo.

Para zafar de esto, tenés que configurar las variables de entorno de OpenMP como `OMP_PLACES=cores` y `OMP_PROC_BIND=spread`, que te atan los hilos a núcleos de hardware específicos. Además, si aplicás una política de inicialización de memoria de "*first-touch*" (primer contacto) —donde los hilos adentro de una región paralela, en vez del hilo principal, ponen en cero inicialmente las matrices asignadas— obligás al sistema operativo a repartir las páginas de memoria física entre todos los nodos NUMA disponibles. Esto te alinea la distribución de la memoria con la del procesamiento y te devuelve el ancho de banda máximo de la memoria.

**¿Por qué esto aplica a nuestro proyecto?**

En la implementación actual de *filters.c*, funciones como *apply_blur* y *apply_sobel* meten bucles de inicialización secuenciales para dejar los búferes en negro (`img_out[i] = 0;`) antes de correr la lógica de filtrado. Si simplemente encapsulás los bucles de cálculo siguientes con directivas OpenMP sin paralelizar estos bucles de inicialización, el hilo principal va a hacer el "primer acceso" a las páginas de memoria, asignándolas exclusivamente a su propio socket (nodo NUMA). Así, los hilos de trabajo en los otros sockets van a tener una latencia alta por acceso remoto a la memoria.

**¿Es necesario para el clúster?**

* **Para garantizar la precisión:** No. El programa te va a tirar resultados correctos igual, incluso sin esto.
* **Para mejorar el rendimiento y la escalabilidad:** Sí. Como se compila y se corre en un clúster (donde los nodos de cómputo tienen arquitecturas multisocket con un montón de núcleos), ignorar la mitigación NUMA va a hacer que las curvas de *speedup* y eficiencia de OpenMP/Híbrido no escalen como corresponde.

