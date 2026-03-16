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
    NodoHash *nuevo;
    size_t largo;

    if (tabla == NULL || clave == NULL || clave[0] == '\0') {
        return 0;
    }

    indice = obtener_indice_hash(clave, tabla->capacidad);

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
    nuevo->posicion = posicion;

    /* Encadenamiento: inserta al inicio de la lista de la cubeta. */
    nuevo->siguiente = tabla->cubetas[indice];
    tabla->cubetas[indice] = nuevo;

    return 1;
}

int contar_coincidencias_hash(const TablaHash *tabla, const char *clave)
{
    unsigned int indice;
    int total = 0;
    NodoHash *actual;

    if (tabla == NULL || clave == NULL || clave[0] == '\0') {
        return 0;
    }

    indice = obtener_indice_hash(clave, tabla->capacidad);
    actual = tabla->cubetas[indice];

    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            total++;
        }
        actual = actual->siguiente;
    }

    return total;
}

int imprimir_coincidencias_hash(const TablaHash *tabla, const char *clave)
{
    unsigned int indice;
    NodoHash *actual;
    int primero = 1;
    int total = 0;

    if (tabla == NULL || clave == NULL || clave[0] == '\0') {
        return 0;
    }

    indice = obtener_indice_hash(clave, tabla->capacidad);
    actual = tabla->cubetas[indice];

    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            if (primero) {
                printf("%s:", clave);
                primero = 0;
            }
            else {
                printf(",");
            }

            printf("%lld", actual->posicion);
            total++;
        }

        actual = actual->siguiente;
    }

    if (!primero) {
        printf("\n");
    }

    return total;
}

int escribir_coincidencias_hash(const TablaHash *tabla, const char *clave,
                                char *buffer, size_t buffer_tamano)
{
    unsigned int indice;
    NodoHash *actual;
    int total = 0;
    size_t usados;

    if (buffer == NULL || buffer_tamano == 0) {
        return 0;
    }

    buffer[0] = '\0';

    if (tabla == NULL || clave == NULL || clave[0] == '\0') {
        return 0;
    }

    usados = (size_t) snprintf(buffer, buffer_tamano, "%s:", clave);
    if (usados >= buffer_tamano) {
        buffer[buffer_tamano - 1] = '\0';
        return 0;
    }

    indice = obtener_indice_hash(clave, tabla->capacidad);
    actual = tabla->cubetas[indice];

    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            int escritos;

            escritos = snprintf(buffer + usados, buffer_tamano - usados,
                                "%s%lld", (total == 0) ? "" : ",",
                                actual->posicion);
            if (escritos < 0) {
                break;
            }

            if ((size_t) escritos >= (buffer_tamano - usados)) {
                usados = buffer_tamano - 1;
                buffer[usados] = '\0';
                total++;
                break;
            }

            usados += (size_t) escritos;
            total++;
        }

        actual = actual->siguiente;
    }

    if (total == 0) {
        buffer[0] = '\0';
    }

    return total;
}

void imprimir_todo_el_indice(const TablaHash *tabla)
{
    unsigned int i;

    if (tabla == NULL) {
        return;
    }

    for (i = 0; i < tabla->capacidad; i++) {
        NodoHash *nodo = tabla->cubetas[i];

        while (nodo != NULL) {
            int es_primera_vez = 1;
            NodoHash *anterior = tabla->cubetas[i];
            NodoHash *it = tabla->cubetas[i];
            int primero = 1;

            while (anterior != nodo) {
                if (strcmp(anterior->clave, nodo->clave) == 0) {
                    es_primera_vez = 0;
                    break;
                }
                anterior = anterior->siguiente;
            }

            if (es_primera_vez) {
                printf("%s:", nodo->clave);

                while (it != NULL) {
                    if (strcmp(it->clave, nodo->clave) == 0) {
                        if (!primero) {
                            printf(",");
                        }
                        printf("%lld", it->posicion);
                        primero = 0;
                    }
                    it = it->siguiente;
                }

                printf("\n");
            }

            nodo = nodo->siguiente;
        }
    }
}
