#ifndef HASH_UTIL_H
#define HASH_UTIL_H

typedef struct NodoHash {
    char *clave;
    long long posicion;
    struct NodoHash *siguiente;
} NodoHash;

typedef struct TablaHash {
    unsigned int capacidad;
    NodoHash **cubetas;
} TablaHash;

unsigned long long calcular_hash_fnv1a(const char *texto);
unsigned int obtener_indice_hash(const char *texto, unsigned int capacidad);
TablaHash *crear_tabla_hash(unsigned int capacidad);
void liberar_tabla_hash(TablaHash *tabla);
int insertar_en_tabla_hash(TablaHash *tabla, const char *clave, long long posicion);

#endif
