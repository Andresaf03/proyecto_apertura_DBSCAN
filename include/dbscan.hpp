#pragma once

#include <string>
#include <vector>

constexpr int NOISE = -1;
constexpr int CORE1 = 1;
constexpr int CORE2 = 2;

// Estructura de un punto (coordenadas x,y y etiqueta)
struct Point {
    double x{};
    double y{};
    int label = NOISE;
};

// Utilidades compartidas
std::vector<Point> loadPoints(const std::string& archivo);

double distanceSquared(const Point& a, const Point& b);

std::string writeResultsCSV(const std::vector<Point>& puntos, const std::string& output_dir, const std::string& etiqueta);

// Implementaciones del algoritmo
std::vector<Point> dbscan_serial(const std::string& ruta, double epsilon, int min_samples);

std::vector<Point> dbscan_parallel_full(const std::string& ruta, double epsilon, int min_samples, int num_threads = 0);

std::vector<Point> dbscan_parallel_divided(const std::string& ruta, double epsilon, int min_samples, int num_threads = 0, std::size_t block_size = 512);
