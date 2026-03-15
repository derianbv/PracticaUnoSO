#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "registro.h"
using namespace std;

int main() {
    // Crear tuberías (si ya existen, no pasa nada)
    mkfifo("/tmp/tuberia_peticion", 0666);
    mkfifo("/tmp/tuberia_respuesta", 0666);
    
    cout << "🚀 Servidor esperando peticiones..." << endl;
    
    // Cargar tabla hash
    long long tablaHash[TAMANO_TABLA];
    ifstream archivoTabla("tabla_hash.dat", ios::binary);
    if (!archivoTabla.is_open()) {
        cout << "❌ No encuentro tabla_hash.dat" << endl;
        return 1;
    }
    archivoTabla.read((char*)tablaHash, sizeof(long long) * TAMANO_TABLA);
    archivoTabla.close();
    
    // Abrir archivo de viajes
    ifstream archivoViajes("viajes.dat", ios::binary);
    if (!archivoViajes.is_open()) {
        cout << "❌ No encuentro viajes.dat" << endl;
        return 1;
    }
    
    while (true) {
        // Abrir tubería de petición para LEER
        int fd_peticion = open("/tmp/tuberia_peticion", O_RDONLY);
        int pulABuscar;
        read(fd_peticion, &pulABuscar, sizeof(int));
        close(fd_peticion);
        
        if (pulABuscar == 0) {
            cout << "👋 Servidor terminado" << endl;
            break;
        }
        
        cout << "📥 Buscando PULocationID: " << pulABuscar << endl;
        
        // Calcular hash y buscar
        int hash = pulABuscar % TAMANO_TABLA;
        long long offset = tablaHash[hash];
        
        // Abrir tubería de respuesta para ESCRIBIR
        int fd_respuesta = open("/tmp/tuberia_respuesta", O_WRONLY);
        
        if (offset == -1) {
            // Enviar marca de "no encontrado"
            Registro vacio;
            vacio.pulID = -1;
            write(fd_respuesta, &vacio, sizeof(Registro));
        } else {
            int encontrados = 0;
            while (offset != -1) {
                archivoViajes.seekg(offset);
                Registro r;
                archivoViajes.read((char*)&r, sizeof(Registro));
                
                if (r.pulID == pulABuscar) {
                    encontrados++;
                    write(fd_respuesta, &r, sizeof(Registro));
                }
                offset = r.offsetSiguiente;
            }
            // Marca de fin
            Registro fin;
            fin.pulID = -1;
            write(fd_respuesta, &fin, sizeof(Registro));
            cout << "📤 Enviados " << encontrados << " registros" << endl;
        }
        close(fd_respuesta);
    }
    
    archivoViajes.close();
    return 0;
}