#include "dbscan.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using std::size_t;

std::vector<Point> loadPoints(const std::string& archivo) {
    std::ifstream in(archivo);
    if (!in) {
        std::cerr << "Error: no se pudo abrir " << archivo << std::endl;
        return {};
    }

    std::vector<Point> puntos;
    const auto nombre = archivo.substr(archivo.find_last_of("/\\") + 1);
    const auto subrayado = nombre.find('_');
    if (subrayado != std::string::npos) {
        try {
            size_t estimado = static_cast<size_t>(std::stoul(nombre.substr(0, subrayado)));
            puntos.reserve(estimado);
        } catch (const std::exception&) {
            // Continuamos sin reservar si no coincide el formato.
        }
    }

    std::string linea;
    while (std::getline(in, linea)) {
        if (linea.empty()) {
            continue;
        }

        std::stringstream ss(linea);
        Point p;
        char coma;
        if (ss >> p.x >> coma >> p.y) {
            puntos.push_back(p);
        }
    }

    return puntos;
}

double distanceSquared(const Point& a, const Point& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

std::string writeResultsCSV(const std::vector<Point>& puntos, 
                            const std::string& output_dir, 
                            const std::string& etiqueta) {
    namespace fs = std::filesystem;
    if (!fs::exists(output_dir)) {
        fs::create_directories(output_dir);
    }

    const std::string archivo =
        output_dir + "/" + std::to_string(puntos.size()) + "_results_" + etiqueta + ".csv";
    std::ofstream out(archivo);
    if (!out) {
        std::cerr << "Error: no se pudo escribir " << archivo << std::endl;
        return "";
    }

    out << "x,y,label\n";
    for (const auto& p : puntos) {
        out << p.x << ',' << p.y << ',' << p.label << '\n';
    }

    return archivo;
}

std::vector<Point> dbscan_serial(const std::string& ruta, 
                                 double epsilon, 
                                 int min_samples) {
    std::vector<Point> puntos = loadPoints(ruta);
    const size_t n = puntos.size();
    if (n == 0) {
        return puntos;
    }

    const double eps2 = epsilon * epsilon;
    std::vector<int> vecinos(n, 0);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (distanceSquared(puntos[i], puntos[j]) <= eps2) {
                ++vecinos[i];
                ++vecinos[j];
            }
        }
    }

    for (size_t i = 0; i < n; ++i) {
        if (vecinos[i] >= min_samples) {
            puntos[i].label = CORE1;
        }
    }

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

