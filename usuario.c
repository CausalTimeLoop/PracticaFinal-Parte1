#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

struct Cuenta {
  int id;            // numero identificador de cuenta
  char titular[50];  // nombre del titular de cuenta
  float saldo;       // saldo de cuenta
  int operaciones;   // numero de operaciones de cuenta
};

struct Config {
  // limites de operaciones
  int LIMITE_RETIRO;
  int LIMITE_TRANSFERENCIA;
  // umbrales de deteccion de anomalias
  int UMBRAL_RETIROS;
  int UMBRAL_TRANSFERENCIAS;
  // parametros de ejecucion
  int NUM_HILOS;
  char ARCHIVO_CUENTAS[50];
  char ARCHIVO_LOG[50];
};

// Variables globales
struct Cuenta cuenta_actual;  // La cuenta del usuario actual
struct Config *config;        // Configuración del sistema
sem_t *sem_cuentas;           // Semáforo para acceso al archivo de cuentas
int pipe_usuario_banco[2];    // Tubería usuario -> banco
int pipe_banco_usuario[2];    // Tubería banco -> usuario

// Estructura para pasar datos a hilos
typedef struct {
  int tipo_operacion;  // 1: Depósito, 2: Retiro, 3: Transferencia
  float monto;
  int cuenta_destino;  // Solo para transferencias
} OperacionDatos;

// Funciones
void inicializar_usuario();
void menu_usuario();
void *realizar_deposito(void *arg);
void *realizar_retiro(void *arg);
void *realizar_transferencia(void *arg);
void consultar_saldo();
int actualizar_cuenta(struct Cuenta cuenta);
struct Cuenta buscar_cuenta_por_id(int id);
void registrar_operacion(const char *mensaje);

struct Config* configImport() {
  struct Config *config = malloc(sizeof(struct Config)); 
  if (config == NULL) {                                  
    perror("Error asignando memoria para Config\n");    
    return NULL;                                        
  }
  
  FILE *config_file = fopen("config.txt", "r"); 
  if (config_file == NULL) {                   
    perror("Error abriendo config.txt\n");     
    free(config);                             
    return NULL;                               
  }
  
  char key[50], val[50]; // 2 strings para leer los parametros del archivo 
  
  while (fscanf(config_file, "%s = %s", key, val) != EOF) { 
    if (strcmp(key, "LIMITE_RETIRO") == 0)                 
      config->LIMITE_RETIRO = atoi(val);                   
    else if (strcmp(key, "LIMITE_TRANSFERENCIA") == 0)    
      config->LIMITE_TRANSFERENCIA = atoi(val);            
    else if (strcmp(key, "UMBRAL_RETIROS") == 0)           
      config->UMBRAL_RETIROS = atoi(val);                  
    else if (strcmp(key, "UMBRAL_TRANSFERENCIAS") == 0)   
      config->UMBRAL_TRANSFERENCIAS = atoi(val);           
    else if (strcmp(key, "NUM_HILOS") == 0)                
      config->NUM_HILOS = atoi(val);                        
    else if (strcmp(key, "ARCHIVO_CUENTAS") == 0)        
      strcpy(config->ARCHIVO_CUENTAS, val);                
    else if (strcmp(key, "ARCHIVO_LOG") == 0)               
      strcpy(config->ARCHIVO_LOG, val);                     
  }
  
  fclose(config_file);
  return config;    
}

 // Inicializa el usuario leyendo los datos de la cuenta desde el FIFO
 
