# Informe de Evaluación Experimental

## 1. Objetivo

Evaluar empíricamente el comportamiento temporal de las tres variantes de DBSCAN implementadas: serial, paralela P1 (parallel_full o matriz indivisible) y paralela P2 (parallel_divided o matriz dividida en bloques). El objetivo central es cuantificar el speedup obtenido al variar tanto el tamaño de la entrada como el número de hilos de ejecución, permitiendo identificar la configuración óptima para este algoritmo de clustering en nuestro entorno de hardware específico.

DBSCAN (Density-Based Spatial Clustering of Applications with Noise) es un algoritmo utilizado para la detección de clusters y clasificación de ruido en conjuntos de datos espaciales. Este algoritmo clasifica puntos en tres categorías fundamentales: CORE1 para puntos núcleo que tienen al menos min_samples vecinos dentro de un radio epsilon, CORE2 para puntos frontera que se encuentran dentro del radio epsilon de algún punto núcleo pero no califican como núcleo ellos mismos, y NOISE para puntos aislados que no pertenecen a ningún cluster.

El principal cuello de botella computacional de DBSCAN reside en el cálculo de vecinos, que requiere comparar cada punto con todos los demás puntos del conjunto de datos. Esta operación tiene una complejidad computacional de O(n²). Por eso el algoritmo sea particularmente costoso para conjuntos de datos grandes y hace mas atractiva la exploración de estrategias de paralelización efectivas.

## 2. Definición del experimento

Los experimentos se diseñaron para cubrir amplias cantidades de combinaciones que permitan observar el comportamiento de escalabilidad de las implementaciones. Los datasets utilizados son archivos sintéticos en formato CSV (`<n>_data.csv`) generados mediante la libreta `notebooks/experiments.ipynb` utilizando la función `make_blobs` de scikit-learn. Esta función genera clusters bien definidos, proporcionando un caso de prueba controlado y reproducible. Los tamaños de entrada utilizados fueron n ∈ {20,000, 40,000, 80,000, 120,000, 140,000, 160,000, 180,000, 200,000} puntos, permitiendo observar cómo cambia el desempeño con el tamaño del problema.

Los parámetros del algoritmo DBSCAN se mantuvieron constantes a lo largo de todos los experimentos: epsilon igual a 0.03 (el radio de búsqueda) y min_samples igual a 10 (el umbral para que un punto sea considerado CORE1). Estos valores fueron tomados de la generacion del csv 4000_results del notebook original del profesor.

En cuanto a la configuración de paralelización, se probaron cuatro configuraciones diferentes de hilos siguiendo la regla {1, C/2, C, 2C}, donde C representa el número de cores virtuales reportado por `omp_get_max_threads()`. En nuestro equipo de pruebas, V = 20, resultando en configuraciones de 1, 10, 20 y 40 hilos. Esta progresión permite observar el comportamiento desde la ausencia de paralelización real (1 hilo), pasando por subutilización de recursos (10 hilos), utilización completa (20 hilos), y sobreasignación (40 hilos).

Para obtener medidas estadísticamente robustas, cada combinación de (tamaño de entrada, número de hilos, implementación) se ejecutó 10 veces. El programa `main.cpp` calcula automáticamente el promedio y la desviación estándar de estos tiempos, escribiendo los resultados consolidados en `data/results/experiments.csv`. El comando de ejecución utilizado fue: `./dbscan --benchmark`.

Como salida auxiliar, cada ejecución genera archivos `data/output/<n>_results_{serial|parallel_full|parallel_divided}.csv` que contienen las etiquetas asignadas a cada punto. Estos archivos permiten validar visualmente la correctitud de las implementaciones usando las libretas de Jupyter proporcionadas, específicamente `notebooks/DBSCAN_noise.ipynb`.

## 3. Plataforma de pruebas

Los experimentos se ejecutaron en un entorno de WSL2 (Windows Subsystem for Linux 2) corriendo una distribución de Ubuntu. El sistema cuenta con 20 núcleos virtuales y 16 GB de memoria RAM, sobre una arquitectura x86_64. Esta configuración permite evaluar el comportamiento de las implementaciones en hardware moderno.

