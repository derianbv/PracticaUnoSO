#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_util.h"

#define MAX_FILAS 500000
#define MAX_CHARS 256

int main() {
    // 1. Estructura de indice en hash
    TablaHash *tabla_artistas = crear_tabla_hash(10007);

    if (tabla_artistas == NULL) {
        printf("Error: No se pudo crear la tabla hash.\n");
        return 1;
    }

    FILE *archivo = fopen("spotify_dataset.csv", "rb");
    if (!archivo) {
        printf("Error: No se encontró el archivo spotify_dataset.csv\n");
        liberar_tabla_hash(tabla_artistas);
        return 1;
    }

    char linea[15000]; // Buffer extra grande para las letras de canciones
    int fila_actual = 0;

    // Saltar la línea de cabecera
    if (!fgets(linea, sizeof(linea), archivo)) {
        printf("Archivo vacío.\n");
        fclose(archivo);
        liberar_tabla_hash(tabla_artistas);
        return 1;
    }

    // 2. Lectura y guardado
    while (fila_actual < MAX_FILAS) {
        long long posicion_byte = ftell(archivo);
        char artista[MAX_CHARS];
        int i = 0, j = 0, en_comillas = 0;

        if (posicion_byte < 0) {
            break;
        }

        if (!fgets(linea, sizeof(linea), archivo)) {
            break;
        }

        // Procesar solo hasta la primera coma (fuera de comillas)
        while (linea[i] != '\0' && linea[i] != '\n') {
            if (linea[i] == '"') {
                en_comillas = !en_comillas;
            } 
            else if (linea[i] == ',' && !en_comillas) {
                break; 
            } 
            else if (j < MAX_CHARS - 1) {
                artista[j++] = linea[i];
            }
            i++;
        }
        artista[j] = '\0'; // Cierre de la cadena

        if (!insertar_en_tabla_hash(tabla_artistas, artista, posicion_byte)) {
            break;
        }

        fila_actual++;
    }

    // 3. Salida final solicitada: artista:byte1,byte2,...
    imprimir_todo_el_indice(tabla_artistas);

    // 4. Limpieza
    fclose(archivo);
    liberar_tabla_hash(tabla_artistas);

    return 0;
}