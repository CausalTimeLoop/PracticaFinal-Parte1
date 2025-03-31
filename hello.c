#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// define the Cuenta struct
struct Cuenta {
  int id;            // numero identificador de cuenta
  char titular [50]; // nombre del titular de cuenta
  float saldo;       // saldo de cuenta
  int operaciones;   // numero de operaciones de cuenta
};

int main() {
  char *fifo_path = "/tmp/banco_fifo"; // path to the fifo file

  printf("Esperando datos de la cuenta...\n");

  // named pipes (FIFOs) allow inter-process communication (IPC)
  FILE *fifo = fopen(fifo_path, "r");           // open named pipe in read mode
  if (!fifo) {                                  // if fopen() fails
    perror("Error abriendo FIFO para lectura"); // print error message
    return 1;                                   // exit function early
  }

  struct Cuenta cuenta; // cuenta struct to store the parsed data
  
  // read formatted data from the fifo, fscanf will block(waiting) if no data is available
  fscanf(fifo, "%d \"%49[^\"]\" %f", &cuenta.id, cuenta.titular, &cuenta.saldo);
  fclose(fifo); // close the fifo after reading

  // displays account 
  printf("Cuenta Iniciada:\n");
  printf("ID: %d\n", cuenta.id);
  printf("Titular: %s\n", cuenta.titular);
  printf("Saldo: %.2f\n", cuenta.saldo);

  // keep hello.c running
  while (1) {
      sleep(1);
  }

  return 0;
}
