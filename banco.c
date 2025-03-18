#include <stdio.h>
#include <stdlib.h>

// define the Cuenta struct
struct Cuenta {
  int id;            // numero identificador de cuenta
  char titular [50]; // nombre del titular de cuenta
  float saldo;       // saldo de cuenta
  int operaciones;   // numero de operaciones de cuenta
};

int main() {

  // open cuentas.dat file in binary read mode
  FILE *fileCuentas = fopen("cuentas.dat", "rb");
  if (fileCuentas == NULL) {
    perror("Error abriendo cuentas.dat\n");
    return 1;  
  }
  
  // determine file size
  fseek(fileCuentas, 0, SEEK_END);        // moves fileCuentas to the end to get file size
  long file_size = ftell(fileCuentas);
  rewind(fileCuentas);                    // moves fileCuentas pointer back to beginning
  
  // calculates number of structs by dividing file size by struct size
  size_t num_records = file_size / sizeof(struct Cuenta); 
  if (num_records == 0) {
    perror("No se encontraron registros en cuentas.dat\n");
    return 1;
  }

  // allocate memory for Cuenta array based on number of records
  struct Cuenta *cuentas = malloc(num_records * sizeof(struct Cuenta));
  if (cuentas == NULL) {
    perror("No se pudo asignar memoria a las cuentas\n");
    fclose(fileCuentas);
    return 1;
  }
  
  // read all records into the dynamically allocated array
  size_t read_count = fread(cuentas, sizeof(struct Cuenta), num_records, fileCuentas);
  if (read_count != num_records) {
    perror("Error leyendo cuentas.dat\n");
  }
  
  fclose(fileCuentas);
  
  // loops through the array and prints each cuenta record
  printf("Cuentas almacenadas:\n");
  for (size_t i = 0; i < num_records; i++) {
    printf("ID: %d | Titular: %s | Saldo: %.2f | Operaciones: %d\n",
      cuentas[i].id, cuentas[i].titular, cuentas[i].saldo, cuentas[i].operaciones);
  }
  
  // free allocated memory
  free(cuentas);
  return 0;
}
