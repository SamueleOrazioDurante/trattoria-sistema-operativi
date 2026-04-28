# trattoria-sistema-operativi

Persona 1

- main.h
   1- inizializza i thread del personale
- server.h:
   1- messaggio hello con matricole e strategia
   2- gestire messaggio di benvenuto con info personale
   3- comunicazione server
- ipc.h
   1- memoria condivisa (lavagna, messaggi, sala, cucina, cassa)

Persona 2

- worker.h 
  1- esecuzione dei compiti
  2- scrittura sulla memoria condivisa (lavagna)
  3- gestione della stanchezza
  4- chiama strategies per gestire cosa fare

Persona 3

- strategies.c
  1- logica di gestione dei compiti
  
