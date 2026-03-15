#include <iostream>
#include <fstream>
#include <sstream>
#include "registro.h"
using namespace std;

int hashFuncion(int clave) {
    return clave % TAMANO_TABLA;
}

int main() {
    ifstream archivoCSV("../dataset/dataset_grande.csv");
    ofstream archivoBinario("viajes.dat", ios::binary);
    ofstream archivoTabla("tabla_hash.dat", ios::binary);
    
    if (!archivoCSV.is_open()) {
        cout << "ERROR: No se pudo abrir el CSV" << endl;
        return 1;
    }
    
    long long tablaHash[TAMANO_TABLA];
    for (int i = 0; i < TAMANO_TABLA; i++) {
        tablaHash[i] = -1;
    }
    
    string linea;
    getline(archivoCSV, linea); // Saltar encabezados
    
    cout << "INDEXANDO ARCHIVO..." << endl;
    
    int contador = 0;
    long long posicionActual = 0;
    
    while (getline(archivoCSV, linea)) {
        stringstream ss(linea);
        string campo;
        Registro r;
        
        // ORDEN CORRECTO según tu CSV:
        getline(ss, campo, ','); r.vendorID = atoi(campo.c_str());              // VendorID
        getline(ss, campo, ','); // tpep_pickup_datetime (lo ignoramos)
        getline(ss, campo, ','); // tpep_dropoff_datetime (lo ignoramos)
        getline(ss, campo, ','); r.pasajeros = atoi(campo.c_str());              // passenger_count
        getline(ss, campo, ','); r.distancia = atof(campo.c_str());              // trip_distance
        getline(ss, campo, ','); // RatecodeID (lo ignoramos)
        getline(ss, campo, ','); // store_and_fwd_flag (lo ignoramos)
        getline(ss, campo, ','); r.pulID = atoi(campo.c_str());                  // ← PULocationID (COLUMNA 8)
        getline(ss, campo, ','); r.dolID = atoi(campo.c_str());                  // DOLocationID
        getline(ss, campo, ','); r.tipoPago = atoi(campo.c_str());               // Paymode
        
        // Los campos que no están en tu CSV los dejamos en 0
        r.recogidaUnix = 0;
        r.llegadaUnix = 0;
        r.tarifa = 0;      // Si no está en tu CSV
        r.total = 0;       // Si no está en tu CSV
        r.offsetSiguiente = -1;
        
        // MOSTRAR LOS PRIMEROS 10 PARA VERIFICAR
        if (contador < 10) {
            cout << "Registro " << contador << ": PULocationID = " << r.pulID << endl;
        }
        
        int hash = hashFuncion(r.pulID);
        long long posicionDeEsteRegistro = posicionActual;
        
        archivoBinario.write((char*)&r, sizeof(Registro));
        
        r.offsetSiguiente = tablaHash[hash];
        tablaHash[hash] = posicionDeEsteRegistro;
        
        archivoBinario.seekp(posicionDeEsteRegistro);
        archivoBinario.write((char*)&r, sizeof(Registro));
        archivoBinario.seekp(0, ios::end);
        
        posicionActual += sizeof(Registro);
        contador++;
        
        if (contador % 100000 == 0) {
            cout << "Procesados " << contador << " registros..." << endl;
        }
    }
    
    cout << "\n¡INDEXACIÓN COMPLETADA!" << endl;
    cout << "Registros procesados: " << contador << endl;
    
    archivoTabla.write((char*)tablaHash, sizeof(long long) * TAMANO_TABLA);
    
    archivoCSV.close();
    archivoBinario.close();
    archivoTabla.close();
    
    return 0;
}