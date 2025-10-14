# Informe Código y Estrategias de Paralelización

## 1. Introducción

El proyecto implementa el algoritmo DBSCAN para identificar puntos core y ruido en un plano bidimensional. Se parte de una versión serial y se construyen dos variantes paralelas con OpenMP con el objetivo de medir speedups y entender el impacto de distintas estrategias de paralelización. Este reporte cuenta con una descripción del código y las estrategias de paralelización implementadas. Para ver los datalles del experimento refiérase a `docs/informe_experimental.md`.

## 2. Organización del código

- `include/dbscan.hpp`: define el `struct Point` para estandarizar los elementos de un punto, constantes de etiquetas (`CORE1`, `CORE2`, `NOISE`) y las firmas comunes de lectura/escritura, distancia y funciones dbscan.
- `src/serial.cpp`: utilidades compartidas (`loadPoints`, `distanceSquared`, `writeResultsCSV`) y `dbscan_serial`.
- `src/parallel_1.cpp`: implementación paralela P1 (matriz indivisible).
- `src/parallel_2.cpp`: implementación paralela P2 (matriz dividida en bloques).
- `src/main.cpp`: ejecutable que corre pruebas puntuales o experimentos completa¿os (`--benchmark`).
- `data/`: conjuntos de entrada (`input/`), salidas de etiquetas (`output/`) y resultados agregados (`results/experiments.csv`).
- `notebooks/`: generación de datasets (`experiments.ipynb`) y visualización (`DBSCAN_noise.ipynb`).

## 3. Versión serial

Para la versión serial, el código que implementamos sigue la siguiente lógica. Lo común entre las tres implementaciones es la definición de las estructuras de puntos, las etiquetas asignadas, la lectura y escritura de archivos csv, el cálculo de distancias entre puntos y la lógica algorítmica de DBSCAN. En general, la implementación serial carga los puntos en vectores desde el csv, ejecuta el algoritmo de DBSCAN y escribe el resultado (puntos con etiquetas) en un csv de salida. Algo a notar es que, el cálculo de distancias usa la distancia euclideana entre dos puntos, solamente se evita usar raíz cuadrada y se eleva la distancia épsilon al cuadrado para mantener consistencia. En particular, el algoritmo DBSCAN que implementamos recorre la "matriz de distancias" (entrecomillado porque no se persiste una matriz en memoria, es solo teórica) y calcula la distancia entre dos puntos, si esta es menor a épsilon, añade al contador de épsilon-vecinos de ambos puntos. Al finalizar, para cada punto se cuentan sus épsilon-vecinos, y si este número es mayor o igual al número mínimo de la muestra para considerarlo un core, se etiqueta con `CORE1` (notar que todos los puntos comienzan con la etiqueta de `NOISE`). Finalmente, se recorre de nuevo la "matriz de distancias", únicamente para los puntos que áun son considerados como ruido, si cualquiera de esos puntos está a menos de épsilon de un punto `CORE1` etiquetado anteriormente, se etiqueta como `CORE2`.

`dbscan_serial` ejecuta DBSCAN en tres etapas:

1. **Lectura** (`loadPoints`): lee `x,y` desde CSV y, si el nombre del archivo es `<n>_data.csv`, reserva memoria aproximada para evitar reallocations.
2. **Conteo de vecinos**: recorre la mitad superior de la matriz de distancias acumulando cuántos puntos están a distancia `ε` o menos. Se compara contra `ε²` para evitar la raíz cuadrada.
3. **Etiquetado**: primero marca como `CORE1` a los puntos con `vecinos >= min_samples`; después promociona a `CORE2` a los puntos `NOISE` que son vecinos de un `CORE1`.
4. **Salida** (`writeResultsCSV`): escribe `data/output/<n>_results_serial.csv` con encabezado `x,y,label`.

Se escogieron las etiquetas de -1 para ruido, 1 para puntos `CORE1` y 2 para puntos `CORE2` vs. la alternativa de 0 o 1 para poder identificar visualmente también cuales puntos son core 2.

Esta versión sirve como referencia de corrección y como base de comparación para medir speedup.

