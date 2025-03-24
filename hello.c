#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// define the Cuenta struct
struct Cuenta {
  int id;            // numero identificador de cuenta
  char titular [50]; // nombre del titular de cuenta
  float saldo;       // saldo de cuenta
  int operaciones;   // numero de operaciones de cuenta
};

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso incorrecto. Ejecutar desde banco.c\n");
        getchar();
        return 1;
    }
  

  struct Cuenta cuenta;
  cuenta.id = atoi(argv[1]);
  strcpy(cuenta.titular, argv[2]); 
  cuenta.saldo = atof(argv[3]);
  cuenta.operaciones = atoi(argv[4]);
  
  printf("\n=== Información de la Cuenta ===\n");
  printf("ID: %d\n", cuenta.id);
  printf("Titular: %s\n", cuenta.titular);
  printf("Saldo: %.2f\n", cuenta.saldo);
  printf("Operaciones: %d\n", cuenta.operaciones);
  printf("===============================\n");

  //code
  printf("Hello world\n");
  getchar();
  return 0;
}
