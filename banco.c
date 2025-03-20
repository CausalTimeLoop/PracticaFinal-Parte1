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

// import cuentas.dat into a Cuenta array for the program to use
struct Cuenta* cuentasImport(size_t *cuentas_num) {

  // open cuentas.dat file in binary read mode
  FILE *cuentas_file = fopen("cuentas.dat", "rb");
  if (cuentas_file == NULL) {
    perror("Error abriendo cuentas.dat\n");
    return NULL;  
  }
  
  // determine file size
  fseek(cuentas_file, 0, SEEK_END);        // moves cuentas_file to the end to get file size
  long file_size = ftell(cuentas_file);
  rewind(cuentas_file);                    // moves cuentas_file pointer back to beginning
  
  // calculates number of structs by dividing file size by struct size
  *cuentas_num = file_size / sizeof(struct Cuenta); 
  if (*cuentas_num == 0) {
    perror("No se encontraron registros en cuentas.dat\n");
    fclose(cuentas_file);
    return NULL;
  }
  
  // allocate memory for Cuenta array based on number of records
  struct Cuenta *cuentas = malloc(*cuentas_num * sizeof(struct Cuenta));
  if (cuentas == NULL) {
    perror("No se pudo asignar memoria a las cuentas\n");
    fclose(cuentas_file);
    return NULL;
  }
  
  // read all records into the dynamically allocated array
  size_t read_count = fread(cuentas, sizeof(struct Cuenta), *cuentas_num, cuentas_file);
  if (read_count != *cuentas_num) {
    perror("Error leyendo cuentas.dat\n");
    free(cuentas);
    fclose(cuentas_file);
    return NULL;
  }
  
  fclose(cuentas_file);
  return cuentas;
}

void cuentasPrint(struct Cuenta *cuentas, size_t cuentas_num) {
  if (cuentas == NULL || cuentas_num == 0) {
    printf("No hay cuentas por mostrar\n");
    return;
  }
  
  // loops through the array and prints each cuenta record
  printf("Cuentas almacenadas:\n");
  for (size_t i = 0; i < cuentas_num; i++) {
    printf("ID: %d | Titular: %s | Saldo: %.2f | Operaciones: %d\n",
      cuentas[i].id, cuentas[i].titular, cuentas[i].saldo, cuentas[i].operaciones);
  }
}

void cuentaLogin(struct Cuenta *cuentas, size_t cuentas_num) {
  
  int cuenta_id;
  printf("Ingrese su Cuenta ID: ");
  scanf("%d", &cuenta_id);
  
  struct Cuenta *cuenta_selec = NULL;
  for (size_t i = 0; i < cuentas_num; i++) {
    if (cuentas[i].id == cuenta_id) {
      cuenta_selec = &cuentas[i];
      break;
    }
  }
  
  if (!cuenta_selec) {
    printf("Cuenta no encontrada\n");
    return;
  }
  
  char command[256];
  sprintf(command, "gnome-terminal -- ./hello %d \ "%s\" %.2f  %d",
  cuenta_selec->id, cuenta_selec->titular,
  cuenta_selec->saldo, cuenta_selec->operaciones);
  
  printf("Iniciando sesion...\n");
  system(command);
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
  
  // free allocated memory
  free(cuentas);
  return 0;
}
