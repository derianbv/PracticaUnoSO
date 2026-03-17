#include <stddef.h> //stddef.h es necesario para size_t y NULL  
#include <stdint.h>  //stdint.h es necesario para uint64_t usado por el hash 
#include <stdlib.h>  //stdlib.h es necesario para malloc, free, realloc 
#include <string.h> //string.h es necesario para strlen, strcmp, memcpy

typedef struct NodoHash {
    char *key; //la key, en este caso el nombre del artista o genero EN ESPECIFICO y luego se pasa por la fn hash
    long long *posicionesBytes; //puntero a un arreglo dinámico de posiciones en el archivo CSV donde se encuentra el artista o genero
    size_t cantidad_posiciones; // cantidad de posiciones almacenadas en el arreglo
    size_t capacidad_posiciones; // capacidad actual del arreglo de posiciones, se duplica cuando se alcanza el límite
    struct NodoHash *siguiente; //puntero al siguiente nodo en caso de colisiones (encadenamiento)
} NodoHash;

typedef struct TablaHash {
    unsigned int capacidad;
    NodoHash **cubetas;
} TablaHash;

unsigned long long calcular_hash_fnv1a(const char *texto)
{
    uint64_t semilla = 1469598103934665603ULL;

    while (*texto != '\0') {
        semilla ^= (unsigned char) *texto; //cast a positivo para XOR con las semilla 
        semilla *= 1099511628211ULL;
        texto++;
    }

    return (unsigned long long) semilla;
}

unsigned int hashARam(const char *texto, unsigned int capacidad)
{
    if (capacidad == 0) {
        return 0;
    }

    return (unsigned int) (calcular_hash_fnv1a(texto) % capacidad);
}

TablaHash *crear_tabla_hash(unsigned int capacidad){ //* en una funcion significa que retorna la direccion de memoria de la tabla hash se devuelve
    TablaHash *tabla;

    if (capacidad == 0) {
        return NULL;
    }

    tabla = (TablaHash *) malloc(sizeof(TablaHash));
    if (tabla == NULL) {
        return NULL;
    }

    tabla->cubetas = (NodoHash **) calloc(capacidad, sizeof(NodoHash *)); //calloc daja vacia los espacios en memoria, se asigna un arreglo de punteros a NodoHash con tamaño igual a la capacidad de la tabla
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
            free(actual->key);
            free(actual->posicionesBytes);
            free(actual);
            actual = siguiente;
        }
    }

    free(tabla->cubetas);
    free(tabla);
}

int insertar_en_tabla_hash(TablaHash *tabla, const char *key, long long posicion)
{
    unsigned int indice;
    NodoHash *actual;
    NodoHash *nuevo;
    size_t largo;

    if (tabla == NULL || key == NULL || key[0] == '\0') {
        return 0;
    }

    indice = hashARam(key, tabla->capacidad);
    actual = tabla->cubetas[indice];

    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            if (actual->cantidad_posiciones == actual->capacidad_posiciones) {
                size_t nueva_capacidad = actual->capacidad_posiciones * 2;
                long long *nuevas_posiciones =
                    (long long *) realloc(actual->posicionesBytes,
                                          nueva_capacidad * sizeof(long long));

                if (nuevas_posiciones == NULL) {
                    return 0;
                }

                actual->posicionesBytes = nuevas_posiciones;
                actual->capacidad_posiciones = nueva_capacidad;
            }

            actual->posicionesBytes[actual->cantidad_posiciones] = posicion;
            actual->cantidad_posiciones++;
            return 1;
        }

        actual = actual->siguiente;
    }

    nuevo = (NodoHash *) malloc(sizeof(NodoHash));
    if (nuevo == NULL) {
        return 0;
    }

    largo = strlen(key);
    nuevo->key = (char *) malloc(largo + 1);
    if (nuevo->key == NULL) {
        free(nuevo);
        return 0;
    }

    memcpy(nuevo->key, key, largo + 1);
    nuevo->capacidad_posiciones = 2;
    nuevo->cantidad_posiciones = 1;
    nuevo->posicionesBytes =
        (long long *) malloc(nuevo->capacidad_posiciones * sizeof(long long));

    if (nuevo->posicionesBytes == NULL) {
        free(nuevo->key);
        free(nuevo);
        return 0;
    }

    nuevo->posicionesBytes[0] = posicion;

    /* Encadenamiento: inserta al inicio de la lista de la cubeta. */
    nuevo->siguiente = tabla->cubetas[indice];
    tabla->cubetas[indice] = nuevo;

    return 1;
}