## 4. Paralelización P1 (matriz indivisible)

La lógica de esta estrategia de paralelización es simplemente aprovechar el hecho de que se pueden ejecutar las iteraciones en los distintos hilos, al considerar la matriz como indivisible, pero hay que cuidar el hecho de que pueden existir condiciones de carrera. Para esto, utilizamos for paralelos que asignan cada iteración a un hilo y evitamos las condiciones de carrera a través atomic cuando existen procesos de escritura. Si bien no encontramos problemas de coordinación, el *overhead* que se agrega con la operación atómica parece estar mitigado por la velocidad que se gana cuando distribuimos el cómputo en los diferentes hilos.

`dbscan_parallel_full` mantiene el doble bucle global pero lo paraleliza con `#pragma omp parallel for` sobre `i`:

- Cada hilo procesa un rango de `i` y recorre todos los `j > i`.
- Las actualizaciones a `vecinos[i]` y `vecinos[j]` se protegen con `#pragma omp atomic` para evitar condiciones de carrera.
- El etiquetado (`CORE1`, `CORE2`) se realiza en pases independientes con `parallel for` y `schedule(static)`, de esta manera, podemos aprovechar hilos que ya han terminado.

La implementación es simple y logra speedups superiores a 5x con 40 hilos, aunque el número de operaciones atómicas crece con $n^2$. Aún así, se está ganando tiempo al poder procesar varios renglones de la matriz paralelamente.

## 5. Paralelización P2 (matriz dividida)

La lógica de esta estrategia de paralelización es reducir el *overhead* que agregan las operaciones atómicas de escritura a través de la reducción del número de estas operaciones. En esta estrategia no consideramos la matriz como indivisible, sino que la partimos por chunks cuadrados de tamaño constante asignados a diferentes hilos y solamente realizamos operaciones atómicas una vez que se procesó todo el chunk en cuanto al cálculo de las distancias. Este proceso es un poco más intensivo en memoria ya que crea buffers locales para almacenar los cálculos intermedios de chunks, pero en general se logran mejores speedups.

`dbscan_parallel_divided` reduce la contención dividiendo el dominio en bloques (`block_size` configurable, 512 por defecto):

- Los hilos procesan pares de bloques `(bi, bj)` únicamente en la parte superior de la matriz (para no duplicar trabajo).
- Se emplean buffers locales `local_i` y `local_j` donde se acumulan los conteos mientras se recorre el bloque.
- Al terminar cada bloque se vuelcan los acumulados al vector global con una sola pasada de `#pragma omp atomic` por elemento distinto.
- Se reutilizan las fases de etiquetado y promoción (`CORE1`/`CORE2`).

El uso de chunks de la "matriz" mejora el uso de caché y **reduce significativamente el número de atómicos**, por lo que obtiene el mejor speedup cuando hay más de 4 hilos y datasets grandes (superior a 6x con 40 hilos y 200000 puntos).

## 6. Pipeline de entrada/salida

- **Entrada**: CSVs sin encabezado (`<n>_data.csv`) generados con `notebooks/experiments.ipynb` usando `make_blobs`.
- **Procesamiento**: `main` lee el archivo, ejecuta las variantes solicitadas y mide tiempos con `omp_get_wtime`.
- **Salida**: por cada corrida se generan CSV en `data/output/` (`*_results_serial.csv`, `*_results_parallel_full.csv`, `*_results_parallel_divided.csv`). En modo `--benchmark` se agrega `data/results/experiments.csv` con promedios de tiempo y desviaciones estándar.
- **Visualización**: `notebooks/DBSCAN_noise.ipynb` permite superponer etiquetas sobre los puntos; `experiments.ipynb` construye gráficos de speedup para ambas paralelizaciones.

## 7. Consideraciones

- **Consistencia**: los modos paralelos validan sus etiquetas contra la versión serial en cada ejecución para detectar divergencias.
- **Costo**: todas las variantes siguen siendo `O(n²)`; lo que puede ser intensivo en cómputo para datasets de orden mayor. Aún así, la paralelización devuelve resultados consistentes y acelera la ejecución.
