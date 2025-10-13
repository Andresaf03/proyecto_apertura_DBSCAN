#include "dbscan.hpp"

#include <omp.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
    std::size_t countMismatches(const std::vector<Point> &a, const std::vector<Point> &b) {
        if (a.size() != b.size()) {
            return std::max(a.size(), b.size());
        }
        std::size_t mismatches = 0;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (a[i].label != b[i].label) {
                ++mismatches;
            }
        }
        return mismatches;
    }

    struct Stats {
        double mean{};
        double stdev{};
    };

    Stats computeStats(const std::vector<double> &tiempos) {
        if (tiempos.empty()) {
            return {0.0, 0.0};
        }
        double suma = 0.0;
        for (double t : tiempos) {
            suma += t;
        }
        const double mean = suma / static_cast<double>(tiempos.size());

        double var = 0.0;
        for (double t : tiempos) {
            const double diff = t - mean;
            var += diff * diff;
        }
        var /= static_cast<double>(tiempos.size());
        return {mean, std::sqrt(var)};
    }

    Stats runSerial(const std::string &ruta,
                    double epsilon,
                    int min_samples,
                    int iterations,
                    std::vector<double> &tiempos,
                    const std::string &output_dir,
                    std::vector<Point> &ultimo_serial) {
        tiempos.clear();
        tiempos.reserve(iterations);
        for (int i = 0; i < iterations; ++i) {
            const double inicio = omp_get_wtime();
            ultimo_serial = dbscan_serial(ruta, epsilon, min_samples);
            tiempos.push_back(omp_get_wtime() - inicio);
        }
        const std::string salida =
            writeResultsCSV(ultimo_serial, output_dir, "serial");
        std::cout << "  -> serial guardó: " << salida << '\n';
        return computeStats(tiempos);
    }

    Stats runParallelFull(const std::string &ruta,
                          double epsilon,
                          int min_samples,
                          int threads,
                          int iterations,
                          std::vector<double> &tiempos,
                          const std::string &output_dir,
                          std::vector<Point> &ultimo,
                          const std::vector<Point> &referencia_serial) {
        if (threads > 0) {
            omp_set_num_threads(threads);
        }
        tiempos.clear();
        tiempos.reserve(iterations);
        for (int i = 0; i < iterations; ++i) {
            const double inicio = omp_get_wtime();
            ultimo = dbscan_parallel_full(ruta, epsilon, min_samples, threads);
            tiempos.push_back(omp_get_wtime() - inicio);
        }
        const std::string salida =
            writeResultsCSV(ultimo, output_dir, "parallel_full");
        std::cout << "  -> paralelo1 guardó: " << salida << '\n';

        const std::size_t mismatches = countMismatches(referencia_serial, ultimo);
        if (mismatches != 0) {
            std::cout << "  (!) " << mismatches
                      << " etiquetas difieren entre serial y paralelo1.\n";
        }

        return computeStats(tiempos);
    }

    Stats runParallelDivided(const std::string &ruta,
                             double epsilon,
                             int min_samples,
                             int threads,
                             int iterations,
                             std::size_t block_size,
                             std::vector<double> &tiempos,
                             const std::string &output_dir,
                             std::vector<Point> &ultimo,
                             const std::vector<Point> &referencia_serial) {
        if (threads > 0) {
            omp_set_num_threads(threads);
        }
        tiempos.clear();
        tiempos.reserve(iterations);
        for (int i = 0; i < iterations; ++i) {
            const double inicio = omp_get_wtime();
            ultimo = dbscan_parallel_divided(ruta, epsilon, min_samples, threads, block_size);
            tiempos.push_back(omp_get_wtime() - inicio);
        }
        const std::string salida =
            writeResultsCSV(ultimo, output_dir, "parallel_divided");
        std::cout << "  -> paralelo2 guardó: " << salida << '\n';

        const std::size_t mismatches = countMismatches(referencia_serial, ultimo);
        if (mismatches != 0) {
            std::cout << "  (!) " << mismatches
                      << " etiquetas difieren entre serial y paralelo2.\n";
        }

        return computeStats(tiempos);
    }

}

