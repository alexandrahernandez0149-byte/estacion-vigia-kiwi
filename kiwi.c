#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UMBRAL_ALERTA 50.0f
#define TAM_COMANDO 24
#define TAM_ESTADO 16

typedef struct {
    float *lecturas;
    size_t capacidad;
    size_t cantidad;
} EstacionVigia;

typedef struct NodoEvento {
    int id;
    char tipo[24];
    float valor;
    struct NodoEvento *siguiente;
} NodoEvento;

typedef struct NodoComando {
    char comando[TAM_COMANDO];
    struct NodoComando *siguiente;
} NodoComando;

typedef struct {
    NodoComando *frente;
    NodoComando *final;
} ColaComandos;

typedef struct NodoHistorial {
    char comando[TAM_COMANDO];
    char estadoAnterior[TAM_ESTADO];
    char estadoNuevo[TAM_ESTADO];
    struct NodoHistorial *siguiente;
} NodoHistorial;

/* Prototipos */
void inicializarSistema(EstacionVigia *estacion, ColaComandos *cola,
                        NodoHistorial **pila, NodoEvento **eventos,
                        char estadoActuador[TAM_ESTADO], int *siguienteId);
int capturarLectura(EstacionVigia *estacion, NodoEvento **eventos,
                    float lectura, int *siguienteId);
int agregarEvento(NodoEvento **cabeza, int id, const char *tipo, float valor);
void mostrarLecturasYEstadisticas(const EstacionVigia *estacion);
void mostrarEventos(const NodoEvento *cabeza);
void buscarEventoPorTipo(const NodoEvento *cabeza);
void eliminarEvento(NodoEvento **cabeza);
void enviarComando(ColaComandos *cola);
void ejecutarComandos(ColaComandos *cola, NodoHistorial **pila,
                      char estadoActuador[TAM_ESTADO]);
void deshacerUltimoComando(NodoHistorial **pila,
                           char estadoActuador[TAM_ESTADO]);
void mostrarEstadoActuador(const char estadoActuador[TAM_ESTADO]);
void liberarTodaMemoria(EstacionVigia *estacion, NodoEvento **eventos,
                        ColaComandos *cola, NodoHistorial **pila);
void vaciarCola(ColaComandos *cola);
void vaciarEventos(NodoEvento **cabeza);
void vaciarPila(NodoHistorial **pila);
int leerEntero(const char *mensaje, int *valor);
int leerFlotante(const char *mensaje, float *valor);
void limpiarRestoLinea(void);
int leerCadena(const char *mensaje, char *destino, size_t capacidad);

int main(int argc, char *argv[]) {
    EstacionVigia estacion;
    ColaComandos cola;
    NodoHistorial *pilaDeshacer;
    NodoEvento *eventos;
    char estadoActuador[TAM_ESTADO];
    int siguienteId;
    int opcion = -1;
    int continuar;

    (void)argc;
    (void)argv;

    inicializarSistema(&estacion, &cola, &pilaDeshacer, &eventos,
                       estadoActuador, &siguienteId);

    do {
        printf("\n========== ESTACION VIGIA ==========\n");
        printf("Estado del actuador: %s\n", estadoActuador);
        printf("1. Capturar lectura de sensor             <- Fase 1\n");
        printf("2. Ver lecturas y estadisticas            <- Fase 1\n");
        printf("3. Ver registro de eventos/alertas        <- Fase 2\n");
        printf("4. Buscar evento por tipo                 <- Fase 2\n");
        printf("5. Eliminar evento por ID                 <- Fase 2\n");
        printf("6. Enviar comando al actuador (cola)       <- Fase 3\n");
        printf("7. Ejecutar comandos pendientes           <- Fase 3\n");
        printf("8. Deshacer ultimo comando (pila)         <- Fase 3\n");
        printf("9. Consultar estado del actuador\n");
        printf("0. Salir y liberar toda la memoria\n");

        continuar = leerEntero("Opcion: ", &opcion);
        if (!continuar) {
            printf("Entrada no valida. Intenta de nuevo.\n");
            continue;
        }

        switch (opcion) {
            case 1: {
                float lectura;
                if (leerFlotante("Ingresa la lectura del sensor: ", &lectura)) {
                    if (capturarLectura(&estacion, &eventos, lectura, &siguienteId)) {
                        printf("Lectura %.2f guardada correctamente.\n", lectura);
                        if (lectura > UMBRAL_ALERTA) {
                            printf("ALERTA: la lectura supero el umbral de %.2f.\n",
                                   UMBRAL_ALERTA);
                        } else {
                            printf("Estado normal: lectura dentro del umbral.\n");
                        }
                    } else {
                        printf("No se pudo guardar la lectura por falta de memoria.\n");
                    }
                } else {
                    printf("Lectura no valida.\n");
                }
                break;
            }
            case 2:
                mostrarLecturasYEstadisticas(&estacion);
                break;
            case 3:
                mostrarEventos(eventos);
                break;
            case 4:
                buscarEventoPorTipo(eventos);
                break;
            case 5:
                eliminarEvento(&eventos);
                break;
            case 6:
                enviarComando(&cola);
                break;
            case 7:
                ejecutarComandos(&cola, &pilaDeshacer, estadoActuador);
                break;
            case 8:
                deshacerUltimoComando(&pilaDeshacer, estadoActuador);
                break;
            case 9:
                mostrarEstadoActuador(estadoActuador);
                break;
            case 0:
                liberarTodaMemoria(&estacion, &eventos, &cola, &pilaDeshacer);
                printf("Sistema cerrado. Toda la memoria dinamica fue liberada.\n");
                break;
            default:
                printf("Opcion fuera de rango. Selecciona una opcion del menu.\n");
                break;
        }
    } while (opcion != 0);

    return 0;
}

