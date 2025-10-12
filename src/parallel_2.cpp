#include "dbscan.hpp"

#include <omp.h>

#include <algorithm>
#include <vector>

namespace {

std::size_t computeBlockCount(std::size_t n, std::size_t block_size) {
    if (block_size == 0) {
        block_size = 512;
    }
    return (n + block_size - 1) / block_size;
}

} 

std::vector<Point> dbscan_parallel_divided(const std::string& ruta,
                                           double epsilon,
                                           int min_samples,
                                           int num_threads,
                                           std::size_t block_size) {
    std::vector<Point> puntos = loadPoints(ruta);
    const std::size_t n = puntos.size();
    if (n == 0) {
        return puntos;
    }

    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }

    if (block_size == 0) {
        block_size = 512;
    }

    const std::size_t block_count = computeBlockCount(n, block_size);
    const double eps2 = epsilon * epsilon;
    std::vector<int> vecinos(n, 0);

#pragma omp parallel
    {
        std::vector<int> local_i;
        std::vector<int> local_j;

#pragma omp for schedule(dynamic)
        for (std::size_t bi = 0; bi < block_count; ++bi) {
            const std::size_t i_begin = bi * block_size;
            const std::size_t i_end = std::min(n, i_begin + block_size);
            const std::size_t i_block = i_end - i_begin;
            if (i_block == 0) {
                continue;
            }

            local_i.assign(i_block, 0);

            for (std::size_t bj = bi; bj < block_count; ++bj) {
                const std::size_t j_begin = bj * block_size;
                const std::size_t j_end = std::min(n, j_begin + block_size);
                const std::size_t j_block = j_end - j_begin;
                if (j_block == 0) {
                    continue;
                }

                if (bj != bi) {
                    local_j.assign(j_block, 0);
                }

                if (bi == bj) {
                    for (std::size_t ii = i_begin; ii < i_end; ++ii) {
                        const std::size_t local_ii = ii - i_begin;
                        for (std::size_t jj = ii + 1; jj < i_end; ++jj) {
                            if (distanceSquared(puntos[ii], puntos[jj]) <= eps2) {
                                ++local_i[local_ii];
                                ++local_i[jj - i_begin];
                            }
                        }
                    }
                } else {
                    for (std::size_t ii = i_begin; ii < i_end; ++ii) {
                        const std::size_t local_ii = ii - i_begin;
                        for (std::size_t jj = j_begin; jj < j_end; ++jj) {
                            if (distanceSquared(puntos[ii], puntos[jj]) <= eps2) {
                                ++local_i[local_ii];
                                ++local_j[jj - j_begin];
                            }
                        }
                    }

                    for (std::size_t offset = 0; offset < j_block; ++offset) {
                        const int value = local_j[offset];
                        if (value != 0) {
#pragma omp atomic
                            vecinos[j_begin + offset] += value;
                        }
                    }
                }
            }

            for (std::size_t offset = 0; offset < i_block; ++offset) {
                const int value = local_i[offset];
                if (value != 0) {
#pragma omp atomic
                    vecinos[i_begin + offset] += value;
                }
            }
        }
    }

#pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < n; ++i) {
        if (vecinos[i] >= min_samples) {
            puntos[i].label = CORE1;
        }
    }

#pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < n; ++i) {
        if (puntos[i].label != NOISE) {
            continue;
        }
        for (std::size_t j = 0; j < n; ++j) {
            if (puntos[j].label == CORE1 &&
                distanceSquared(puntos[i], puntos[j]) <= eps2) {
                puntos[i].label = CORE2;
                break;
            }
        }
    }

    return puntos;
}

