#include "dbscan.hpp"

#include <omp.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::size_t countMismatches(const std::vector<Point>& a, const std::vector<Point>& b) {
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

}  // namespace

int main(int argc, char** argv) {
    const std::string ruta =
        argc > 1 ? argv[1] : "data/input/4000_data.csv";
    const double epsilon =
        argc > 2 ? std::stod(argv[2]) : 0.03;
    const int min_samples =
        argc > 3 ? std::stoi(argv[3]) : 10;
    const int num_threads =
        argc > 4 ? std::stoi(argv[4]) : 0;  // 0 => omp decide
    const std::string output_dir =
        argc > 5 ? argv[5] : "data/output";

    std::cout << "Ruta: " << ruta << '\n'
              << "epsilon: " << epsilon << "  min_samples: " << min_samples
              << "  threads paralelo: " << (num_threads > 0 ? num_threads : omp_get_max_threads())
              << "\n\n";

    const double inicio_serial = omp_get_wtime();
    std::vector<Point> serial_pts = dbscan_serial(ruta, epsilon, min_samples);
    const double tiempo_serial = omp_get_wtime() - inicio_serial;
    const std::string salida_serial =
        writeResultsCSV(serial_pts, output_dir, "serial");

    const double inicio_paralelo = omp_get_wtime();
    std::vector<Point> paralelo_full = dbscan_parallel_full(ruta, epsilon, min_samples, num_threads);
    const double tiempo_paralelo = omp_get_wtime() - inicio_paralelo;
    const std::string salida_paralelo =
        writeResultsCSV(paralelo_full, output_dir, "parallel_full");

    const std::size_t desmatches = countMismatches(serial_pts, paralelo_full);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Serial:    " << tiempo_serial << " s  -> " << salida_serial << '\n';
    std::cout << "Paralelo1: " << tiempo_paralelo << " s  -> " << salida_paralelo << '\n';

    const double speed_up = tiempo_serial/tiempo_paralelo;

    std::cout << "Speed up de " << speed_up << "x\n";
    
    if (desmatches == 0) {
        std::cout << "Comparación: etiquetas iguales en ambas versiones.\n";
    } else {
        std::cout << "Comparación: " << desmatches
                  << " puntos con etiqueta distinta entre serial y paralelo.\n";
    }

    std::cout << "\n(P2) versión paralela dividida queda pendiente.\n";

    return 0;
}

