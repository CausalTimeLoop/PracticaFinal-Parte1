#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#define PIPE_PATH "/tmp/alert_pipe" // path to the named pipe

// define the Cuenta struct
struct Cuenta {
  int id;            // numero identificador de cuenta
  char titular [50]; // nombre del titular de cuenta
  float saldo;       // saldo de cuenta
  int operaciones;   // numero de operaciones de cuenta
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
  listen for and process monitor alerts
INPUT:
  void
OUTPUT:
  void
*/
void receiveAlerts() {
  char alert_message[256];               // buffer for alert message
  
  // Check if the FIFO exists (no need to remove it)
  if (access(PIPE_PATH, F_OK) == -1) {
    // if FIFO doesnt exist then create it
    if (mkfifo(PIPE_PATH, 0666) != 0) {
      perror("Error creando la tuberia FIFO\n");
      return;
    }
  }

  FILE *fifo = fopen(PIPE_PATH, "r"); // open pipe in read mode
  if (!fifo) {                                    // if opening fails then
    perror("Error abriendo FIFO para lectura\n"); // print error
    return;                                       // and exit function early
  }

  while (1) { // infinite loop
    // block and read from fifo
    if (fgets(alert_message, sizeof(alert_message), fifo)) {
      printf("Alerta recibida: %s", alert_message);  // display alert message
    } else {
      break; // if theres no more data to read then break loop
    }
  }

  fclose(fifo); // close pipe
  return;
}


/*
USAGE:
  start monitor.c as a background child process
INPUT:
  void
OUTPUT:
  void
*/
void monitorStart() {
  pid_t pid = fork(); // create new child process

  if (pid < 0) {                              // if process creation fails
    perror("Error creando fork (monitor)\n"); // print error
    exit(1);                                  // exit with error
  }

  if (pid == 0) { // child process
    printf("Iniciando monitor en segundo plano...\n"); // show we are in the child process
    execl("./monitor", "monitor", (char *)NULL);       // execute monitor.c
    perror("Error al ejecutar monitor");               // if execl fails print error
    exit(1);                                           // exit with error
  }
  // parent process continues execution
}

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
  print in a structured manner the imported config.txt to make sure it was read correctly
INPUT: 
  struct Config *config == a pointer to the config structure
OUTPUT:
  void
*/
void configPrint() {
  struct Config *config = configImport();         // import config data
  if (config == NULL) {                           // if config import fails
    perror("Error en la funcion configImport\n"); // print error
    free(config);                                 // free allocated memory
    return;                                       // exit function early
  }

  // display information
  printf("Configuración Cargada:\n");
  printf("LIMITE_RETIRO: %d\n",         config->LIMITE_RETIRO);
  printf("LIMITE_TRANSFERENCIA: %d\n",  config->LIMITE_TRANSFERENCIA);
  printf("UMBRAL_RETIROS: %d\n",        config->UMBRAL_RETIROS);
  printf("UMBRAL_TRANSFERENCIAS: %d\n", config->UMBRAL_TRANSFERENCIAS);
  printf("NUM_HILOS: %d\n",             config->NUM_HILOS);
  printf("ARCHIVO_CUENTAS: %s\n",       config->ARCHIVO_CUENTAS);
  printf("ARCHIVO_LOG: %s\n",           config->ARCHIVO_LOG);
  
  free(config); // free allocated memory
  return;
}

/*
USAGE: 
  import cuentas.dat into a Cuenta array for the program to use
INPUT: 
  size_t *cuentas_num    == a pointer to the amount of records imported from cuenta.dat
  const char *file_name  == string containing the name of the data file
OUTPUT:
  struct Cuenta *cuentas == a pointer to the array containing all imported cuentas
*/
struct Cuenta* cuentasImport(size_t *cuentas_num) {

  // create /cuentas_sem semaphore with an initial value of 1
  sem_t *sem = sem_open("/cuentas_sem", O_CREAT, S_IRUSR | S_IWUSR, 1);
  if (sem == SEM_FAILED) {               // if semaphore fails
    perror("Error abriendo semaforo\n"); // print error message
    return NULL;                         // and exit function early
  }
  
  if (sem_wait(sem) < 0) {                  // aquire semaphore
    perror("Error esperando a semaforo\n"); // if it fails print error
    sem_close(sem);                         // close semaphore
    return NULL;                            // and exit function early
  }

  struct Config *config = configImport();         // import config data
  if (config == NULL) {                           // if config import fails
    perror("Error en la funcion configImport\n"); // print error
    free(config);                                 // free allocated memory
    return NULL;                                  // exit function early
  }
  
