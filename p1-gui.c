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

#define RESPUESTA_TAM 65536
#define OFFSET_ESTADO 0
#define OFFSET_TOTAL (OFFSET_ESTADO + (int) sizeof(int))
#define OFFSET_ARTISTA (OFFSET_TOTAL + (int) sizeof(int))
#define OFFSET_RESPUESTA (OFFSET_ARTISTA + MAX_CHARS)
#define SHM_TAM (OFFSET_RESPUESTA + RESPUESTA_TAM)

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
    int opcion;
    int shmid;
    char *memoria;
    int *estado;
    int *total_compartido;
    char *artista_compartido;
    char *respuesta_compartida;

    shmid = shmget(SHM_KEY, SHM_TAM, 0666);
    if (shmid < 0) {
        printf("Error: no se encontro la memoria compartida.\n");
        printf("Ejecuta primero ./p1-dataProgram\n");
        return 1;
    }

    memoria = (char *) shmat(shmid, NULL, 0);
    if (memoria == (void *) -1) {
        printf("Error: no se pudo mapear la memoria compartida.\n");
        return 1;
    }

    estado = (int *) (void *) (memoria + OFFSET_ESTADO);
    total_compartido = (int *) (void *) (memoria + OFFSET_TOTAL);
    artista_compartido = memoria + OFFSET_ARTISTA;
    respuesta_compartida = memoria + OFFSET_RESPUESTA;

    while (1) {
        char artista[MAX_CHARS];

        printf("\n=== MENU CONSULTAS ARTISTA ===\n");
        printf("1. Buscar artista\n");
        printf("2. Salir\n");
        printf("Opcion: ");

        if (scanf("%d", &opcion) != 1) {
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

        if (opcion == 1) {
            if (*estado != ESTADO_LIBRE) {
                printf("Servidor ocupado, intenta de nuevo.\n");
                continue;
            }

            printf("Ingresa nombre del artista: ");
            leer_linea(artista, sizeof(artista));

            if (artista[0] == '\0') {
                printf("Nombre vacio.\n");
                continue;
            }

            strncpy(artista_compartido, artista, MAX_CHARS - 1);
            artista_compartido[MAX_CHARS - 1] = '\0';
            *estado = ESTADO_PETICION;

            while (*estado != ESTADO_RESPUESTA) {
                sleep(1);
            }

            printf("\n%s\n", respuesta_compartida);
            printf("Total coincidencias: %d\n", *total_compartido);

            *estado = ESTADO_LIBRE;
        }
        else if (opcion == 2) {
            *estado = ESTADO_SALIR;
            break;
        }
        else {
            printf("Opcion invalida.\n");
        }
    }

    shmdt(memoria);
    return 0;
}
