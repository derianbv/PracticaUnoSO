#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "hash_util.h"

#define MAX_CHARS 256
#define SHM_KEY 0x44504131

//variables del semaforo
#define ESTADO_LIBRE 0
#define ESTADO_PETICION 1
#define ESTADO_RESPUESTA 2
#define ESTADO_SALIR 3

#define TAMANO_BUFFER_RESPUESTA 262144

typedef struct RespuestaCanal {
    int estado;
    int totalResultados;
    char nombreArtista[MAX_CHARS];
    char textoRespuesta[TAMANO_BUFFER_RESPUESTA];
} RespuestaCanal;

static void append_texto(char *destino, size_t capacidad, size_t *usados,
                         const char *texto)
{
    size_t i = 0;

    if (destino == NULL || usados == NULL || texto == NULL || capacidad == 0) {
        return;
    }

    while (texto[i] != '\0' && (*usados + 1) < capacidad) {
        destino[*usados] = texto[i];
        (*usados)++;
        i++;
    }

    destino[*usados] = '\0';
}

static void append_entero_ll(char *destino, size_t capacidad, size_t *usados,
                             long long valor)
{
    char temporal[32];
    int i = 0;
    unsigned long long numero;

    if (destino == NULL || usados == NULL || capacidad == 0) {
        return;
    }

    if (valor < 0) {
        append_texto(destino, capacidad, usados, "-");
        numero = (unsigned long long) (-(valor + 1)) + 1ULL;
    }
    else {
        numero = (unsigned long long) valor;
    }

    if (numero == 0ULL) {
        append_texto(destino, capacidad, usados, "0");
        return;
    }

    while (numero > 0ULL && i < (int) sizeof(temporal)) {
        temporal[i++] = (char) ('0' + (numero % 10ULL));
        numero /= 10ULL;
    }

    while (i > 0) {
        i--;
        if ((*usados + 1) >= capacidad) {
            break;
        }
        destino[*usados] = temporal[i];
        (*usados)++;
    }

    destino[*usados] = '\0';
}

static void append_entero(char *destino, size_t capacidad, size_t *usados,
                          int valor)
{
    append_entero_ll(destino, capacidad, usados, (long long) valor);
}

