#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define MAX_CHARS 256
#define SHM_KEY 0x44504131

#define ESTADO_LIBRE 0
#define ESTADO_PETICION 1
#define ESTADO_RESPUESTA 2
#define ESTADO_SALIR 3

#define TAMANO_BUFFER_RESPUESTA 262144

typedef struct RespuestaCanal {
    int estado;
    int tipoBusqueda;
    int totalResultados;
    char terminoBusqueda[MAX_CHARS];
    char textoRespuesta[TAMANO_BUFFER_RESPUESTA];
} RespuestaCanal;

static void leer_linea(char *buffer, size_t tamano)
{
    if (fgets(buffer, (int) tamano, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
}

int main(void)
{
    int opcionMenu;
    int idMemoriaCompartida;
    RespuestaCanal *ptrCanalRespuesta;

    idMemoriaCompartida = shmget(SHM_KEY, sizeof(RespuestaCanal), 0666);
    if (idMemoriaCompartida < 0) {
        printf("Error: no se encontro la memoria compartida.\n");
        printf("Ejecuta primero ./p1-dataProgram\n");
        return 1;
    }

    ptrCanalRespuesta = (RespuestaCanal *) shmat(idMemoriaCompartida, NULL, 0);
    if (ptrCanalRespuesta == (void *) -1) {
        printf("Error: no se pudo mapear la memoria compartida.\n");
        return 1;
    }

    while (1) {
        char bufferBusqueda[MAX_CHARS];

        printf("\n=== MENU ===\n");
        printf("1. Buscar artista\n");
        printf("2. Buscar genero\n");
        printf("3. Salir\n");
        printf("Opcion: ");

        if (scanf("%d", &opcionMenu) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
            printf("Opcion invalida.\n");
            continue;
        }

        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
        }

        if (opcionMenu == 1 || opcionMenu == 2) {
            if (ptrCanalRespuesta->estado != ESTADO_LIBRE) {
                printf("Servidor ocupado, intenta de nuevo.\n");
                continue;
            }

            ptrCanalRespuesta->tipoBusqueda = opcionMenu;
            ptrCanalRespuesta->terminoBusqueda[0] = '\0';
            ptrCanalRespuesta->estado = ESTADO_PETICION;

            while (ptrCanalRespuesta->estado != ESTADO_RESPUESTA) {
                sleep(1);
            }

            if (strncmp(ptrCanalRespuesta->textoRespuesta,
                        "Error",
                        strlen("Error")) == 0) {
                printf("%s\n", ptrCanalRespuesta->textoRespuesta);
                ptrCanalRespuesta->estado = ESTADO_LIBRE;
                continue;
            }

            ptrCanalRespuesta->estado = ESTADO_LIBRE;

            if (opcionMenu == 1) {
                printf("Ingresa nombre del artista: ");
            }
            else {
                printf("Ingresa genero musical: ");
            }

            leer_linea(bufferBusqueda, sizeof(bufferBusqueda));

            if (bufferBusqueda[0] == '\0') {
                printf("Nombre vacio.\n");
                continue;
            }

            if (ptrCanalRespuesta->estado != ESTADO_LIBRE) {
                printf("Servidor ocupado, intenta de nuevo.\n");
                continue;
            }

            ptrCanalRespuesta->tipoBusqueda = opcionMenu;
            strncpy(ptrCanalRespuesta->terminoBusqueda, bufferBusqueda,
                    MAX_CHARS - 1);
            ptrCanalRespuesta->terminoBusqueda[MAX_CHARS - 1] = '\0';
            ptrCanalRespuesta->estado = ESTADO_PETICION;

            while (ptrCanalRespuesta->estado != ESTADO_RESPUESTA) {
                sleep(1);
            }

            printf("\n%s\n", ptrCanalRespuesta->textoRespuesta);

            ptrCanalRespuesta->estado = ESTADO_LIBRE;
        }
        else if (opcionMenu == 3) {
            ptrCanalRespuesta->estado = ESTADO_SALIR;
            break;
        }
        else {
            printf("Opcion invalida.\n");
        }
    }

    shmdt(ptrCanalRespuesta);
    return 0;
}
