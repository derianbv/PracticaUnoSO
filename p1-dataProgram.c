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
#define MAX_COINCIDENCIAS_MOSTRADAS 100

typedef struct RespuestaCanal {
    int estado;
    int tipoBusqueda;
    int totalResultados;
    char terminoBusqueda[MAX_CHARS];
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

static void extraer_columna_csv(const char *linea, int indiceColumna,
                                char *salida, size_t tamSalida)
{
    int columnaActual = 0;
    int dentroComillas = 0;
    size_t i = 0;
    size_t usadosSalida = 0;

    if (salida == NULL || tamSalida == 0) {
        return;
    }

    salida[0] = '\0';

    while (linea[i] != '\0' && linea[i] != '\n') {
        char c = linea[i];

        if (c == '"') {
            dentroComillas = !dentroComillas;
            i++;
            continue;
        }

        if (c == ',' && !dentroComillas) {
            columnaActual++;
            i++;
            continue;
        }

        if (columnaActual == indiceColumna && (usadosSalida + 1) < tamSalida) {
            salida[usadosSalida] = c;
            usadosSalida++;
            salida[usadosSalida] = '\0';
        }

        i++;
    }
}

static void recortar_espacios(char *texto)
{
    size_t inicio = 0;
    size_t fin;
    size_t i = 0;

    if (texto == NULL || texto[0] == '\0') {
        return;
    }

    while (texto[inicio] == ' ' || texto[inicio] == '\t') {
        inicio++;
    }

    fin = strlen(texto);
    while (fin > inicio && (texto[fin - 1] == ' ' || texto[fin - 1] == '\t')) {
        fin--;
    }

    while (inicio < fin) {
        texto[i++] = texto[inicio++];
    }
    texto[i] = '\0';
}

static void insertar_generos_en_hash(TablaHash *tabla,
                                     const char *cadenaGeneros,
                                     long long posicion)
{
    char token[MAX_CHARS];
    size_t i = 0;
    size_t j = 0;

    if (tabla == NULL || cadenaGeneros == NULL) {
        return;
    }

    while (1) {
        char c = cadenaGeneros[i];

        if (c == ',' || c == '\0') {
            token[j] = '\0';
            recortar_espacios(token);
            if (token[0] != '\0') {
                insertar_en_tabla_hash(tabla, token, posicion);
            }
            j = 0;
            if (c == '\0') {
                break;
            }
            i++;
            continue;
        }

        if (j < (MAX_CHARS - 1)) {
            token[j++] = c;
        }
        i++;
    }
}

int main(void) {
    // 1. Estructura de indice en hash
    TablaHash *ptrTablaHashArtistas = crear_tabla_hash(10007);
    TablaHash *ptrTablaHashGeneros = crear_tabla_hash(10007);
    int idMemoriaCompartida;
    int idMemoriaVieja;
    RespuestaCanal *ptrCanalRespuesta;

    if (ptrTablaHashArtistas == NULL || ptrTablaHashGeneros == NULL) {
        printf("Error: No se pudo crear la tabla hash.\n");
        liberar_tabla_hash(ptrTablaHashArtistas);
        liberar_tabla_hash(ptrTablaHashGeneros);
        return 1;
    }

    FILE *ptrArchivoDataset = fopen("spotify_dataset.csv", "rb");
    if (!ptrArchivoDataset) {
        printf("Error: No se encontró el archivo spotify_dataset.csv\n");
        liberar_tabla_hash(ptrTablaHashArtistas);
        liberar_tabla_hash(ptrTablaHashGeneros);
        return 1;
    }

    char bufferLineaCSV[15000]; // Buffer extra grande para las letras de canciones
    // Saltar la línea de cabecera
    if (!fgets(bufferLineaCSV, sizeof(bufferLineaCSV), ptrArchivoDataset)) {
        printf("Archivo vacío.\n");
        fclose(ptrArchivoDataset);
        liberar_tabla_hash(ptrTablaHashArtistas);
        liberar_tabla_hash(ptrTablaHashGeneros);
        return 1;
    }

    // 2. Lectura y guardado
    while (1) {
        long long posicionByte = ftell(ptrArchivoDataset);
        char bufferArtista[MAX_CHARS];
        char bufferGeneros[1024];

        if (posicionByte < 0) {
            break;
        }

        if (!fgets(bufferLineaCSV, sizeof(bufferLineaCSV), ptrArchivoDataset)) {
            break;
        }

        extraer_columna_csv(bufferLineaCSV, 0, bufferArtista, sizeof(bufferArtista));
        extraer_columna_csv(bufferLineaCSV, 5, bufferGeneros, sizeof(bufferGeneros));
        recortar_espacios(bufferArtista);

        if (bufferArtista[0] != '\0' &&
            !insertar_en_tabla_hash(ptrTablaHashArtistas, bufferArtista, posicionByte)) {
            break;
        }

        insertar_generos_en_hash(ptrTablaHashGeneros, bufferGeneros, posicionByte);
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
        liberar_tabla_hash(ptrTablaHashGeneros);
        return 1;
    }

    ptrCanalRespuesta = (RespuestaCanal *) shmat(idMemoriaCompartida, NULL, 0);
    if (ptrCanalRespuesta == (void *) -1) {
        printf("Error: No se pudo mapear memoria compartida.\n");
        fclose(ptrArchivoDataset);
        liberar_tabla_hash(ptrTablaHashArtistas);
        liberar_tabla_hash(ptrTablaHashGeneros);
        return 1;
    }

    ptrCanalRespuesta->estado = ESTADO_LIBRE;
    ptrCanalRespuesta->tipoBusqueda = 0;
    ptrCanalRespuesta->totalResultados = 0;
    ptrCanalRespuesta->terminoBusqueda[0] = '\0';
    ptrCanalRespuesta->textoRespuesta[0] = '\0';

    printf("Servidor listo. Esperando consultas por memoria compartida...\n");

    while (1) {
        if (ptrCanalRespuesta->estado == ESTADO_PETICION) {
            unsigned int indice;
            NodoHash *actual;
            TablaHash *tablaBusqueda;
            const char *etiquetaBusqueda;
            size_t usados = 0;
            int total = 0;
            int mostrados = 0;

            ptrCanalRespuesta->textoRespuesta[0] = '\0';
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         "=== RESULTADOS DE BUSQUEDA ===\n");
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         "");

            if (ptrCanalRespuesta->tipoBusqueda == 1) {
                tablaBusqueda = ptrTablaHashArtistas;
                etiquetaBusqueda = "Artista solicitado: ";
            }
            else if (ptrCanalRespuesta->tipoBusqueda == 2) {
                tablaBusqueda = ptrTablaHashGeneros;
                etiquetaBusqueda = "Genero solicitado: ";
            }
            else {
                ptrCanalRespuesta->textoRespuesta[0] = '\0';
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usados,
                             "NA");
                ptrCanalRespuesta->totalResultados = 0;
                ptrCanalRespuesta->estado = ESTADO_RESPUESTA;
                sleep(1);
                continue;
            }

            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         etiquetaBusqueda);
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         ptrCanalRespuesta->terminoBusqueda);
            append_texto(ptrCanalRespuesta->textoRespuesta,
                         TAMANO_BUFFER_RESPUESTA,
                         &usados,
                         "\n\n");

            indice = obtener_indice_hash(ptrCanalRespuesta->terminoBusqueda,
                                         tablaBusqueda->capacidad);
            actual = tablaBusqueda->cubetas[indice];

            while (actual != NULL) {
                if (strcmp(actual->clave, ptrCanalRespuesta->terminoBusqueda) == 0) {
                    size_t p;
                    total = (int) actual->cantidad_posiciones;

                    for (p = 0; p < actual->cantidad_posiciones; p++) {
                        char lineaCsv[15000];
                        size_t usadosLinea = 0;
                        char c;

                        if (mostrados >= MAX_COINCIDENCIAS_MOSTRADAS) {
                            break;
                        }

                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "---- Coincidencia ----\n");

                        lineaCsv[0] = '\0';
                        if (fseek(ptrArchivoDataset, actual->posiciones[p], SEEK_SET) == 0) {
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
                        }
                        lineaCsv[usadosLinea] = '\0';

                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     lineaCsv);
                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "\n\n");
                        mostrados++;
                    }

                    break;
                }
                actual = actual->siguiente;
            }

            ptrCanalRespuesta->totalResultados = total;
            if (total > mostrados) {
                char resumen[64];
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usados,
                             "\nMostrados: ");
                snprintf(resumen, sizeof(resumen), "%d | Total: %d", mostrados, total);
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usados,
                             resumen);
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usados,
                             "\n");
            }

            if (total == 0) {
                size_t usadosMensaje = 0;
                ptrCanalRespuesta->textoRespuesta[0] = '\0';
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usadosMensaje,
                             "NA");
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
    liberar_tabla_hash(ptrTablaHashGeneros);




    return 0;
}