void inicializar_usuario() {
    config = configImport();
    if (config == NULL) {
        perror("Error cargando configuración");
        exit(1);
    }
    
    // Abre el semáforo para el acceso a cuentas
    sem_cuentas = sem_open("/cuentas_sem", 0);
    if (sem_cuentas == SEM_FAILED) {
        perror("Error abriendo semáforo de cuentas");
        exit(1);
    }
    
    // Lee los datos de la cuenta desde el FIFO
    char *fifo_path = "/tmp/banco_fifo";
    FILE *fifo = fopen(fifo_path, "r");
    if (!fifo) {
        perror("Error abriendo FIFO para lectura");
        exit(1);
    }
    
    // Lee los datos de la cuenta
    if (fscanf(fifo, "%d \"%[^\"]\" %f", &cuenta_actual.id, cuenta_actual.titular, &cuenta_actual.saldo) != 3) {
        perror("Error leyendo datos de cuenta desde FIFO");
        fclose(fifo);
        exit(1);
    }
    
    fclose(fifo);
    
    // Lee los datos completos de la cuenta desde el archivo
    if (sem_wait(sem_cuentas) == -1) {
        perror("Error adquiriendo semáforo");
        exit(1);
    }
    
    struct Cuenta cuenta_completa = buscar_cuenta_por_id(cuenta_actual.id);
    if (cuenta_completa.id == cuenta_actual.id) {
        cuenta_actual = cuenta_completa;
    }
    
    sem_post(sem_cuentas);
    
    printf("\n=== Bienvenido al Sistema Bancario SecureBank ===\n");
    printf("Usuario: %s (ID: %d)\n", cuenta_actual.titular, cuenta_actual.id);
    printf("Saldo actual: %.2f\n", cuenta_actual.saldo);
    printf("Operaciones realizadas: %d\n", cuenta_actual.operaciones);
}

// Menú
void menu_usuario() {
    int opcion;
    pthread_t hilo;
    OperacionDatos datos;
    
    while (1) {
        printf("\n=== MENÚ DE USUARIO ===\n");
        printf("1. Depósito\n");
        printf("2. Retiro\n");
        printf("3. Transferencia\n");
        printf("4. Consultar saldo\n");
        printf("5. Salir\n");
        printf("Seleccione una opción: ");
        
        scanf("%d", &opcion);
        
        switch (opcion) {
            case 1: { // Depósito
                printf("Ingrese monto a depositar: ");
                scanf("%f", &datos.monto);
                
                if (datos.monto <= 0) {
                    printf("El monto debe ser positivo.\n");
                    break;
                }
                
                pthread_create(&hilo, NULL, realizar_deposito, (void *)&datos);
                pthread_join(hilo, NULL);
                break;
            }
            
            case 2: { // Retiro
                printf("Ingrese monto a retirar: ");
                scanf("%f", &datos.monto);
                
                if (datos.monto <= 0) {
                    printf("El monto debe ser positivo.\n");
                    break;
                }
                
                if (datos.monto > config->LIMITE_RETIRO) {
                    printf("Error: El monto excede el límite de retiro (%d)\n", config->LIMITE_RETIRO);
                    break;
                }
                
                pthread_create(&hilo, NULL, realizar_retiro, (void *)&datos);
                pthread_join(hilo, NULL);
                break;
            }
            
            case 3: { // Transferencia
                printf("Ingrese cuenta destino: ");
                scanf("%d", &datos.cuenta_destino);
                
                if (datos.cuenta_destino == cuenta_actual.id) {
                    printf("No puedes transferir a tu propia cuenta.\n");
                    break;
                }
                
                printf("Ingrese monto a transferir: ");
                scanf("%f", &datos.monto);
                
                if (datos.monto <= 0) {
                    printf("El monto debe ser positivo.\n");
                    break;
                }
                
                if (datos.monto > config->LIMITE_TRANSFERENCIA) {
                    printf("Error: El monto excede el límite de transferencia (%d)\n", config->LIMITE_TRANSFERENCIA);
                    break;
                }
                
                pthread_create(&hilo, NULL, realizar_transferencia, (void *)&datos);
                pthread_join(hilo, NULL);
                break;
            }
            
            case 4: // Consultar saldo
                consultar_saldo();
                break;
                
            case 5: // Salir
                printf("Gracias por utilizar SecureBank. ¡Hasta pronto!\n");
                sem_close(sem_cuentas);
                free(config);
                exit(0);
                
            default:
                printf("Opción inválida. Intente nuevamente.\n");
        }
    }
}