En términos de software, se utilizó el compilador GCC con soporte completo para OpenMP. Todo el código está escrito en C++17. Para el análisis de resultados y generación de gráficas se utilizó Python 3.12 con las bibliotecas pandas, matplotlib, numpy y scikit-learn, todas estándares en el ecosistema de ciencia de datos.

La medición de tiempos se realizó utilizando `omp_get_wtime()`, que proporciona mediciones de alta precisión de tiempo. Esta función es particularmente apropiada para benchmarking de código paralelo ya que captura el tiempo real transcurrido incluyendo todos los efectos de sincronización y overhead de paralelización.

## 4. Resultados principales

La tabla consolidada en `experiments.csv` muestra el tiempo promedio de ejecución (`time_avg`) y su desviación estándar (`time_std`) para cada combinación de parámetros. El análisis de estos datos revela patrones claros en el comportamiento de las tres implementaciones.

La versión serial establece nuestra línea base de referencia. Sus tiempos de ejecución crecen cuadráticamente con el tamaño de entrada, como es esperado dado que el algoritmo realiza comparaciones O(n²). Para el conjunto más pequeño de 20,000 puntos, la versión serial completa en aproximadamente 0.156 segundos, mientras que para 200,000 puntos requiere varios minutos. Esta escalabilidad limitada justifica claramente la necesidad de paralelización para conjuntos de datos de tamaño real.

La implementación P1 (parallel_full) muestra mejoras sustanciales sobre la versión serial. Con 10 hilos alcanza speedups promedio de 3.2x, llegando a aproximadamente 4.7x con 20 hilos para conjuntos de datos grandes (≥120,000 puntos). Sin embargo, observamos que P1 exhibe limitaciones de escalabilidad a medida que aumentamos el número de hilos. Con 40 hilos, el speedup solo alcanza 5.6x, sugiriendo que se está alcanzando un punto de saturación donde agregar más hilos no proporciona beneficios proporcionales debido a la contención en operaciones atómicas.

La implementación P2 (parallel_divided) demuestra mejoras consistentes sobre P1 en prácticamente todos los escenarios probados. Con 10 hilos, P2 logra speedups de 4.3x, superando a P1 por aproximadamente 34%. La ventaja se amplía con 20 hilos, donde P2 alcanza 6.4x de speedup (con picos de 6.5x), comparado con los 4.7x de P1. Esto representa una mejora del 36%, una diferencia muy significativa en términos de desempeño computacional. Incluso con 40 hilos y la degradación por sobreasignación, P2 mantiene su ventaja logrando 6.1x versus 5.6x de P1.

Con un solo hilo, ambas implementaciones paralelas muestran un overhead marginal del 3-4% comparado con la versión serial. Este comportamiento es esperado: con un solo hilo no hay paralelización real, pero sí existe el costo de la infraestructura de OpenMP (creación de regiones paralelas, inicialización de buffers locales, etc.). El overhead ligeramente menor de P2 sugiere que su diseño, a pesar de ser más complejo, introduce menos sobrecarga estructural que P1.

Un patrón interesante emerge al analizar el efecto del tamaño de entrada. Para tamaños pequeños (≤20,000 puntos), el beneficio de P2 sobre P1 es modesto. La estrategia de división en bloques se vuelve verdaderamente efectiva a partir de aproximadamente 40,000 puntos, donde el trabajo por bloque es suficiente para "esconder" el costo de la gestión de buffers locales. Con 80,000 puntos o más, P2 consistentemente supera a P1 por márgenes significativos (con 10 o mas hilos).

Las gráficas generadas por la libreta `notebooks/experiments.ipynb` visualizan estos patrones claramente. La primera gráfica muestra el speedup de P1 para diferentes configuraciones de hilos, evidenciando su escalabilidad sublineal. La segunda gráfica muestra los speedups de P2, que aunque tampoco son lineales, se acercan más al comportamiento asintotico ideal. Las subgráficas que comparan ambas estrategias directamente para cada configuración de hilos hacen evidente la ventaja consistente de P2, particularmente en la configuración óptima de 20 hilos.

## 5. Interpretación

Para comprender por qué P2 supera consistentemente a P1, debemos analizar las diferencias fundamentales en cómo ambas implementaciones abordan la paralelización del cálculo de vecinos.

