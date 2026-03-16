#include "hash_util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long long calcular_hash_fnv1a(const char *texto)
{
    uint64_t hash = 1469598103934665603ULL;

    while (*texto != '\0') {
        hash ^= (unsigned char) *texto;
        hash *= 1099511628211ULL;
        texto++;
    }

    return (unsigned long long) hash;
}

unsigned int obtener_indice_hash(const char *texto, unsigned int capacidad)
{
    if (capacidad == 0) {
        return 0;
    }

    return (unsigned int) (calcular_hash_fnv1a(texto) % capacidad);
}

TablaHash *crear_tabla_hash(unsigned int capacidad)
{
    TablaHash *tabla;

    if (capacidad == 0) {
        return NULL;
    }

    tabla = (TablaHash *) malloc(sizeof(TablaHash));
    if (tabla == NULL) {
        return NULL;
    }

    tabla->cubetas = (NodoHash **) calloc(capacidad, sizeof(NodoHash *));
    if (tabla->cubetas == NULL) {
        free(tabla);
        return NULL;
    }

    tabla->capacidad = capacidad;
    return tabla;
}

void liberar_tabla_hash(TablaHash *tabla)
{
    unsigned int i;

    if (tabla == NULL) {
        return;
    }

    for (i = 0; i < tabla->capacidad; i++) {
        NodoHash *actual = tabla->cubetas[i];

        while (actual != NULL) {
            NodoHash *siguiente = actual->siguiente;
            free(actual->clave);
            free(actual->posiciones);
            free(actual);
            actual = siguiente;
        }
    }

    free(tabla->cubetas);
    free(tabla);
}

int insertar_en_tabla_hash(TablaHash *tabla, const char *clave, long long posicion)
{
    unsigned int indice;
    NodoHash *actual;
    NodoHash *nuevo;
    size_t largo;

    if (tabla == NULL || clave == NULL || clave[0] == '\0') {
        return 0;
    }

    indice = obtener_indice_hash(clave, tabla->capacidad);
    actual = tabla->cubetas[indice];

    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            if (actual->cantidad_posiciones == actual->capacidad_posiciones) {
                size_t nueva_capacidad = actual->capacidad_posiciones * 2;
                long long *nuevas_posiciones =
                    (long long *) realloc(actual->posiciones,
                                          nueva_capacidad * sizeof(long long));

                if (nuevas_posiciones == NULL) {
                    return 0;
                }

                actual->posiciones = nuevas_posiciones;
                actual->capacidad_posiciones = nueva_capacidad;
            }

            actual->posiciones[actual->cantidad_posiciones] = posicion;
            actual->cantidad_posiciones++;
            return 1;
        }

        actual = actual->siguiente;
    }

    nuevo = (NodoHash *) malloc(sizeof(NodoHash));
    if (nuevo == NULL) {
        return 0;
    }

    largo = strlen(clave);
    nuevo->clave = (char *) malloc(largo + 1);
    if (nuevo->clave == NULL) {
        free(nuevo);
        return 0;
    }

    memcpy(nuevo->clave, clave, largo + 1);
    nuevo->capacidad_posiciones = 2;
    nuevo->cantidad_posiciones = 1;
    nuevo->posiciones =
        (long long *) malloc(nuevo->capacidad_posiciones * sizeof(long long));

    if (nuevo->posiciones == NULL) {
        free(nuevo->clave);
        free(nuevo);
        return 0;
    }

    nuevo->posiciones[0] = posicion;

    /* Encadenamiento: inserta al inicio de la lista de la cubeta. */
    nuevo->siguiente = tabla->cubetas[indice];
    tabla->cubetas[indice] = nuevo;

    return 1;
}
