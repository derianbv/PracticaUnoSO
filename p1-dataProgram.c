#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct columnasCSV
{
    char artist[256];
    char song[256];
    char text[4096];
    char length[16];
    char emotion[64];
    char genre[64];
    char album[256];
    char release_date[32];
    char key[16];
    int tempo;
};



int hash(char *str) {
    int hash = 0;
    while (*str) {
        hash = (hash * 31 + *str) % 1000; // Simple hash function
        str++;
    }
    return hash;
}

static int leer_campo_csv(char **cursor, char *destino, size_t tamDestino) {
    size_t i = 0;
    char *p = *cursor;

    if (p == NULL || *p == '\0') {
        if (tamDestino > 0) {
            destino[0] = '\0';
        }
        return 0;
    }

    if (*p == '"') {
        p++;
        while (*p) {
            if (*p == '"' && *(p + 1) == '"') {
                if (i + 1 < tamDestino) {
                    destino[i++] = '"';
                }
                p += 2;
                continue;
            }

            if (*p == '"') {
                p++;
                break;
            }

            if (i + 1 < tamDestino) {
                destino[i++] = *p;
            }
            p++;
        }
    } else {
        while (*p && *p != ',') {
            if (i + 1 < tamDestino) {
                destino[i++] = *p;
            }
            p++;
        }
    }

    destino[i] = '\0';

    while (*p && *p != ',') {
        p++;
    }
    if (*p == ',') {
        p++;
    }

    *cursor = p;
    return 1;
}





int main() {

    printf("Analizando archivos y crenadolos en memoria\n"); 

    FILE *ptrArchivo;
    ptrArchivo = fopen("spotify_dataset.csv", "r");
    if (ptrArchivo == NULL) {
        printf("Error al abrir el archivo\n");
        return 1;
    }

    char linea[16384];
    if (fgets(linea, sizeof(linea), ptrArchivo) == NULL) {
        printf("No se pudo leer el encabezado del CSV\n");
        fclose(ptrArchivo);
        return 1;
    }
    linea[strcspn(linea, "\r\n")] = '\0';

    struct columnasCSV primeraFila = {0};
    char tempoTexto[32] = {0};
    char *cursor = linea;

    leer_campo_csv(&cursor, primeraFila.artist, sizeof(primeraFila.artist));
    leer_campo_csv(&cursor, primeraFila.song, sizeof(primeraFila.song));
    leer_campo_csv(&cursor, primeraFila.text, sizeof(primeraFila.text));
    leer_campo_csv(&cursor, primeraFila.length, sizeof(primeraFila.length));
    leer_campo_csv(&cursor, primeraFila.emotion, sizeof(primeraFila.emotion));
    leer_campo_csv(&cursor, primeraFila.genre, sizeof(primeraFila.genre));
    leer_campo_csv(&cursor, primeraFila.album, sizeof(primeraFila.album));
    leer_campo_csv(&cursor, primeraFila.release_date, sizeof(primeraFila.release_date));
    leer_campo_csv(&cursor, primeraFila.key, sizeof(primeraFila.key));
    leer_campo_csv(&cursor, tempoTexto, sizeof(tempoTexto));
    primeraFila.tempo = atoi(tempoTexto);

    printf("Primera fila leida:\n");
    printf("Artist: %s\n", primeraFila.artist);
    printf("Song: %s\n", primeraFila.song);
    printf("Emotion: %s\n", primeraFila.emotion);
    printf("Genre: %s\n", primeraFila.genre);
    printf("Tempo: %d\n", primeraFila.tempo);

    fclose(ptrArchivo);



    return 0;
}