int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "--benchmark") {
        const double epsilon = argc > 2 ? std::stod(argv[2]) : 0.03;
        const int min_samples = argc > 3 ? std::stoi(argv[3]) : 10;
        const int iterations = argc > 4 ? std::stoi(argv[4]) : 10;
        const std::string output_dir = argc > 5 ? argv[5] : "data/output";
        const std::string results_file = argc > 6 ? argv[6] : "data/results/experiments.csv";
        const std::size_t block_size = argc > 7 ? static_cast<std::size_t>(std::stoul(argv[7])) : 512;

        const std::vector<std::size_t> sizes = {20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000};

        const int virtual_cores = omp_get_max_threads();
        const std::vector<int> thread_options = {
            1,
            std::max(1, virtual_cores / 2),
            virtual_cores,
            virtual_cores * 2};

        if (!results_file.empty()) {
            fs::create_directories(fs::path(results_file).parent_path());
        }
        std::ofstream csv(results_file);
        csv << "points,threads,mode,time_avg,time_std\n";

        std::vector<double> tiempos_serial;
        std::vector<double> tiempos_par_full;
        std::vector<double> tiempos_par_div;

        for (std::size_t n : sizes) {
            const std::string ruta = "data/input/" + std::to_string(n) + "_data.csv";
            std::cout << "\n=== Tamaño: " << n << " puntos ===\n";
            std::vector<Point> ultimo_serial;

            const Stats stats_serial = runSerial(
                ruta, epsilon, min_samples, iterations,
                tiempos_serial, output_dir, ultimo_serial);

            if (ultimo_serial.empty()) {
                std::cout << "  (!) No se pudieron cargar puntos para " << ruta
                          << ". Se omite este tamaño.\n";
                continue;
            }

            csv << n << ",1,serial,"
                << stats_serial.mean << ',' << stats_serial.stdev << '\n';
            csv.flush();

            for (int threads : thread_options) {
                std::cout << "  Hilos: " << threads << '\n';
                std::vector<Point> ultimo_par_full;
                const Stats stats_par_full = runParallelFull(
                    ruta, epsilon, min_samples, threads,
                    iterations, tiempos_par_full, output_dir,
                    ultimo_par_full, ultimo_serial);

                csv << n << ',' << threads << ",parallel_full,"
                    << stats_par_full.mean << ',' << stats_par_full.stdev << '\n';
                csv.flush();

                std::vector<Point> ultimo_par_div;
                const Stats stats_par_div = runParallelDivided(
                    ruta, epsilon, min_samples, threads,
                    iterations, block_size, tiempos_par_div,
                    output_dir, ultimo_par_div, ultimo_serial);

                csv << n << ',' << threads << ",parallel_divided,"
                    << stats_par_div.mean << ',' << stats_par_div.stdev << '\n';
                csv.flush();
            }
        }

        std::cout << "\nResultados guardados en: " << results_file << '\n';
        return 0;
    }

    const std::string ruta =
        argc > 1 ? argv[1] : "data/input/4000_data.csv";
    const double epsilon =
        argc > 2 ? std::stod(argv[2]) : 0.03;
    const int min_samples =
        argc > 3 ? std::stoi(argv[3]) : 10;
    const int num_threads =
        argc > 4 ? std::stoi(argv[4]) : 0;
    const std::string output_dir =
        argc > 5 ? argv[5] : "data/output";
    const std::size_t block_size =
        argc > 6 ? static_cast<std::size_t>(std::stoul(argv[6])) : 512;

    std::cout << "Ruta: " << ruta << '\n'
              << "epsilon: " << epsilon << "  min_samples: " << min_samples
              << "  threads paralelo: " << (num_threads > 0 ? num_threads : omp_get_max_threads())
              << "\n\n";

    std::vector<double> tiempos_serial;
    std::vector<Point> resultado_serial;
    const Stats stats_serial = runSerial(ruta, epsilon, min_samples, 1, tiempos_serial, output_dir, resultado_serial);
    if (resultado_serial.empty()) {
        std::cout << "No se pudieron cargar puntos desde " << ruta << ".\n";
        return 1;
    }
    std::vector<double> tiempos_par_full;
    std::vector<Point> resultado_paralelo;
    const Stats stats_par_full = runParallelFull(
        ruta, epsilon, min_samples, num_threads,
        1, tiempos_par_full, output_dir, resultado_paralelo, resultado_serial);
    const std::size_t mismatches = countMismatches(resultado_serial, resultado_paralelo);

    std::vector<double> tiempos_par_div;
    std::vector<Point> resultado_paralelo_div;
    const Stats stats_par_div = runParallelDivided(
        ruta, epsilon, min_samples, num_threads,
        1, block_size, tiempos_par_div, output_dir,
        resultado_paralelo_div, resultado_serial);
    const std::size_t mismatches_div = countMismatches(resultado_serial, resultado_paralelo_div);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Serial:    " << stats_serial.mean << " s\n";
    std::cout << "Paralelo1: " << stats_par_full.mean << " s\n";
    if (stats_par_full.mean > 0.0) {
        std::cout << "Speedup:   " << (stats_serial.mean / stats_par_full.mean) << "x\n";
    }
    std::cout << "Paralelo2: " << stats_par_div.mean << " s\n";
    if (stats_par_div.mean > 0.0) {
        std::cout << "Speedup2:  " << (stats_serial.mean / stats_par_div.mean) << "x\n";
    }

    if (mismatches == 0) {
        std::cout << "Comparación P1: etiquetas iguales entre serial y paralelo2.\n";
    }
    else {
        std::cout << "Comparación: " << mismatches
                  << " puntos con etiqueta distinta entre serial y paralelo.\n";
    }

    if (mismatches_div == 0) {
        std::cout << "Comparación P2: etiquetas iguales entre serial y paralelo2.\n";
    }
    else {
        std::cout << "Comparación P2: " << mismatches_div
                  << " puntos con etiqueta distinta entre serial y paralelo2.\n";
    }

    return 0;
}
