/**
 * usuario.c
 * 
 * Implementación del proceso hijo (usuario) para el sistema bancario SecureBank.
 * Gestiona el menú interactivo y las operaciones bancarias mediante hilos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>

// Estructura para la configuración del sistema
typedef struct {
    int limite_retiro;
    int limite_transferencia;
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

// Estructura para datos de operación
typedef struct {
    int tipo_operacion;  // 1: Depósito, 2: Retiro, 3: Transferencia
    int numero_cuenta;
    int cuenta_destino;  // Solo para transferencias
    float monto;
} DatosOperacion;

// Variables globales
Config config;
int pipe_hijo_padre[2];
int pipe_padre_hijo[2];
int cuenta_usuario;
sem_t *semaforo;

// Prototipos de funciones
void ejecutar_menu_usuario(int tuberias[2], Config cfg, int num_cuenta);
void *realizar_operacion(void *arg);
struct Cuenta buscar_cuenta(int numero_cuenta);
int actualizar_cuenta(struct Cuenta cuenta);
void enviar_mensaje_padre(const char *mensaje);

/**
 * Inicializa y ejecuta el menú de usuario
 */
void ejecutar_menu_usuario(int tuberia_ph[2], int tuberia_hp[2], Config cfg, int num_cuenta) {
    // Inicializar variables globales
    config = cfg;
    pipe_padre_hijo[0] = tuberia_ph[0];
    pipe_padre_hijo[1] = tuberia_ph[1];
    pipe_hijo_padre[0] = tuberia_hp[0];
    pipe_hijo_padre[1] = tuberia_hp[1];
    cuenta_usuario = num_cuenta;
    
    // Abrir el semáforo
    semaforo = sem_open("/cuentas_sem", 0);
    if (semaforo == SEM_FAILED) {
        perror("Error al abrir el semáforo");
        exit(EXIT_FAILURE);
    }
    
    printf("\n--- Bienvenido al Sistema Bancario SecureBank ---\n");
    printf("Usuario conectado. Cuenta: %d\n", cuenta_usuario);
    
    int opcion;
    float monto;
    int cuenta_destino;
    pthread_t hilo;
    DatosOperacion datos;
    
    while (1) {
        printf("\n=== MENÚ DE USUARIO ===\n");
        printf("1. Depósito\n");
        printf("2. Retiro\n");
        printf("3. Transferencia\n");
        printf("4. Consultar saldo\n");
        printf("5. Salir\n");
        printf("Seleccione una opción: ");
        
        if (scanf("%d", &opcion) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Entrada inválida. Intente nuevamente.\n");
            continue;
        }
        
        switch (opcion) {
            case 1: // Depósito
                printf("Ingrese monto a depositar: ");
                scanf("%f", &monto);
                if (monto <= 0) {
                    printf("El monto debe ser positivo.\n");
                    break;
                }
                
                datos.tipo_operacion = 1;
                datos.numero_cuenta = cuenta_usuario;
                datos.monto = monto;
                
                pthread_create(&hilo, NULL, realizar_operacion, &datos);
                pthread_join(hilo, NULL);
                break;
                
            case 2: // Retiro
                printf("Ingrese monto a retirar: ");
                scanf("%f", &monto);
                if (monto <= 0) {
                    printf("El monto debe ser positivo.\n");
                    break;
                }
                
                if (monto > config.limite_retiro) {
                    printf("Error: Excede el límite de retiro (%d)\n", config.limite_retiro);
                    break;
                }
                
                datos.tipo_operacion = 2;
                datos.numero_cuenta = cuenta_usuario;
                datos.monto = monto;
                
                pthread_create(&hilo, NULL, realizar_operacion, &datos);
                pthread_join(hilo, NULL);
                break;
                
            case 3: // Transferencia
                printf("Ingrese cuenta destino: ");
                scanf("%d", &cuenta_destino);
                
                if (cuenta_destino == cuenta_usuario) {
                    printf("No puede transferir a su propia cuenta.\n");
                    break;
                }
                
                printf("Ingrese monto a transferir: ");
                scanf("%f", &monto);
                if (monto <= 0) {
                    printf("El monto debe ser positivo.\n");
                    break;
                }
                
                if (monto > config.limite_transferencia) {
                    printf("Error: Excede el límite de transferencia (%d)\n", config.limite_transferencia);
                    break;
                }
                
                datos.tipo_operacion = 3;
                datos.numero_cuenta = cuenta_usuario;
                datos.cuenta_destino = cuenta_destino;
                datos.monto = monto;
                
                pthread_create(&hilo, NULL, realizar_operacion, &datos);
                pthread_join(hilo, NULL);
                break;
                
            case 4: // Consultar saldo
                sem_wait(semaforo);
                struct Cuenta cuenta = buscar_cuenta(cuenta_usuario);
                sem_post(semaforo);
                
                if (cuenta.numero_cuenta == cuenta_usuario) {
                    printf("\n=== DATOS DE LA CUENTA ===\n");
                    printf("Titular: %s\n", cuenta.titular);
                    printf("Número: %d\n", cuenta.numero_cuenta);
                    printf("Saldo: %.2f\n", cuenta.saldo);
                    printf("Transacciones: %d\n", cuenta.num_transacciones);
                } else {
                    printf("Error al obtener datos de la cuenta.\n");
                }
                break;
                
            case 5: // Salir
                printf("Gracias por utilizar SecureBank. ¡Hasta pronto!\n");
                sem_close(semaforo);
                close(pipe_hijo_padre[1]);
                close(pipe_padre_hijo[0]);
                exit(EXIT_SUCCESS);
                
            default:
                printf("Opción inválida.\n");
        }
    }
}

