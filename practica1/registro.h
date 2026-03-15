#ifndef REGISTRO_H
#define REGISTRO_H

#define TAMANO_TABLA 1000

struct Registro {
    int vendorID;
    long long recogidaUnix;
    long long llegadaUnix;
    int pasajeros;
    float distancia;
    int pulID;
    int dolID;
    int tipoPago;
    float tarifa;
    float total;
    long long offsetSiguiente;
};

#endif