void *realizar_deposito(void *arg) {
    OperacionDatos *datos = (OperacionDatos *)arg;
    float monto = datos->monto;
    
    // Adquiere el semáforo
    if (sem_wait(sem_cuentas) == -1) {
        perror("Error adquiriendo semáforo");
        pthread_exit(NULL);
    }
    
    // Busca la cuenta y la actualiza
    struct Cuenta cuenta = buscar_cuenta_por_id(cuenta_actual.id);
    
    if (cuenta.id != cuenta_actual.id) {
        printf("Error: No se encontró la cuenta.\n");
        sem_post(sem_cuentas);
        pthread_exit(NULL);
    }
    
    // Realiza el depósito
    cuenta.saldo += monto;
    cuenta.operaciones++;
    
    // Actualiza la cuenta en el archivo
    if (actualizar_cuenta(cuenta)) {
        cuenta_actual = cuenta;
        printf("Depósito realizado con éxito. Nuevo saldo: %.2f\n", cuenta.saldo);
        
        // Registra la operación
        char mensaje[100];
        sprintf(mensaje, "Depósito en cuenta %d: +%.2f", cuenta.id, monto);
        registrar_operacion(mensaje);
    } else {
        printf("Error al realizar el depósito.\n");
    }
    
    // Libera el semáforo
    sem_post(sem_cuentas);
    pthread_exit(NULL);
}

void *realizar_retiro(void *arg) {
    OperacionDatos *datos = (OperacionDatos *)arg;
    float monto = datos->monto;
    
    // Adquiere el semáforo
    if (sem_wait(sem_cuentas) == -1) {
        perror("Error adquiriendo semáforo");
        pthread_exit(NULL);
    }
    
    // Buscar la cuenta y la actualiza
    struct Cuenta cuenta = buscar_cuenta_por_id(cuenta_actual.id);
    
    if (cuenta.id != cuenta_actual.id) {
        printf("Error: No se encontró la cuenta.\n");
        sem_post(sem_cuentas);
        pthread_exit(NULL);
    }
    
    // Verifica si el saldo es suficiente
    if (cuenta.saldo < monto) {
        printf("Error: Saldo insuficiente. Saldo actual: %.2f\n", cuenta.saldo);
        sem_post(sem_cuentas);
        pthread_exit(NULL);
    }
    
    // Realiza el retiro
    cuenta.saldo -= monto;
    cuenta.operaciones++;
    
    // Actualiza la cuenta en el archivo
    if (actualizar_cuenta(cuenta)) {
        cuenta_actual = cuenta;
        printf("Retiro realizado con éxito. Nuevo saldo: %.2f\n", cuenta.saldo);
        
        // Registra la operación
        char mensaje[100];
        sprintf(mensaje, "Retiro en cuenta %d: -%.2f", cuenta.id, monto);
        registrar_operacion(mensaje);
    } else {
        printf("Error al realizar el retiro.\n");
    }
    
    // Libera el semáforo
    sem_post(sem_cuentas);
    pthread_exit(NULL);
}


void *realizar_transferencia(void *arg) {
    OperacionDatos *datos = (OperacionDatos *)arg;
    float monto = datos->monto;
    int cuenta_destino_id = datos->cuenta_destino;
    
    // Adquirir el semáforo
    if (sem_wait(sem_cuentas) == -1) {
        perror("Error adquiriendo semáforo");
        pthread_exit(NULL);
    }
    
    // Busca la cuenta de origen
    struct Cuenta cuenta_origen = buscar_cuenta_por_id(cuenta_actual.id);
    
    if (cuenta_origen.id != cuenta_actual.id) {
        printf("Error: No se encontró la cuenta origen.\n");
        sem_post(sem_cuentas);
        pthread_exit(NULL);
    }
    
    // Verifica si el saldo es sufuficiente
    if (cuenta_origen.saldo < monto) {
        printf("Error: Saldo insuficiente. Saldo actual: %.2f\n", cuenta_origen.saldo);
        sem_post(sem_cuentas);
        pthread_exit(NULL);
    }
    
    // Busca la cuenta de destino
    struct Cuenta cuenta_destino = buscar_cuenta_por_id(cuenta_destino_id);
    
    if (cuenta_destino.id != cuenta_destino_id) {
        printf("Error: La cuenta destino no existe.\n");
        sem_post(sem_cuentas);
        pthread_exit(NULL);
    }
    
    // Realizar la transferencia
    cuenta_origen.saldo -= monto;
    cuenta_origen.operaciones++;
    
    cuenta_destino.saldo += monto;
    cuenta_destino.operaciones++;
    
    // Actualiza ambas cuentas
    int exito = actualizar_cuenta(cuenta_origen) && actualizar_cuenta(cuenta_destino);
    
    if (exito) {
        cuenta_actual = cuenta_origen;
        printf("Transferencia realizada con éxito. Nuevo saldo: %.2f\n", cuenta_origen.saldo);
        
        // Registra la operación
        char mensaje[100];
        sprintf(mensaje, "Transferencia de cuenta %d a cuenta %d: %.2f", 
                cuenta_origen.id, cuenta_destino.id, monto);
        registrar_operacion(mensaje);
    } else {
        printf("Error al realizar la transferencia.\n");
    }
    
    // Libera el semáforo
    sem_post(sem_cuentas);
    pthread_exit(NULL);
}

