#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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
void configPrint(struct Config *config) {
  printf("Configuración Cargada:\n");
  printf("LIMITE_RETIRO: %d\n",         config->LIMITE_RETIRO);
  printf("LIMITE_TRANSFERENCIA: %d\n",  config->LIMITE_TRANSFERENCIA);
  printf("UMBRAL_RETIROS: %d\n",        config->UMBRAL_RETIROS);
  printf("UMBRAL_TRANSFERENCIAS: %d\n", config->UMBRAL_TRANSFERENCIAS);
  printf("NUM_HILOS: %d\n",             config->NUM_HILOS);
  printf("ARCHIVO_CUENTAS: %s\n",       config->ARCHIVO_CUENTAS);
  printf("ARCHIVO_LOG: %s\n",           config->ARCHIVO_LOG);
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
struct Cuenta* cuentasImport(const char *file_name, size_t *cuentas_num) {
  FILE *cuentas_file = fopen("cuentas.dat", "rb"); // open cuentas.dat file in binary read mode
  if (cuentas_file == NULL) {                      // if the file does not exist or fails to open
    perror("Error abriendo cuentas.dat\n");        // print error message
    return NULL;                                   // and exit function early
  }
  
  fseek(cuentas_file, 0, SEEK_END);        // moves file pointer to the end
  long file_size = ftell(cuentas_file);    // get current position to determine file size
  rewind(cuentas_file);                    // moves file pointer back to beginning
  
  *cuentas_num = file_size / sizeof(struct Cuenta); // get number of structs by dividing file size by struct size 
  if (*cuentas_num == 0) {                                  // if no records are found
    perror("No se encontraron registros en cuentas.dat\n"); // print error message
    fclose(cuentas_file);                                   // close file
    return NULL;                                            // and exit function early
  }
  
  struct Cuenta *cuentas = malloc(*cuentas_num * sizeof(struct Cuenta)); // allocate memory for array of cuentas
  if (cuentas == NULL) {                                  // if malloc fails due to lack of memory
    perror("No se pudo asignar memoria a las cuentas\n"); // print message
    fclose(cuentas_file);                                 // close file
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
    return NULL;                           // exit function early
  }
  
  fclose(cuentas_file);
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
void cuentasPrint(struct Cuenta *cuentas, size_t cuentas_num) {
  if (cuentas == NULL || cuentas_num == 0) { // check if array is NULL or contains no accounts
    printf("No hay cuentas por mostrar\n");  // if so then print error message
    return;                                  // exit function early
  }
  
  printf("Cuentas almacenadas:\n");          // print the header
  for (size_t i = 0; i < cuentas_num; i++) { // loop through all acounts and print their details
    printf("ID: %d | Titular: %s | Saldo: %.2f | Operaciones: %d\n",
      cuentas[i].id, cuentas[i].titular, cuentas[i].saldo, cuentas[i].operaciones);
  }
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
void cuentaLogin(struct Cuenta *cuentas, size_t cuentas_num) {
  int cuenta_id;                    // variable to store account id
  printf("Ingrese su Cuenta ID: "); // prompt user to enter account id
  scanf("%d", &cuenta_id);          // read account id from user input
  
  struct Cuenta *cuenta_selec = NULL; // pointer to store the selected account
  
  for (size_t i = 0; i < cuentas_num; i++) { // loop through list of accounts to match id
    if (cuentas[i].id == cuenta_id) {        // check if current id matches user input
      cuenta_selec = &cuentas[i];            // if so then store address of matching account
      break;                                 // stop search when found
    }
  }
  
  if (!cuenta_selec) { // if no matching account is found inform of error and return
    printf("Cuenta no encontrada\n");
    return;
  }
  
  char command[256]; // buffer to store new terminal command
  
  sprintf(command,                                                 // construct new terminal command
          "gnome-terminal -- bash -c './hello %d \"%s\" %.2f %d'", // run ./hello and pass 4 arguments
          cuenta_selec->id,           // %d id as integer
          cuenta_selec->titular,      // \"%s\" name string quoted to handle spaces
          cuenta_selec->saldo,        // %.2f 2 decimal floating point balance
          cuenta_selec->operaciones); // %d operations as integer
  
  printf("Iniciando sesion...\n"); // inform user of succesful startup
  system(command);                 // execute command to open new terminal
  return; 
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
void menu(struct Cuenta *cuentas, size_t cuentas_num) {
  int choice; // users menu choice
  
  while (1) { // infinite loop to stay in menu until user chooses exit
    printf("\n===== Menu Principal =====\n");
    printf("1. Iniciar Sesion\n");
    printf("2. Salir\n");
    printf("Seleccione una opcion: ");
    
    scanf("%d", &choice); // read user input
    system("clear"); // clear terminal
    
    switch (choice) {
      case 1:
        cuentaLogin(cuentas, cuentas_num); // call login function
        break;
      case 2:
        printf("Saliendo del programa...\n");
        return; // exit menu loop
      default:
        printf("Opcion no valida, intente de nuevo.\n");
        break;
    }
  }
}

int main() {
  system("clear"); // clear terminal
  
  struct Config *config = configImport();
  if (config == NULL) {
    perror("Error en la funcion configImport\n");
    return 1;
  }
  // void configPrint(struct Config *config)
  //configPrint(config);
  
  // struct Cuenta* cuentasImport(size_t *cuentas_num)
  size_t cuentas_num;
  struct Cuenta *cuentas = cuentasImport(config->ARCHIVO_CUENTAS, &cuentas_num);
  if (cuentas == NULL) {
    perror("Error en la funcion cuentasImport\n");
    return 1;
  }
  // void cuentasPrint(struct Cuenta *cuentas, size_t cuentas_num)
  //cuentasPrint(cuentas, cuentas_num);
  
  // void menu(struct Cuenta *cuentas, size_t cuentas_num)
  //menu(cuentas, cuentas_num);
  
  // free allocated memory
  free(config);
  free(cuentas);
  printf("\nPrograma termino sin problemas...\n\n");
  return 0;
}
