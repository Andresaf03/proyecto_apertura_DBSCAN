#include "dbscan.hpp"

#include <omp.h>

#include <vector>

using std::size_t;

std::vector<Point> dbscan_parallel_full(const std::string& ruta, 
                                        double epsilon, 
                                        int min_samples, 
                                        int num_threads) {
    std::vector<Point> puntos = loadPoints(ruta);
    const size_t n = puntos.size();
    if (n == 0) {
        return puntos;
    }

    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    const double eps2 = epsilon * epsilon;
    std::vector<int> vecinos(n, 0);

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (distanceSquared(puntos[i], puntos[j]) <= eps2) {
#pragma omp atomic
                ++vecinos[i];
#pragma omp atomic
                ++vecinos[j];
            }
        }
    }

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        if (vecinos[i] >= min_samples) {
            puntos[i].label = CORE1;
        }
    }

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < n; ++i) {
        if (puntos[i].label != NOISE) {
            continue;
        }
        for (size_t j = 0; j < n; ++j) {
            if (puntos[j].label == CORE1 &&
                distanceSquared(puntos[i], puntos[j]) <= eps2) {
                puntos[i].label = CORE2;
                break;
            }
        }
    }

    return puntos;
}