  // open cuentas.dat file in binary read mode
  FILE *cuentas_file = fopen(config->ARCHIVO_CUENTAS, "rb");
  if (cuentas_file == NULL) {                      // if the file does not exist or fails to open
    perror("Error abriendo cuentas.dat\n");        // print error message
    sem_post(sem);                                 // release semaphore
    sem_close(sem);                                // close semaphore
    return NULL;                                   // and exit function early
  }
  
  free(config); // free allocated memory
  
  fseek(cuentas_file, 0, SEEK_END);        // moves file pointer to the end
  long file_size = ftell(cuentas_file);    // get current position to determine file size
  rewind(cuentas_file);                    // moves file pointer back to beginning
  
  *cuentas_num = file_size / sizeof(struct Cuenta); // get number of structs by dividing file size by struct size 
  if (*cuentas_num == 0) {                                  // if no records are found
    perror("No se encontraron registros en cuentas.dat\n"); // print error message
    fclose(cuentas_file);                                   // close file
    sem_post(sem);                                          // release semaphore
    sem_close(sem);                                         // close semaphore
    return NULL;                                            // and exit function early
  }
  
  struct Cuenta *cuentas = malloc(*cuentas_num * sizeof(struct Cuenta)); // allocate memory for array of cuentas
  if (cuentas == NULL) {                                  // if malloc fails due to lack of memory
    perror("No se pudo asignar memoria a las cuentas\n"); // print message
    fclose(cuentas_file);                                 // close file
    sem_post(sem);                                        // release semaphore
    sem_close(sem);                                       // close semaphore
    return NULL;                                          // and exit function early
  }
  
  // reads *cuentas_num records from cuentas.dat into the allocated array cuentas
  //   cuentas               == memory location where data will be stored
  //   sizeof(struct Cuenta) == size of each record
  //   *cuentas_num          == number of records to read
  //   cuentas_file          == file to read from
  size_t read_count = fread(cuentas, sizeof(struct Cuenta), *cuentas_num, cuentas_file);
  if (read_count != *cuentas_num) {        // if read fewer records than expected then
    perror("Error leyendo cuentas.dat\n"); // print error
    free(cuentas);                         // free allocated memory
    fclose(cuentas_file);                  // close file
    sem_post(sem);                         // release semaphore
    sem_close(sem);                        // close semaphore
    return NULL;                           // exit function early
  }
  