void inicializarSistema(EstacionVigia *estacion, ColaComandos *cola,
                        NodoHistorial **pila, NodoEvento **eventos,
                        char estadoActuador[TAM_ESTADO], int *siguienteId) {
    estacion->lecturas = NULL;
    estacion->capacidad = 0;
    estacion->cantidad = 0;
    cola->frente = NULL;
    cola->final = NULL;
    *pila = NULL;
    *eventos = NULL;
    strcpy(estadoActuador, "APAGADO");
    *siguienteId = 1;
}

int capturarLectura(EstacionVigia *estacion, NodoEvento **eventos,
                    float lectura, int *siguienteId) {
    if (estacion->cantidad == estacion->capacidad) {
        size_t nuevaCapacidad = (estacion->capacidad == 0)
                                    ? 5 : estacion->capacidad * 2;
        float *temporal = (float *)realloc(
            estacion->lecturas, nuevaCapacidad * sizeof(float));
        if (temporal == NULL) {
            return 0;
        }
        estacion->lecturas = temporal;
        estacion->capacidad = nuevaCapacidad;
    }

    estacion->lecturas[estacion->cantidad] = lectura;
    estacion->cantidad++;

    if (lectura > UMBRAL_ALERTA) {
        if (!agregarEvento(eventos, *siguienteId, "UMBRAL SUPERADO", lectura)) {
            printf("Aviso: la lectura se guardo, pero no se pudo reservar memoria para la alerta.\n");
        } else {
            (*siguienteId)++;
        }
    }
    return 1;
}

int agregarEvento(NodoEvento **cabeza, int id, const char *tipo, float valor) {
    NodoEvento *nuevo = (NodoEvento *)malloc(sizeof(NodoEvento));
    NodoEvento *actual;
    if (nuevo == NULL) {
        return 0;
    }

    nuevo->id = id;
    snprintf(nuevo->tipo, sizeof(nuevo->tipo), "%s", tipo);
    nuevo->valor = valor;
    nuevo->siguiente = NULL;

    if (*cabeza == NULL) {
        *cabeza = nuevo;
    } else {
        actual = *cabeza;
        while (actual->siguiente != NULL) {
            actual = actual->siguiente;
        }
        actual->siguiente = nuevo;
    }
    return 1;
}

void mostrarLecturasYEstadisticas(const EstacionVigia *estacion) {
    size_t i;
    float minimo;
    float maximo;
    double suma = 0.0;

    if (estacion->cantidad == 0) {
        printf("Todavia no hay lecturas registradas.\n");
        return;
    }

    minimo = estacion->lecturas[0];
    maximo = estacion->lecturas[0];
    printf("\n--- LECTURAS DEL SENSOR ---\n");
    for (i = 0; i < estacion->cantidad; i++) {
        printf("%zu) %.2f\n", i + 1, estacion->lecturas[i]);
        suma += estacion->lecturas[i];
        if (estacion->lecturas[i] < minimo) minimo = estacion->lecturas[i];
        if (estacion->lecturas[i] > maximo) maximo = estacion->lecturas[i];
    }

    printf("Cantidad: %zu\n", estacion->cantidad);
    printf("Capacidad reservada: %zu\n", estacion->capacidad);
    printf("Promedio: %.2f\n", suma / (double)estacion->cantidad);
    printf("Maximo: %.2f\n", maximo);
    printf("Minimo: %.2f\n", minimo);
}

void mostrarEventos(const NodoEvento *cabeza) {
    if (cabeza == NULL) {
        printf("No hay eventos o alertas registrados.\n");
        return;
    }
    printf("\n--- REGISTRO DE EVENTOS ---\n");
    while (cabeza != NULL) {
        printf("ID: %d | Tipo: %s | Lectura: %.2f\n",
               cabeza->id, cabeza->tipo, cabeza->valor);
        cabeza = cabeza->siguiente;
    }
}
