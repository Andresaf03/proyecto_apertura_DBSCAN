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

Nuestra implementación logra un speed up de hasta **placeholder**x, con la estrategia de paralelización de **placeholder**.
