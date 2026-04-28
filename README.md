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
  2- osservazioni delle famiglie

## Relazione

1. Descrizione delle caratteristiche del personale ottenuto;
2. Osservazioni sulle caratteristiche delle famiglie ottenute;
3. Spiegazione delle scelte fatte nella progettazione della strategia, con
   specifico riferimento alle caratteristiche del personale;
4. Spiegazione dei criteri usati per soddisfare i vincoli di tempo di
   completamento minore, di reputazione maggiore ed eventualmente di
   assenza di stanchezza alta;
5. Descrizione delle problematiche incontrate e delle soluzioni adottate;
6. Spiegazione degli apporti specifici di ogni membro del gruppo di
   studenti nello sviluppo del codice e della relazione.
