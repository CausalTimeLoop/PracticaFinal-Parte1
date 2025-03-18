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
  // declaration default cuentas
  struct Cuenta cuentas[] = {
    {1001, "John Doe",       5000.00, 0},
    {1002, "Freddy Fazbear", 1987.00, 0},
    {1003, "Ariane Yeong",    512.00, 0}
  };
  
  // open .dat file in binary write mode
  FILE *file = fopen("cuentas.dat","wb");
  if (file == NULL) {
    perror("Error abriendo cuentas.dat\n");
    return 1;
  }
  
  // write cuentas to .dat in binary format
  size_t count = fwrite(cuentas, sizeof(struct Cuenta), 3, file);
  if (count != 3) {
    perror("Error escribiendo a cuentas.dat\n");
  }
  
  // close file
  fclose(file);
  printf("Cuentas iniciales generadas y guardadas!\n\n");
  
  return 0;
}