/**
 * Función ejecutada por el hilo para realizar operaciones bancarias
 */
void *realizar_operacion(void *arg) {
    DatosOperacion *datos = (DatosOperacion *)arg;
    char mensaje[100];
    
    sem_wait(semaforo);
    
    // Obtener cuenta origen
    struct Cuenta cuenta = buscar_cuenta(datos->numero_cuenta);
    if (cuenta.numero_cuenta != datos->numero_cuenta) {
        printf("Error: Cuenta no encontrada.\n");
        sem_post(semaforo);
        pthread_exit(NULL);
    }
    
    switch (datos->tipo_operacion) {
        case 1: // Depósito
            cuenta.saldo += datos->monto;
            cuenta.num_transacciones++;
            if (actualizar_cuenta(cuenta)) {
                printf("Depósito realizado con éxito. Nuevo saldo: %.2f\n", cuenta.saldo);
                sprintf(mensaje, "Depósito en cuenta %d: +%.2f", cuenta.numero_cuenta, datos->monto);
                enviar_mensaje_padre(mensaje);
            } else {
                printf("Error al realizar el depósito.\n");
            }
            break;
            
        case 2: // Retiro
            if (cuenta.saldo < datos->monto) {
                printf("Error: Saldo insuficiente.\n");
                sem_post(semaforo);
                pthread_exit(NULL);
            }
            
            cuenta.saldo -= datos->monto;
            cuenta.num_transacciones++;
            if (actualizar_cuenta(cuenta)) {
                printf("Retiro realizado con éxito. Nuevo saldo: %.2f\n", cuenta.saldo);
                sprintf(mensaje, "Retiro en cuenta %d: -%.2f", cuenta.numero_cuenta, datos->monto);
                enviar_mensaje_padre(mensaje);
            } else {
                printf("Error al realizar el retiro.\n");
            }
            break;
            
        case 3: // Transferencia
            // Verificar saldo suficiente
            if (cuenta.saldo < datos->monto) {
                printf("Error: Saldo insuficiente para la transferencia.\n");
                sem_post(semaforo);
                pthread_exit(NULL);
            }
            
            // Buscar cuenta destino
            struct Cuenta cuenta_dest = buscar_cuenta(datos->cuenta_destino);
            if (cuenta_dest.numero_cuenta != datos->cuenta_destino) {
                printf("Error: Cuenta destino no encontrada.\n");
                sem_post(semaforo);
                pthread_exit(NULL);
            }
            
            // Realizar transferencia
            cuenta.saldo -= datos->monto;
            cuenta.num_transacciones++;
            cuenta_dest.saldo += datos->monto;
            cuenta_dest.num_transacciones++;
            
            // Actualizar ambas cuentas
            if (actualizar_cuenta(cuenta) && actualizar_cuenta(cuenta_dest)) {
                printf("Transferencia realizada con éxito. Nuevo saldo: %.2f\n", cuenta.saldo);
                sprintf(mensaje, "Transferencia de cuenta %d a cuenta %d: %.2f", 
                        cuenta.numero_cuenta, cuenta_dest.numero_cuenta, datos->monto);
                enviar_mensaje_padre(mensaje);
            } else {
                printf("Error al realizar la transferencia.\n");
            }
            break;
    }
    
    sem_post(semaforo);
    pthread_exit(NULL);
}

/**
 * Busca una cuenta en el archivo de cuentas
 */
struct Cuenta buscar_cuenta(int numero_cuenta) {
    struct Cuenta cuenta;
    memset(&cuenta, 0, sizeof(struct Cuenta));
    
    FILE *archivo = fopen(config.archivo_cuentas, "rb");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de cuentas");
        return cuenta;
    }
    
    while (fread(&cuenta, sizeof(struct Cuenta), 1, archivo) == 1) {
        if (cuenta.numero_cuenta == numero_cuenta) {
            fclose(archivo);
            return cuenta;
        }
    }
    
    // Si llega aquí, no encontró la cuenta
    memset(&cuenta, 0, sizeof(struct Cuenta));
    fclose(archivo);
    return cuenta;
}

/**
 * Actualiza una cuenta en el archivo de cuentas
 */
int actualizar_cuenta(struct Cuenta cuenta) {
    FILE *archivo = fopen(config.archivo_cuentas, "rb+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de cuentas");
        return 0;
    }
    
    struct Cuenta temp;
    while (fread(&temp, sizeof(struct Cuenta), 1, archivo) == 1) {
        if (temp.numero_cuenta == cuenta.numero_cuenta) {
            // Retroceder al inicio del registro
            fseek(archivo, -sizeof(struct Cuenta), SEEK_CUR);
            // Escribir el registro actualizado
            fwrite(&cuenta, sizeof(struct Cuenta), 1, archivo);
            fclose(archivo);
            return 1;
        }
    }
    
    fclose(archivo);
    return 0;
}

/**
 * Envía un mensaje al proceso padre
 */
void enviar_mensaje_padre(const char *mensaje) {
    write(pipe_hijo_padre[1], mensaje, strlen(mensaje) + 1);
}
