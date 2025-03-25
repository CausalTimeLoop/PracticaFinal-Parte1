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

/*
USAGE: 
  import cuentas.dat into a Cuenta array for the program to use
INPUT: 
  size_t *cuentas_num    == a pointer to the amount of records imported from cuenta.dat
OUTPUT:
  struct Cuenta *cuentas == a pointer to the array containing all imported cuentas
*/
struct Cuenta* cuentasImport(size_t *cuentas_num) {
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

int main() {
  
  // struct Cuenta* cuentasImport(size_t *cuentas_num)
  size_t cuentas_num;
  struct Cuenta *cuentas = cuentasImport(&cuentas_num);
  if (cuentas == NULL) {
    perror("Error en la funcion cuentasImport\n");
    return 1;
  }
  
  // void cuentasPrint(struct Cuenta *cuentas, size_t cuentas_num)
  cuentaLogin(cuentas, cuentas_num);
  
  // free allocated memory
  free(cuentas);
  printf("Programa Termino sin problemas...\n");
  return 0;
}