P1 implementa una estrategia de paralelización directa: el bucle externo del cálculo de distancias se paraleliza con `#pragma omp parallel for`, distribuyendo las filas de la matriz de comparaciones entre los hilos disponibles. Cada hilo procesa un subconjunto de puntos i y compara cada uno con todos los puntos j subsecuentes. El problema crítico surge en la actualización de contadores: como múltiples hilos pueden intentar incrementar el mismo `vecinos[j]` simultáneamente causando condicion de carrera, cada incremento debe protegerse con `#pragma omp atomic`. Para un conjunto de 100,000 puntos, esto puede generar aproximadamente 10 mil millones de operaciones atómicas en el peor caso.

Cada operación atómica requiere sincronización. El procesador debe garantizar que solo un núcleo modifique la memoria a la vez y potencialmente hacer que hilos esperen en colas de sincronización. Cuando muchos hilos intentan actualizar contadores cercanos en memoria, este overhead de sincronización domina el tiempo de ejecución, explicando por qué P1 escala solo hasta aproximadamente 3-4x incluso con 20 hilos.

P2 ataca estos problemas mediante una estrategia de división en bloques combinada con buffers locales. Conceptualmente, la matriz de comparaciones se divide en bloques de 512×512 puntos. Cada hilo procesa un par de bloques a la vez, pero en lugar de actualizar inmediatamente el arreglo global `vecinos[]`, acumula los conteos en arreglos locales `local_i` y `local_j` que son exclusivos de ese hilo. Solo al finalizar el procesamiento del bloque completo, los valores acumulados se agregan al arreglo global usando operaciones atómicas.

Esta estrategia reduce dramáticamente el número de operaciones atómicas. Con bloques de 512 puntos, un conjunto de 100,000 puntos se divide en aproximadamente 200 bloques. Las operaciones atómicas se reducen a aproximadamente 200 × 512 = 102,400. Esta diferencia no es trivial, es la diferencia entre un programa que pasa la mayor parte de su tiempo esperando en sincronización y uno que realmente ejecuta trabajo útil.

Otro factor que contribuye al mejor desempeño de P2 es su uso de schedule dinámico en lugar de estático. Con schedule estático, las iteraciones del bucle se distribuyen en bloques predeterminados entre hilos. El problema es que en el bucle triangular superior del cálculo de vecinos (en la matriz), las iteraciones tempranas hacen más trabajo que las tardías. Esto puede llevar a una diferencia de carga donde algunos hilos terminan mucho antes que otros. P2 usa schedule dinámico, donde los bloques se asignan a hilos bajo demanda, resultando en mejor balance cuando los bloques tienen carga irregular.

La configuración óptima identificada es claramente P2 con 20 hilos. Esta configuración logra el mejor speedup absoluto (6.4x promedio con picos de 6.5x). El hecho de que 20 hilos coincida exactamente con el número de núcleos virtuales no es coincidencia: esta configuración evita tanto la subutilización de recursos (con 10 hilos) como el overhead por sobreasignación (con 40 hilos), logrando el balance óptimo entre grado de paralelismo y recursos de hardware disponibles.

## 6. Uso responsable y ético de IA generativa

- Se empleó ChatGPT para razonamiento y revisión de código. Las decisiones finales se validaron manualmente.
- Se empleó ChatGPT en la generación de funciones para leer y escribir CSVs con c++.
- No se utilizaron modelos generativos para fabricar datos o gráficos; todas las mediciones provienen de la herramienta desarrollada.

## 7. Gráficas

![speedup 1](img/speedup_p1.png)

![speedup 2](img/speedup_p2.png)

- Las figuras también pueden observarse en `notebooks/experiments.ipynb`.
- `notebooks/DBSCAN_noise.ipynb` permite validar visualmente etiquetas para cualquier salida de `data/output/`.

## 8. Datos disponibles

- `data/results/experiments.csv`: resultados consolidados del benchmark.
- `data/output/`: CSV con etiquetas por punto para cada variante.
- `notebooks/`: scripts en Python para reproducir la generación de datos y las gráficas.

## 9. Conclusiones

La división en bloques reduce los cuellos de sincronización y ofrece el mejor speedup con cargas grandes. P1 sigue siendo útil cuando se busca una paralelización directa o cuando los datasets son pequeños. Ajustar `block_size` y explorar índices espaciales serían los siguientes pasos para un proyecto a escala mayor.