int main(void) {
    // 1. Estructura de indice en hash
    TablaHash *ptrTablaHashArtistas = crear_tabla_hash(10007);
    int idMemoriaCompartida;
    int idMemoriaVieja;
    RespuestaCanal *ptrCanalRespuesta;

    if (ptrTablaHashArtistas == NULL) {
        printf("Error: No se pudo crear la tabla hash.\n");
        return 1;
    }

    FILE *ptrArchivoDataset = fopen("spotify_dataset.csv", "rb");
    if (!ptrArchivoDataset) {
        printf("Error: No se encontró el archivo spotify_dataset.csv\n");
        liberar_tabla_hash(ptrTablaHashArtistas);
        return 1;
    }

    char bufferLineaCSV[15000]; // Buffer extra grande para las letras de canciones
    // Saltar la línea de cabecera
    if (!fgets(bufferLineaCSV, sizeof(bufferLineaCSV), ptrArchivoDataset)) {
        printf("Archivo vacío.\n");
        fclose(ptrArchivoDataset);
        liberar_tabla_hash(ptrTablaHashArtistas);
        return 1;
    }

    // 2. Lectura y guardado
    while (1) {
        long long posicionByte = ftell(ptrArchivoDataset);
        char bufferArtista[MAX_CHARS];
        int idxLinea = 0;
        int idxArtista = 0;
        int dentroComillas = 0;

        if (posicionByte < 0) {
            break;
        }

        if (!fgets(bufferLineaCSV, sizeof(bufferLineaCSV), ptrArchivoDataset)) {
            break;
        }

        // Procesar solo hasta la primera coma (fuera de comillas)
        while (bufferLineaCSV[idxLinea] != '\0' && bufferLineaCSV[idxLinea] != '\n') {
            if (bufferLineaCSV[idxLinea] == '"') {
                dentroComillas = !dentroComillas;
            } 
            else if (bufferLineaCSV[idxLinea] == ',' && !dentroComillas) {
                break; 
            } 
            else if (idxArtista < MAX_CHARS - 1) {
                bufferArtista[idxArtista++] = bufferLineaCSV[idxLinea];
            }
            idxLinea++;
        }
        bufferArtista[idxArtista] = '\0'; // Cierre de la cadena

        if (!insertar_en_tabla_hash(ptrTablaHashArtistas, bufferArtista, posicionByte)) {
            break;
        }
    }

    idMemoriaVieja = shmget(SHM_KEY, 1, 0666);
    if (idMemoriaVieja >= 0) {
        shmctl(idMemoriaVieja, IPC_RMID, NULL);
    }

    idMemoriaCompartida = shmget(SHM_KEY, sizeof(RespuestaCanal), IPC_CREAT | 0666);
    if (idMemoriaCompartida < 0) {
        printf("Error: No se pudo crear/abrir memoria compartida.\n");
        fclose(ptrArchivoDataset);
        liberar_tabla_hash(ptrTablaHashArtistas);
        return 1;
    }

    ptrCanalRespuesta = (RespuestaCanal *) shmat(idMemoriaCompartida, NULL, 0);
    if (ptrCanalRespuesta == (void *) -1) {
        printf("Error: No se pudo mapear memoria compartida.\n");
        fclose(ptrArchivoDataset);
        liberar_tabla_hash(ptrTablaHashArtistas);
        return 1;
    }

    ptrCanalRespuesta->estado = ESTADO_LIBRE;
    ptrCanalRespuesta->totalResultados = 0;
    ptrCanalRespuesta->nombreArtista[0] = '\0';
    ptrCanalRespuesta->textoRespuesta[0] = '\0';

    printf("Servidor listo. Esperando consultas por memoria compartida...\n");

    while (1) {
        if (ptrCanalRespuesta->estado == ESTADO_PETICION) {
            unsigned int indice;
            NodoHash *actual;
            size_t usados = 0;
            int total = 0;

            ptrCanalRespuesta->textoRespuesta[0] = '\0';
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         "=== RESULTADOS DE BUSQUEDA ===\n");
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         "Artista solicitado: ");
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         ptrCanalRespuesta->nombreArtista);
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         "\n\n");

            indice = obtener_indice_hash(ptrCanalRespuesta->nombreArtista,
                                         ptrTablaHashArtistas->capacidad);
            actual = ptrTablaHashArtistas->cubetas[indice];

            while (actual != NULL) {
                if (strcmp(actual->clave, ptrCanalRespuesta->nombreArtista) == 0) {
                    char lineaCsv[15000];
                    size_t usadosLinea = 0;
                    char c;

                    append_texto(ptrCanalRespuesta->textoRespuesta,
                                 TAMANO_BUFFER_RESPUESTA,
                                 &usados,
                                 "---- Coincidencia ");
                    append_entero(ptrCanalRespuesta->textoRespuesta,
                                  TAMANO_BUFFER_RESPUESTA,
                                  &usados,
                                  total + 1);
                    append_texto(ptrCanalRespuesta->textoRespuesta,
                                 TAMANO_BUFFER_RESPUESTA,
                                 &usados,
                                 " ----\nPosicion byte: ");
                    append_entero_ll(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     actual->posicion);
                    append_texto(ptrCanalRespuesta->textoRespuesta,
                                 TAMANO_BUFFER_RESPUESTA,
                                 &usados,
                                 "\n");

                    lineaCsv[0] = '\0';
                    if (fseek(ptrArchivoDataset, actual->posicion, SEEK_SET) == 0) {
                        while (fread(&c, 1, 1, ptrArchivoDataset) == 1) {
                            if (c == '\r') {
                                continue;
                            }
                            if (c == '\n') {
                                break;
                            }
                            if ((usadosLinea + 1) < sizeof(lineaCsv)) {
                                lineaCsv[usadosLinea] = c;
                                usadosLinea++;
                            }
                        }
                        lineaCsv[usadosLinea] = '\0';

                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "____________________________\n");
                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     lineaCsv);
                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "\n____________________________\n\n");
                    }
                    else {
                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "ERROR: no se pudo leer la fila en el CSV\n\n");
                    }

                    total++;
                    if ((usados + 1) >= TAMANO_BUFFER_RESPUESTA) {
                        break;
                    }
                }
                actual = actual->siguiente;
            }

            ptrCanalRespuesta->totalResultados = total;
            if (total == 0) {
                size_t usadosMensaje = 0;
                ptrCanalRespuesta->textoRespuesta[0] = '\0';
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usadosMensaje,
                             "No se encontraron coincidencias para '");
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usadosMensaje,
                             ptrCanalRespuesta->nombreArtista);
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usadosMensaje,
                             "'.");
            }

            ptrCanalRespuesta->estado = ESTADO_RESPUESTA;
        }
        else if (ptrCanalRespuesta->estado == ESTADO_SALIR) {
            break;
        }

        sleep(1);
    }


    // 4. Limpieza
    fclose(ptrArchivoDataset);
    shmdt(ptrCanalRespuesta);
    shmctl(idMemoriaCompartida, IPC_RMID, NULL);
    liberar_tabla_hash(ptrTablaHashArtistas);




    return 0;
}