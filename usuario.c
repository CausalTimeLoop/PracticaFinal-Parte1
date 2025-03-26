#include <stdio.h>
#include <stdlib.h>

int opcion;
while (1) 
{
printf("1. Depósito\n2. Retiro\n3. Transferencia\n4. Consultar saldo\n5. Salir\n");
scanf("%d", &opcion);
switch (opcion) 
{
    case 1: realizar_deposito();
    break;
    case 2: realizar_retiro(); 
    break;
    case 3: realizar_transferencia(); 
    break;
    case 4: consultar_saldo(); 
    break;
    case 5: exit(0);
}
}
