#ifndef HASH_UTIL_H
#define HASH_UTIL_H

#include <stddef.h>

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
int contar_coincidencias_hash(const TablaHash *tabla, const char *clave);
int imprimir_coincidencias_hash(const TablaHash *tabla, const char *clave);
int escribir_coincidencias_hash(const TablaHash *tabla, const char *clave,
                                char *buffer, size_t buffer_tamano);
void imprimir_todo_el_indice(const TablaHash *tabla);

#endif
