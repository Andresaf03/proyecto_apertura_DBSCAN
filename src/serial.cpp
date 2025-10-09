#include <cmath>    
#include <fstream>  
#include <iostream> 
#include <sstream>  
#include <string>
#include <stdexcept>
#include <vector>   
#include <filesystem>

using namespace std;
#define NOISE -1
#define CORE1 1
#define CORE2 2

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

double **crearMatrizDistancias(const vector<Point> &puntos)
{
    const size_t n = puntos.size();
    if (n == 0)
    {
        return nullptr;
    }
    double **matriz = new double *[n];
    for (size_t i = 0; i < n; ++i)
    {
        matriz[i] = new double[n];
    }
    for (size_t i = 0; i < n; ++i)
    {
        matriz[i][i] = 0.0;
        for (size_t j = i + 1; j < n; ++j)
        {
            const double dist = distancia(puntos[i], puntos[j]);
            matriz[i][j] = dist;
            matriz[j][i] = dist;
        }
    }
    return matriz;
}

void liberarMatrizDistancias(double **matriz, size_t filas)
{
    if (!matriz)
    {
        return;
    }
    for (size_t i = 0; i < filas; ++i)
    {
        delete[] matriz[i];
    }
    delete[] matriz;
}

vector<Point> dbscan(double epsilon, int min_samples, string ruta)
{
    string ruta_archivo = ruta;
    vector<Point> puntos = leerCSV(ruta_archivo);
    size_t n = puntos.size();

    double **distancias = crearMatrizDistancias(puntos);

    std::vector<int> vecinos(n, 0);

    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            if (i == j)
                continue;
            if (distancias[i][j] <= epsilon)
            {
                ++vecinos[i];
            }
        }
        if (vecinos[i] >= min_samples)
        {
            puntos[i].label = CORE1;
        }
    }

    // Paso 2: upgrade a segundo orden si alcanzable desde un core
    for (size_t i = 0; i < n; ++i)
    {
        if (puntos[i].label != NOISE)
            continue;
        for (size_t j = 0; j < n; ++j)
        {
            if (puntos[j].label == CORE1 && distancias[i][j] <= epsilon)
            {
                puntos[i].label = CORE2;
                break;
            }
        }
    }

    liberarMatrizDistancias(distancias, n);

    return puntos;
}

string escribirCSV(const vector<Point> &puntos, const string &output_dir = "data/output")
{
    if (!std::filesystem::exists(output_dir))
    {
        std::filesystem::create_directories(output_dir);
    }
    const string archivo = output_dir + "/" + to_string(puntos.size()) + "_results_serial.csv";
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
    string ruta = "data/input/4000_data.csv";
    double epsilon = 0.03;
    int min_samples = 10;

    vector<Point> res = dbscan(epsilon, min_samples, ruta);
    string salida = escribirCSV(res);

    cout << "El archivo se genero en: " << salida << endl;
}