#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "hash_util.c"

#define MAX_CHARS 256 //maximo largo de los strings
#define SHM_KEY 0x44504131

//variables del semaforo
#define ESTADO_LIBRE 0
#define ESTADO_PETICION 1
#define ESTADO_RESPUESTA 2
#define ESTADO_SALIR 3

#define TAMANO_BUFFER_RESPUESTA 262144
#define MAX_COINCIDENCIAS_MOSTRADAS 100
#define CAPACIDAD_TABLA_HASH 150001

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

static size_t calcular_memoria_tabla_hash(const TablaHash *tabla)
{
    size_t total = 0;
    unsigned int i;

    if (tabla == NULL) {
        return 0;
    }

    total += sizeof(TablaHash);
    total += (size_t) tabla->capacidad * sizeof(NodoHash *);

    for (i = 0; i < tabla->capacidad; i++) {
        const NodoHash *actual = tabla->cubetas[i];

        while (actual != NULL) {
            total += sizeof(NodoHash);
            total += strlen(actual->key) + 1;
            total += actual->capacidad_posiciones * sizeof(long long);
            actual = actual->siguiente;
        }
    }

    return total;
}

static TablaHash *construir_indice_desde_csv(FILE *ptrArchivoDataset,
                                             int tipoBusqueda)
{
    TablaHash *tabla;
    char bufferLineaCSV[15000];

    if (ptrArchivoDataset == NULL) {
        return NULL;
    }

    tabla = crear_tabla_hash(CAPACIDAD_TABLA_HASH);
    if (tabla == NULL) {
        return NULL;
    }

    if (fseek(ptrArchivoDataset, 0, SEEK_SET) != 0) {
        liberar_tabla_hash(tabla);
        return NULL;
    }

    if (!fgets(bufferLineaCSV, sizeof(bufferLineaCSV), ptrArchivoDataset)) {
        liberar_tabla_hash(tabla);
        return NULL;
    }

    while (1) {
        long long posicionByte = ftell(ptrArchivoDataset);

        if (posicionByte < 0) {
            break;
        }

        if (!fgets(bufferLineaCSV, sizeof(bufferLineaCSV), ptrArchivoDataset)) {
            break;
        }

        if (tipoBusqueda == 1) {
            char bufferArtista[MAX_CHARS];

            extraer_columna_csv(bufferLineaCSV, 0, bufferArtista,
                                sizeof(bufferArtista));
            recortar_espacios(bufferArtista);

            if (bufferArtista[0] != '\0' &&
                !insertar_en_tabla_hash(tabla, bufferArtista, posicionByte)) {
                liberar_tabla_hash(tabla);
                return NULL;
            }
        }
        else if (tipoBusqueda == 2) {
            char bufferGeneros[1024];

            extraer_columna_csv(bufferLineaCSV, 5, bufferGeneros,
                                sizeof(bufferGeneros));
            insertar_generos_en_hash(tabla, bufferGeneros, posicionByte);
        }
    }

    return tabla;
}

int main(void) {
    // 1. Estructura de indice en hash
    TablaHash *ptrTablaHashArtistas = NULL;
    TablaHash *ptrTablaHashGeneros = NULL;
    int idMemoriaCompartida;
    int idMemoriaVieja;
    RespuestaCanal *ptrCanalRespuesta;

    FILE *ptrArchivoDataset = fopen("spotify_dataset.csv", "rb");
    if (!ptrArchivoDataset) {
        printf("Error: No se encontró el archivo spotify_dataset.csv\n");
        liberar_tabla_hash(ptrTablaHashArtistas);
        liberar_tabla_hash(ptrTablaHashGeneros);
        return 1;
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
                if (ptrTablaHashArtistas == NULL) {
                    size_t memoriaArtistas;

                    ptrTablaHashArtistas = construir_indice_desde_csv(ptrArchivoDataset, 1);
                    if (ptrTablaHashArtistas == NULL) {
                        ptrCanalRespuesta->textoRespuesta[0] = '\0';
                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "Error construyendo indice de artistas");
                        ptrCanalRespuesta->totalResultados = 0;
                        ptrCanalRespuesta->estado = ESTADO_RESPUESTA;
                        sleep(1);
                        continue;
                    }

                    memoriaArtistas = calcular_memoria_tabla_hash(ptrTablaHashArtistas);

                    printf("Memoria hash artistas: %zu bytes (%.2f MB)\n",
                           memoriaArtistas,
                           (double) memoriaArtistas / (1024.0 * 1024.0));
                }
                tablaBusqueda = ptrTablaHashArtistas;
                etiquetaBusqueda = "Artista solicitado: ";
            }
            else if (ptrCanalRespuesta->tipoBusqueda == 2) {
                if (ptrTablaHashGeneros == NULL) {
                    size_t memoriaGeneros;

                    ptrTablaHashGeneros = construir_indice_desde_csv(ptrArchivoDataset, 2);
                    if (ptrTablaHashGeneros == NULL) {
                        ptrCanalRespuesta->textoRespuesta[0] = '\0';
                        append_texto(ptrCanalRespuesta->textoRespuesta,
                                     TAMANO_BUFFER_RESPUESTA,
                                     &usados,
                                     "Error construyendo indice de generos");
                        ptrCanalRespuesta->totalResultados = 0;
                        ptrCanalRespuesta->estado = ESTADO_RESPUESTA;
                        sleep(1);
                        continue;
                    }

                    memoriaGeneros = calcular_memoria_tabla_hash(ptrTablaHashGeneros);

                    printf("Memoria hash generos: %zu bytes (%.2f MB)\n",
                           memoriaGeneros,
                           (double) memoriaGeneros / (1024.0 * 1024.0));
                }
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

            if (ptrCanalRespuesta->terminoBusqueda[0] == '\0') {
                ptrCanalRespuesta->textoRespuesta[0] = '\0';
                append_texto(ptrCanalRespuesta->textoRespuesta,
                             TAMANO_BUFFER_RESPUESTA,
                             &usados,
                             "INDICE_LISTO");
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

            indice = hashARam(ptrCanalRespuesta->terminoBusqueda,
                              tablaBusqueda->capacidad);
            actual = tablaBusqueda->cubetas[indice];

            while (actual != NULL) {
                if (strcmp(actual->key, ptrCanalRespuesta->terminoBusqueda) == 0) {
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
                        if (fseek(ptrArchivoDataset, actual->posicionesBytes[p], SEEK_SET) == 0) {
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