  fclose(cuentas_file);
  sem_post(sem);
  sem_close(sem);
  return cuentas;
}

/*
USAGE: 
  print in a structured manner the imported cuenta.dat to make sure it was read correctly
INPUT: 
  struct Cuenta *cuentas == a pointer to the array containing all imported cuentas
  size_t cuentas_num     == the amount of records imported from cuenta.dat
OUTPUT:
  void
*/
void cuentasPrint() {
  // struct Cuenta* cuentasImport(size_t *cuentas_num)
  size_t cuentas_num;
  struct Cuenta *cuentas = cuentasImport(&cuentas_num);
  if (cuentas == NULL) {                           // if cuentas import fails
    perror("Error en la funcion cuentasImport\n"); // print error
    free(cuentas);                                 // free allocated memory
    return;                                        // exit function early
  }

  if (cuentas == NULL || cuentas_num == 0) { // check if array is NULL or contains no accounts
    printf("No hay cuentas por mostrar\n");  // if so then print error message
    free(cuentas);                           // free allocated memory
    return;                                  // exit function early
  }
  
  printf("Cuentas almacenadas:\n");          // print the header
  for (size_t i = 0; i < cuentas_num; i++) { // loop through all acounts and print their details
    printf("ID: %d | Titular: %s | Saldo: %.2f | Operaciones: %d\n",
      cuentas[i].id, cuentas[i].titular, cuentas[i].saldo, cuentas[i].operaciones);
  }
  
  free(cuentas); // free allocated memory
  return;
}

/*
USAGE: 
  ask user to login using their cuenta ID and open a new usuario.c 
  instance in a new terminal with their cuenta loaded
INPUT: 
  struct Cuenta *cuentas == a pointer to the array containing all imported cuentas
  size_t cuentas_num     == the amount of records imported from cuenta.dat
OUTPUT:
  void
*/
void cuentaLogin() {
  int cuenta_id;                    // variable to store account id
  printf("Ingrese su Cuenta ID: "); // prompt user to enter account id
  scanf("%d", &cuenta_id);          // read account id from user input
  
  // struct Cuenta* cuentasImport(size_t *cuentas_num)
  size_t cuentas_num;
  struct Cuenta *cuentas = cuentasImport(&cuentas_num);
  if (cuentas == NULL) {                           // if cuentas import fails
    perror("Error en la funcion cuentasImport\n"); // print error
    free(cuentas);                                 // free allocated memory
    return;                                        // exit function early
  }

  struct Cuenta cuenta_selec; // pointer to store the selected account
  cuenta_selec.id = -1; // Mark as invalid
  
  for (size_t i = 0; i < cuentas_num; i++) { // loop through list of accounts to match id
    if (cuentas[i].id == cuenta_id) {        // check if current id matches user input
      //printf("DEBUG: ID: %d | Titular: '%s' | Saldo: %.2f | Operaciones: %d\n",
      //  cuentas[i].id, cuentas[i].titular, cuentas[i].saldo, cuentas[i].operaciones);
      cuenta_selec = cuentas[i];            // if so then store address of matching account
      break;                                 // stop search when found
    }
  }
  
  //printf("DEBUG: Cuenta Selec ID: %d\n", cuenta_selec.id);

  free(cuentas); // free allocated memory

  if (cuenta_selec.id == -1) {                // if no matching account is found
    printf("Cuenta no encontrada\n"); // print error
    return;                           // exit function early
  }

  // named pipes (FIFOs) allow inter-process communication (IPC)
  char *fifo_path = "/tmp/banco_fifo"; // path to FIFO file
  mkfifo(fifo_path, 0666);             // create FIFO with read/write permission

  pid_t pid = fork();                 // duplicate parent process
  if (pid < 0) {                      // if process creation failed
      perror("Error creando fork\n"); // print error
      return;                         // exit function early
  }
    
    if (pid == 0) { // child process
      // open a new terminal and execute usuario  .c
      // x-terminal-emulator is a symbolic link to the systems default terminal
      execlp("x-terminal-emulator", "x-terminal-emulator", "-e", "./usuario", (char *)NULL);
      perror("Error ejecutando hello.c en una nueva terminal\n"); // if execlp fails print error
      exit(1);                                                    // and then exit
    } else {        // parent process
      FILE *fifo = fopen(fifo_path, "w"); // open named pipe in write mode
      if (!fifo) {                                    // if fopen() fails
        perror("Error abriendo FIFO para escritura"); // print error message
        return;                                       // exit function early
      }
      
      //printf("DEBUG: Escribiendo en FIFO: '%d \"%s\" %.2f'\n",
      //  cuenta_selec.id, cuenta_selec.titular, cuenta_selec.saldo);
      // write account details to named pipe
      fprintf(fifo, "%d \"%s\" %.2f\n", cuenta_selec.id, cuenta_selec.titular, cuenta_selec.saldo);
      fflush(fifo); // ensure all data is written immediately
      fclose(fifo); // close named to indicate writing is complete
    }
}

/*
USAGE: 
  main user menu that the program constantly loops back to
INPUT: 
  struct Cuenta *cuentas == a pointer to the array containing all imported cuentas
  size_t cuentas_num     == the amount of records imported from cuenta.dat
OUTPUT:
  void
*/
void menu() {
  int choice; // users menu choice
  
  while (1) { // infinite loop to stay in menu until user chooses exit
    printf("\n===== Menu Principal =====\n");
    printf("1. Iniciar Sesion\n");
    printf("2. Salir\n");
    printf("3. Imprimir Cuentas\n");
    printf("4. Imprimir Config\n");
    printf("Seleccione una opcion: ");
    
    scanf("%d", &choice); // read user input
    system("clear"); // clear terminal
    
    switch (choice) {
      case 1:
        cuentaLogin(); // call login function
        break;
      case 2:
        printf("Saliendo del programa...\n");
        return; // exit menu loop
      case 3:
        cuentasPrint();
        break;
      case 4:
        configPrint();
        break;
      default:
        printf("Opcion no valida, intente de nuevo.\n");
        break;
    }
  }
}

int main() {
  system("clear"); // clear terminal
  
  // void monitorStart()
  monitorStart();
  
  pid_t pid = fork();
  if (pid == 0) { // child process
    receiveAlerts();  // receive and display alerts
    exit(0);
  }
  
  // void menu()
  menu();

  // free allocated memory
  sem_unlink("/cuentas_sem");
  
  printf("\nPrograma termino sin problemas...\n\n");
  return 0;
}
