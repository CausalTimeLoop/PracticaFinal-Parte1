#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Estructura para almacenar la configuración del sistema
typedef struct Config {
    int limite_retiro;
    int limite_transferencia;
    int umbral_retiros;
    int umbral_transferencias;
    int num_hilos;
    char archivo_cuentas[50];
    char archivo_log[50];
} Config;

// Estructura de cuenta bancaria
struct Cuenta {
    int numero_cuenta;
    char titular[50];
    float saldo;
    int num_transacciones;
};

// Estructura para pasar datos a los hilos
typedef struct {
    int tipo_operacion;  // 1: Depósito, 2: Retiro, 3: Transferencia
    int numero_cuenta;
    int cuenta_destino;  // Solo para transferencias
    float monto;
    sem_t *semaforo;
    char archivo_cuentas[50];
} DatosOperacion;

// Variables globales
Config configuracion;
int pipe_hijo_padre[2];     // Tubería para comunicación hijo -> padre
int pipe_padre_hijo[2];     // Tubería para comunicación padre -> hijo
int cuenta_usuario;         // Número de cuenta del usuario actual
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int usuario_id;             // ID único del proceso usuario


int main()
{
    int opcion;
    while (opcion = 1) 
    {
    printf("1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar saldo\n5. Salir\n");
    scanf("%d", &opcion);
    switch (opcion) 
    {
    case 1: realizar_deposito();
    break;
    case 2: realizar_retiro(); 
    break;
    case 3: realizar_transferencia(); 
    break;
    case 4: consultar_saldo(); 
    break;
    case 5: exit(0);
    }  
    }
}

