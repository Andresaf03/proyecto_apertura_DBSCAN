# Proyecto de Apertura DBSCAN paralelo

## Descripción

Este es el proyecto de apertura para la materia de Cómputo Paralelo y en la Nube que consiste en implementar un DBSCAN paralelo para la detección de ruido o outliers con OpenMP.

Se implementa una versión serial y dos paralelas del algoritmo DBSCAN para comparar el tiempo de ejecución y obtener un speed up de al menos 1.5x. La primera versión paralela considera a toda la matriz como indivisible, mientras que la segunda aprovecha la previsualización de datos para crear una grilla a partir de la matriz original. Adicionalmente se cumple con el uso responsable y ético de la IA Generativa de acuerdo a los lineamientos de IEEE y Elsevier.

## Contenido

Dentro del repositorio se incluye:

- Todo el código fuente con la implementación serial y las dos implementaciones paralelas ubicado en `src/`.

- Un escrito con la descripción del código y las estrategias de paralelización, ubicado en `docs/informe_codigo.md`.

- Un escrito con la descripción de la evaluación experimental del diseño, ubicado en `docs/informe_experimental.md`.

  - Explicación detallada de la definición del experimento.
  
  - Descripción del software y hardware donde se ejecutaron los documentos.

  - Interpretación y análisis de resultados (incluye qué version paralela es mejor y por qué).

  - Uso responsable y ético de la IA Generativa.

  - Gráficas.

  - Archivo con los datos del experimento.

## Integrantes

- Andrés Álvarez Flores (208450)

- Nicolas Álvarez Ortega (206379)

## Hallazgos principales

- La versión paralela P1 alcanza speedups de ~3.4× con 16 hilos en los datasets grandes.
- La versión paralela P2, que trabaja por bloques, llega a ~3.9× con 8–16 hilos y `block_size = 512`.
- Los archivos de resultados (`data/results/experiments.csv`) y las gráficas en `notebooks/experiments.ipynb` documentan el comportamiento completo.

## Requisitos y compilación

1. **Generar datasets**: abre `notebooks/experiments.ipynb` y ejecuta la sección de generación para crear `n_data.csv` en `data/input/`. Sin estos archivos, el binario no encontrará las rutas por defecto.
2. **Entorno Python** (para notebooks y análisis):

   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r src/requirements.txt
   ```

3. **Compilación C++** (OpenMP/C++17 probados con GCC 15 Homebrew):

   ```bash
   g++-15 -std=c++17 -Iinclude -fopenmp \
       src/main.cpp src/serial.cpp src/parallel_1.cpp src/parallel_2.cpp \
       -o dbscan
   ```

  *Si usas `clang++`, instala `libomp` y ajusta los flags.*

## Uso

- **Ejecución puntual (valores por defecto)**

  ```bash
  ./dbscan
  ```

  Usa `data/input/4000_data.csv`, `ε = 0.03`, `min_samples = 10`, `omp_get_max_threads()` hilos, salida en `data/output/` y `block_size = 512`.

- **Ejecución puntual con argumentos**

  ```bash
  ./dbscan <ruta_csv> <epsilon> <min_samples> <num_threads> [directorio_salida] [block_size]
  # Ejemplo
  ./dbscan data/input/4000_data.csv 0.03 10 8 data/output 512
  ```
  
  Los parámetros opcionales permiten cambiar la carpeta de resultados y el tamaño de bloque usado por P2.

- **Benchmark (valores por defecto)**
  
  ```bash
  ./dbscan --benchmark
  ```
  
  Ejecuta los tamaños `{20k … 200k}` promediando 10 corridas por configuración y genera `data/results/experiments.csv`.

- **Benchmark con argumentos**
  
  ```bash
  ./dbscan --benchmark <epsilon> <min_samples> <iteraciones> <directorio_salida> <archivo_resultados> [block_size]
  # Ejemplo
  ./dbscan --benchmark 0.03 10 10 data/output data/results/experiments.csv 512
  ```

- **Generación y análisis**: usa `notebooks/experiments.ipynb` para crear datasets y gráficas; `notebooks/DBSCAN_noise.ipynb` visualiza etiquetas.

## Estructura del repositorio

```txt
├── include/
│   └── dbscan.hpp
├── src/
│   ├── main.cpp
│   ├── serial.cpp
│   ├── parallel_1.cpp
│   └── parallel_2.cpp
├── data/
│   ├── input/
│   ├── output/
│   └── results/
├── notebooks/
│   ├── experiments.ipynb
│   └── DBSCAN_noise.ipynb
├── docs/
│   ├── informe_codigo.md
│   └── informe_experimental.md
└── README.md
```
