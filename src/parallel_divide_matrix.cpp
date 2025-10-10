#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <omp.h>

#define NOISE -1
#define CORE1 1
#define CORE2 2

using namespace std;

struct Point
{
    double x;
    double y;
    int label = NOISE;
};

double distancia(Point a, Point b)
{
    double dist_x = a.x - b.x;
    double dist_y = a.y - b.y;
    double dist = sqrt(dist_x * dist_x + dist_y * dist_y);
    return dist;
}

vector<Point> leerCSV(const string &archivo)
{
    ifstream in(archivo);
    if (!in)
    {
        cerr << "Error: no se pudo abrir " << archivo << endl;
        return {};
    }

    // Si el nombre viene como 4000_data.csv reservamos memoria aproximada
    vector<Point> puntos;
    const auto nombre = archivo.substr(archivo.find_last_of("/\\") + 1);
    const auto subrayado = nombre.find('_');
    if (subrayado != string::npos)
    {
        try
        {
            size_t estimado = static_cast<size_t>(stoul(nombre.substr(0, subrayado)));
            puntos.reserve(estimado);
        }
        catch (const exception &)
        {
            // Si no podemos parsear la longitud simplemente continuamos sin reservar.
        }
    }

    string linea;
    while (getline(in, linea))
    {
        if (linea.empty())
        {
            continue;
        }

        stringstream ss(linea);
        Point p;
        char coma;
        if (ss >> p.x >> coma >> p.y)
        {
            puntos.push_back(p);
        }
    }

    return puntos;
}

vector<Point> dbscan_parallel_p2(double epsilon, int min_samples, string ruta, int num_threads)
{
    string ruta_archivo = ruta;
    vector<Point> puntos = leerCSV(ruta_archivo);
    size_t n = puntos.size();

    vector<int> vecinos(n, 0);

    omp_set_num_threads(num_threads);

// Paso 1: Calcular vecinos y clasificar (PARALELO - Matriz Dividida)
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();

        // Calcular rango de filas para este thread
        size_t chunk_size = (n + nthreads - 1) / nthreads;
        size_t start = tid * chunk_size;
        size_t end = (start + chunk_size > n) ? n : start + chunk_size;

        // Cada thread procesa su rango de puntos
        for (size_t i = start; i < end; ++i)
        {
            // Comparar con TODOS los puntos (j desde i+1 hasta n)
            for (size_t j = i + 1; j < n; ++j)
            {
                const double dist = distancia(puntos[i], puntos[j]);
                if (dist <= epsilon)
                {
// Actualizar contadores de forma atómica
#pragma omp atomic
                    ++vecinos[i];

#pragma omp atomic
                    ++vecinos[j];
                }
            }
        }
    }

// Barrera implícita después del bloque parallel

// Clasificar según vecinos (PARALELO)
#pragma omp parallel for
    for (size_t i = 0; i < n; ++i)
    {
        if (vecinos[i] >= min_samples)
        {
            puntos[i].label = CORE1;
        }
    }

// Paso 2: Reclasificar NOISE a CORE2 (PARALELO - División por bloques)
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();

        size_t chunk_size = (n + nthreads - 1) / nthreads;
        size_t start = tid * chunk_size;
        size_t end = (start + chunk_size > n) ? n : start + chunk_size;

        for (size_t i = start; i < end; ++i)
        {
            if (puntos[i].label == NOISE)
            {
                // Buscar en TODOS los puntos core
                for (size_t j = 0; j < n; ++j)
                {
                    if (puntos[j].label == CORE1 && distancia(puntos[i], puntos[j]) <= epsilon)
                    {
                        puntos[i].label = CORE2;
                        break;
                    }
                }
            }
        }
    }

    return puntos;
}

string escribirCSV(const vector<Point> &puntos, const string &output_dir = "data/output")
{
    if (!std::filesystem::exists(output_dir))
    {
        std::filesystem::create_directories(output_dir);
    }
    const string archivo = output_dir + "/" + to_string(puntos.size()) + "_results_parallel_p2.csv";
    ofstream out(archivo);
    if (!out)
    {
        cerr << "Error: no se pudo escribir " << archivo << endl;
        return "";
    }
    out << "x,y,label\n";
    for (const auto &p : puntos)
    {
        out << p.x << ',' << p.y << ',' << p.label << '\n';
    }
    return archivo;
}

int main()
{
    string ruta = "../data/input/4000_data.csv";
    double epsilon = 0.03;
    int min_samples = 10;
    int num_threads = 4; // Ajusta según tu CPU

    double inicio = omp_get_wtime();
    vector<Point> res = dbscan_parallel_p2(epsilon, min_samples, ruta, num_threads);
    double fin = omp_get_wtime();

    cout << "Tiempo: " << (fin - inicio) << " segundos" << endl;

    string salida = escribirCSV(res);
    cout << "El archivo se genero en: " << salida << endl;
}