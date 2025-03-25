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
/*
argc   == (argument count): number of arguments passed to program
argv[] == (argument vector): array of strings containing the actual arguments
argv[0]: current program nameprogram name (hello.c)
argv[1]: Cuenta id
argv[2]: Cuenta titular
argv[3]: Cuenta saldo
argv[4]: Cuenta operaciones
*/
int main(int argc, char *argv[]) {
    if (argc != 5) { // if all arguments arent correctly passed
        printf("Uso incorrecto. Ejecutar desde banco.c\n"); // print error message
        getchar();   // wait for any keypress to be able to see the terminal
        return 1;    // exit with error code and close terminal
    }
  

  struct Cuenta cuenta; // cuenta struct to store the parsed data

  cuenta.id = atoi(argv[1]);          // convert (id)          from string to integer
  strcpy(cuenta.titular, argv[2]);    // copy    (titular)     into struct
  cuenta.saldo = atof(argv[3]);       // convert (saldo)       from string to float
  cuenta.operaciones = atoi(argv[4]); // convert (operaciones) from string to integer
  
  printf("\n=== Información de la Cuenta ===\n");
  printf("ID: %d\n",          cuenta.id);
  printf("Titular: %s\n",     cuenta.titular);
  printf("Saldo: %.2f\n",     cuenta.saldo);
  printf("Operaciones: %d\n", cuenta.operaciones);
  printf("===============================\n");

  //code
  printf("Hello world\n");
  getchar();
  return 0;
}
