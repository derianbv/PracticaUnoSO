#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "registro.h"
using namespace std;

int main() {
    int opcion;
    int criterio;
    
    do {
        cout << "\n=== SISTEMA DE BUSQUEDA DE VIAJES ===" << endl;
        cout << "1. Buscar por PULocationID" << endl;
        cout << "2. Salir" << endl;
        cout << "Opcion: ";
        cin >> opcion;
        
        switch(opcion) {
            case 1: {
                cout << "Ingrese PULocationID a buscar: ";
                cin >> criterio;
                
                // Enviar petición al servidor
                int fd_peticion = open("/tmp/tuberia_peticion", O_WRONLY);
                write(fd_peticion, &criterio, sizeof(int));
                close(fd_peticion);
                
                // Recibir respuestas
                int fd_respuesta = open("/tmp/tuberia_respuesta", O_RDONLY);
                Registro r;
                int contador = 0;
                
                cout << "\n📋 RESULTADOS DE BUSQUEDA:" << endl;
                cout << "==========================" << endl;
                
                while (true) {
                    read(fd_respuesta, &r, sizeof(Registro));
                    if (r.pulID == -1) break;  // Marca de fin
                    
                    contador++;
                    cout << "\n--- VIAJE " << contador << " ---" << endl;
                    cout << "  VendorID: " << r.vendorID << endl;
                    cout << "  Pasajeros: " << r.pasajeros << endl;
                    cout << "  Distancia: " << r.distancia << " millas" << endl;
                    cout << "  Destino (DOLocationID): " << r.dolID << endl;
                }
                close(fd_respuesta);
                
                if (contador == 0) {
                    cout << "No se encontraron viajes con PULocationID " << criterio << endl;
                } else {
                    cout << "\n==========================" << endl;
                    cout << "Total de viajes encontrados: " << contador << endl;
                }
                break;
            }
            
            case 2: {
                cout << "Saliendo..." << endl;
                // Avisar al servidor que termine
                int fd_peticion = open("/tmp/tuberia_peticion", O_WRONLY);
                int salir = 0;
                write(fd_peticion, &salir, sizeof(int));
                close(fd_peticion);
                break;
            }
            
            default:
                cout << "Opcion no valida" << endl;
        }
    } while(opcion != 2);
    
    return 0;
}
