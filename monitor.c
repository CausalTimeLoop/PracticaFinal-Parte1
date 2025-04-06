#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define MQ_KEY 0451                 // message queue key     
#define PIPE_PATH "/tmp/alert_pipe" // path to the named pipe

// define the Mensaje struct
struct Mensaje {
  long mtype;  // message type, filter and priority of messages, obligatory
  int id;      // numero identificador de cuenta
  char tipo;   // 'R' retiro, 'D' deposito, 'T' transferencia
  float monto; // money amount
};

// define Alert struct for pipeline message
struct Alert {
  int id;            // account id
  char message[100]; // alert message
};

// define the Config struct
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

/*
USAGE: 
  import config.txt into a config struct for the program to use
INPUT: 
  void
OUTPUT:
  struct Config *config = a pointer to the config structure 
*/
struct Config* configImport() {
  struct Config *config = malloc(sizeof(struct Config)); // allocate memory for struct Config
  if (config == NULL) {                                  // if malloc() failed to allocate memory
    perror("Error asignando memoria para Config\n");     // print error message
    return NULL;                                         // exit function early
  }
  
  FILE *config_file = fopen("config.txt","r"); // open file in read mode
  if (config_file == NULL) {                   // if file failed to open
    perror("Error abriendo config.txt\n");     // print error message
    free(config);                              // free memory
    return NULL;                               // exit function early
  }
  
  char key[50], val[50]; // two strings to read parameters from the file
  
  while (fscanf(config_file, "%s = %s", key, val) != EOF) { // read key-value pair until End Of File
    if (strcmp(key, "LIMITE_RETIRO") == 0)                  // if key matchen then
      config->LIMITE_RETIRO = atoi(val);                    // convert to integer and assign value
    else if (strcmp(key, "LIMITE_TRANSFERENCIA") == 0)      // if key matches then 
      config->LIMITE_TRANSFERENCIA = atoi(val);             // convert to integer and assign value
    else if (strcmp(key, "UMBRAL_RETIROS") == 0)            // if key matches
      config->UMBRAL_RETIROS = atoi(val);                   // convert to integer and assign value
    else if (strcmp(key, "UMBRAL_TRANSFERENCIAS") == 0)     // if key matches
      config->UMBRAL_TRANSFERENCIAS = atoi(val);            // convert to integer and assign value
    else if (strcmp(key, "NUM_HILOS") == 0)                 // if key matches
      config->NUM_HILOS = atoi(val);                        // convert to integer and assign value
    else if (strcmp(key, "ARCHIVO_CUENTAS") == 0)           // if key matches
      strcpy(config->ARCHIVO_CUENTAS, val);                 // assign value
    else if (strcmp(key, "ARCHIVO_LOG") == 0)               // if key matches
      strcpy(config->ARCHIVO_LOG, val);                     // assign value
  }
  
  fclose(config_file);
  return config;    
}

/*
USAGE: 
  send alert to the main process via pipe
INPUT: 
  int id              == maccount id
  const char *message == string describing alert
OUTPUT:
  void
*/
void alertSend(int id, const char *message) {
  struct Alert alert; // declare local alert struct
  alert.id = id;      // copy given id into struct id
  // copy given message into alert.message
  strncpy(alert.message, message, sizeof(alert.message) - 1);
  
  // Open the pipe for writing
  FILE *pipe = fopen(PIPE_PATH, "w"); // try to open named pipe for writing
  if (pipe == NULL) {             // if it fails
    perror("Error opening pipe"); // print error
    return;                       // exit function early
  }
  // write alert to the pipe
  fprintf(pipe, "Alert: Cuenta ID %d - %s\n", alert.id, alert.message);
  fclose(pipe); // close pipe
}

/*
USAGE: 
  track consecutive withdrawals
INPUT: 
  int account_id        == id of transaction account         
  float amount          == amount withdrawn in current transaction
  struct Config *config == pointer to configuration struct
  int *last_withdrawal  == pointer to last withdrawal amount
OUTPUT:
  void
*/
void trackWithdrawal(int account_id, float amount, struct Config *config, float *last_withdrawal) {
  if (amount > 0) {                                             // if its a withdrawal
    if (*last_withdrawal > 0) {                                 // and if there was a previous withdrawal
      if (*last_withdrawal + amount > config->UMBRAL_RETIROS) { // analyses if suspicious withdrawals
        char alert_message[100]; // build alert message string
        snprintf(alert_message, sizeof(alert_message), "CLos retiros consecutivos superan el umbral: %.2f + %.2f = %.2f", *last_withdrawal, amount, *last_withdrawal + amount);
        alertSend(account_id, alert_message);  // send alert to main process
      }
    }

    *last_withdrawal = amount; // update pointer value to this latest withdrawal
  } else {
    *last_withdrawal = 0;      // otherwise reset the tracker
  }
  return;
}

/*
USAGE: 
  receive messages from the message queue
INPUT: 
  int msgid               == message queue identifier
  struct Mensaje *mensaje == pointer to buffer mensaje struct
OUTPUT:
  0 /-1                   == communicate whether an error occured
*/
int msgReceive(int msgid, struct Mensaje *mensaje) {
  // read one message from queue
  ssize_t recibido = msgrcv(msgid, mensaje, sizeof(*mensaje) - sizeof(long), 0, 0);  // Receive message from queue
  if (recibido == -1) {                   // if reading message fails
    perror("Error al recibir mensaje\n"); // print error
    return -1;                            // communicate error
  }
  return 0; // function completed correctly
}

/*
USAGE: 
  create a message queue and handle errors
INPUT: 
  void
OUTPUT:
  int msgid == message queue identifier
*/
int msgQueueCreate() {
  int msgid = msgget(MQ_KEY, 0666 | IPC_CREAT); // create or access message queue
  if (msgid == -1) {                                         // if message queue fails
    perror("Error obteniendo cola de mensajes (monitor)\n"); // print error
    exit(1);                                                 // exit function early
  } 
  return msgid;
}

/*
USAGE: 
  create a named pipe and handle errors
INPUT: 
  void
OUTPUT:
  void
*/
void pipeCreate() {
  // Check if the FIFO exists (no need to remove it)
  if (access(PIPE_PATH, F_OK) == -1) {
    // if FIFO doesnt exist then create it
    if (mkfifo(PIPE_PATH, 0666) != 0) {
      perror("Error creando la tuberia FIFO\n");
      return;
    }
  }
}

int main() {
  // initialise message queue
  int msgid = msgQueueCreate();

  // import config data
  struct Config *config = configImport();
  if (config == NULL) {                        // if config fails to load
    perror("Error al cargar configuracion\n"); // print error
    exit(1);                                   // exit prematurely
  }

  // create named pipe
  pipeCreate();

  struct Mensaje mensaje;           // declare mensaje struct
  int consecutive_withdrawals = 0;  // counter for consecutive withdrawals
  float last_withdrawal = 0;        // last withdrawal amount for the account

  while (1) {
    // receive message from the queue
    if (msgReceive(msgid, &mensaje) == -1) {
      continue;
    }

    //printf("[Monitor] Transaccion recibida: Cuenta ID: %d | Tipo: %c | Monto: %.2f\n", mensaje.id, mensaje.tipo, mensaje.monto);

    // track withdrawals and detect suspicious activity
    if (mensaje.tipo == 'R') {
      trackWithdrawal(mensaje.id, mensaje.monto, config, &last_withdrawal);
    }
  }
  return 0;
}
