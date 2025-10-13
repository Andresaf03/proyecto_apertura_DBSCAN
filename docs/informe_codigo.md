# Informe Código y Estrategias de Paralelización

## 1. Introducción

El proyecto implementa el algoritmo DBSCAN para identificar puntos core y ruido en un plano bidimensional. Se parte de una versión serial y se construyen dos variantes paralelas con OpenMP con el objetivo de medir speedups y entender el impacto de distintas estrategias de paralelización.

## 2. Organización del código

- `include/dbscan.hpp`: define el `struct Point`, constantes de etiquetas (`CORE1`, `CORE2`, `NOISE`) y las firmas comunes.
- `src/serial.cpp`: utilidades compartidas (`loadPoints`, `distanceSquared`, `writeResultsCSV`) y `dbscan_serial`.
- `src/parallel_1.cpp`: implementación paralela P1 (matriz indivisible).
- `src/parallel_2.cpp`: implementación paralela P2 (matriz dividida en bloques).
- `src/main.cpp`: ejecutable que corre pruebas puntuales o campañas completas (`--benchmark`).
- `data/`: conjuntos de entrada (`input/`), salidas de etiquetas (`output/`) y resultados agregados (`results/experiments.csv`).
- `notebooks/`: generación de datasets (`experiments.ipynb`) y visualización (`DBSCAN_noise.ipynb`).
- `scripts/`: automatizaciones en desarrollo (`run_experiments.sh`).

## 3. Versión serial

`dbscan_serial` ejecuta DBSCAN en tres etapas:

1. **Lectura** (`loadPoints`): lee `x,y` desde CSV y, si el nombre del archivo es `<n>_data.csv`, reserva memoria aproximada para evitar reallocations.
2. **Conteo de vecinos**: recorre la mitad superior de la matriz de distancias acumulando cuántos puntos están a distancia `ε`. Se compara contra `ε²` para evitar la raíz cuadrada.
3. **Etiquetado**: primero marca como `CORE1` a los puntos con `vecinos >= min_samples`; después promociona a `CORE2` a los puntos `NOISE` que son vecinos de un `CORE1`.
4. **Salida** (`writeResultsCSV`): escribe `data/output/<n>_results_serial.csv` con encabezado `x,y,label`.

Esta versión sirve como referencia de corrección y como base de comparación para medir speedup.

## 4. Paralelización P1 (matriz indivisible)

`dbscan_parallel_full` mantiene el doble bucle global pero lo paraleliza con `#pragma omp parallel for` sobre `i`:

- Cada hilo procesa un rango de `i` y recorre todos los `j > i`.
- Las actualizaciones a `vecinos[i]` y `vecinos[j]` se protegen con `#pragma omp atomic`.
- El etiquetado (`CORE1`, `CORE2`) se realiza en pases independientes con `parallel for` y `schedule(static)`.

La implementación es simple y logra speedups cercanos a 3× con 16 hilos, aunque el número de operaciones atómicas crece con `n²`.

## 5. Paralelización P2 (matriz dividida)

`dbscan_parallel_divided` reduce la contención dividiendo el dominio en bloques (`block_size` configurable, 512 por defecto):

- Los hilos procesan pares de bloques `(bi, bj)` únicamente en la parte superior de la matriz (para no duplicar trabajo).
- Se emplean buffers locales `local_i` y `local_j` donde se acumulan los conteos mientras se recorre el bloque.
- Al terminar cada bloque se vuelcan los acumulados al vector global con una sola pasada de `#pragma omp atomic` por elemento distinto.
- Se reutilizan las fases de etiquetado y promoción (`CORE1`/`CORE2`).

El tiling mejora el uso de caché y reduce significativamente el número de atómicos, por lo que obtiene el mejor speedup cuando hay más de 4 hilos y datasets grandes (hasta ~3.9× con 16 hilos y 20000 puntos).

## 6. Pipeline de entrada/salida

- **Entrada**: CSVs sin encabezado (`<n>_data.csv`) generados con `notebooks/experiments.ipynb` usando `make_blobs`.
- **Procesamiento**: `main` lee el archivo, ejecuta las variantes solicitadas y mide tiempos con `omp_get_wtime`.
- **Salida**: por cada corrida se generan CSV en `data/output/` (`*_results_serial.csv`, `*_results_parallel_full.csv`, `*_results_parallel_divided.csv`). En modo `--benchmark` se agrega `data/results/experiments.csv` con promedios y desviaciones estándar.
- **Visualización**: `notebooks/DBSCAN_noise.ipynb` permite superponer etiquetas sobre los puntos; `experiments.ipynb` construye tablas y gráficos de speedup para ambas paralelizaciones.

## 7. Compilación y ejecución

```bash
g++-15 -std=c++17 -Iinclude -fopenmp \
    src/main.cpp src/serial.cpp src/parallel_1.cpp src/parallel_2.cpp \
    -o dbscan

./dbscan data/input/4000_data.csv 0.03 10 8
./dbscan --benchmark 0.03 10 10 data/output data/results/experiments.csv 512
```

## 8. Consideraciones y futuro

- **Consistencia**: los modos paralelos validan sus etiquetas contra la versión serial en cada ejecución para detectar divergencias.
- **Costo**: todas las variantes siguen siendo `O(n²)`; para datasets de orden mayor sería necesario un índice espacial o técnicas de pruning.
- **Parámetros**: `block_size` puede ajustarse según la jerarquía de caché del equipo en la línea de comandos.
- **Extensiones**: integrar un modo de comparación P1 vs P2 durante el benchmark, agregar pruebas unitarias para `loadPoints` y explorar soporte para `std::execution` una vez que la STL lo permita con OpenMP.
