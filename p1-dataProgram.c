#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "hash_util.h"

#define MAX_FILAS 500000
#define MAX_CHARS 256
#define SHM_KEY 0x44504131

#define ESTADO_LIBRE 0
#define ESTADO_PETICION 1
#define ESTADO_RESPUESTA 2
#define ESTADO_SALIR 3

#define RESPUESTA_TAM 65536
#define OFFSET_ESTADO 0
#define OFFSET_TOTAL (OFFSET_ESTADO + (int) sizeof(int))
#define OFFSET_ARTISTA (OFFSET_TOTAL + (int) sizeof(int))
#define OFFSET_RESPUESTA (OFFSET_ARTISTA + MAX_CHARS)
#define SHM_TAM (OFFSET_RESPUESTA + RESPUESTA_TAM)

int main(void) {
    // 1. Estructura de indice en hash
    TablaHash *tabla_artistas = crear_tabla_hash(10007);
    int shmid;
    char *memoria;
    int *estado;
    int *total_compartido;
    char *artista_compartido;
    char *respuesta_compartida;

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

    shmid = shmget(SHM_KEY, SHM_TAM, IPC_CREAT | 0666);
    if (shmid < 0) {
        printf("Error: No se pudo crear/abrir memoria compartida.\n");
        fclose(archivo);
        liberar_tabla_hash(tabla_artistas);
        return 1;
    }

    memoria = (char *) shmat(shmid, NULL, 0);
    if (memoria == (void *) -1) {
        printf("Error: No se pudo mapear memoria compartida.\n");
        fclose(archivo);
        liberar_tabla_hash(tabla_artistas);
        return 1;
    }

    estado = (int *) (void *) (memoria + OFFSET_ESTADO);
    total_compartido = (int *) (void *) (memoria + OFFSET_TOTAL);
    artista_compartido = memoria + OFFSET_ARTISTA;
    respuesta_compartida = memoria + OFFSET_RESPUESTA;

    *estado = ESTADO_LIBRE;
    *total_compartido = 0;
    artista_compartido[0] = '\0';
    respuesta_compartida[0] = '\0';

    printf("Servidor listo. Esperando consultas por memoria compartida...\n");
    printf("Clave SHM: 0x%x\n", SHM_KEY);

    while (1) {
        if (*estado == ESTADO_PETICION) {
            int total = escribir_coincidencias_hash(tabla_artistas, artista_compartido,
                                                    respuesta_compartida,
                                                    RESPUESTA_TAM);

            *total_compartido = total;
            if (total == 0) {
                snprintf(respuesta_compartida, RESPUESTA_TAM,
                         "No se encontraron coincidencias para '%s'.",
                         artista_compartido);
            }

            *estado = ESTADO_RESPUESTA;
        }
        else if (*estado == ESTADO_SALIR) {
            break;
        }

        sleep(1);
    }


    // 4. Limpieza
    fclose(archivo);
    shmdt(memoria);
    shmctl(shmid, IPC_RMID, NULL);
    liberar_tabla_hash(tabla_artistas);




    return 0;
}