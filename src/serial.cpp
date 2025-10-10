#include <cmath>    
#include <fstream>  
#include <iostream> 
#include <sstream>  
#include <string>
#include <stdexcept>
#include <vector>   
#include <filesystem>

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

vector<Point> dbscan(double epsilon, int min_samples, string ruta)
{
    string ruta_archivo = ruta;
    vector<Point> puntos = leerCSV(ruta_archivo);
    size_t n = puntos.size();

    vector<int> vecinos(n, 0);

    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = i + 1; j < n; ++j)
        {
            const double dist = distancia(puntos[i], puntos[j]);
            if (dist <= epsilon)
            {
                ++vecinos[i];
                ++vecinos[j];

                if (vecinos[i] >= min_samples)
                {
                    puntos[i].label = CORE1;
                }
                if (vecinos[j] >= min_samples)
                {
                    puntos[j].label = CORE1;
                }

                if (puntos[i].label == CORE1 && puntos[j].label == NOISE)
                {
                    puntos[j].label = CORE2;
                }
                else if (puntos[j].label == CORE1 && puntos[i].label == NOISE)
                {
                    puntos[i].label = CORE2;
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