void consultar_saldo() {
    // Adquirir el semáforo
    if (sem_wait(sem_cuentas) == -1) {
        perror("Error adquiriendo semáforo");
        return;
    }
    
    // Buscar la cuenta
    struct Cuenta cuenta = buscar_cuenta_por_id(cuenta_actual.id);
    
    if (cuenta.id == cuenta_actual.id) {
        cuenta_actual = cuenta;
        printf("\n=== INFORMACIÓN DE CUENTA ===\n");
        printf("ID: %d\n", cuenta.id);
        printf("Titular: %s\n", cuenta.titular);
        printf("Saldo actual: %.2f\n", cuenta.saldo);
        printf("Operaciones realizadas: %d\n", cuenta.operaciones);
    } else {
        printf("Error: No se pudo obtener la información de la cuenta.\n");
    }
    
    // Libera el semáforo
    sem_post(sem_cuentas);
}

 // Busca una cuenta por su ID en el archivo de cuentas
struct Cuenta buscar_cuenta_por_id(int id) {
    struct Cuenta cuenta;
    memset(&cuenta, 0, sizeof(struct Cuenta));
    
    FILE *archivo = fopen(config->ARCHIVO_CUENTAS, "rb");
    if (archivo == NULL) {
        perror("Error abriendo el archivo de cuentas");
        return cuenta;
    }
    
    while (fread(&cuenta, sizeof(struct Cuenta), 1, archivo) == 1) {
        if (cuenta.id == id) {
            fclose(archivo);
            return cuenta;
        }
    }
    
    memset(&cuenta, 0, sizeof(struct Cuenta));
    fclose(archivo);
    return cuenta;
}

 // Actualiza una cuenta en el archivo de cuentas
int actualizar_cuenta(struct Cuenta cuenta) {
    FILE *archivo = fopen(config->ARCHIVO_CUENTAS, "rb+");
    if (archivo == NULL) {
        perror("Error abriendo el archivo de cuentas");
        return 0;
    }
    
    struct Cuenta temp;
    int encontrado = 0;
    
    while (fread(&temp, sizeof(struct Cuenta), 1, archivo) == 1) {
        if (temp.id == cuenta.id) {
            fseek(archivo, -sizeof(struct Cuenta), SEEK_CUR);
            fwrite(&cuenta, sizeof(struct Cuenta), 1, archivo);
            encontrado = 1;
            break;
        }
    }
    
    fclose(archivo);
    return encontrado;
}

 // Registra una operación en el log 
void registrar_operacion(const char *mensaje) {
    // Esta función envia el mensaje al proceso principal para el registro
    
    // Por ahora, simplemente escribiremos en el archivo de log directamente
    FILE *log = fopen(config->ARCHIVO_LOG, "a");
    if (log == NULL) {
        perror("Error abriendo archivo de log");
        return;
    }
    
    // Obtener la hora actual
    time_t now = time(NULL);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", localtime(&now));
    
    // Escribir la entrada en el log
    fprintf(log, "%s %s\n", timestamp, mensaje);
    fclose(log);
}

// Función principal
int main() {
    // Inicializa el usuario
    inicializar_usuario();
    
    // Muestra el menú de usuario
    menu_usuario();
    
    return 0;
}
