# Sistemi Operativi II

```
Specifiche di progetto
```

```
Luca Geretti
luca.geretti@univr.it
```

```
Universit`a di Verona
Dipartimento di Informatica
```

```
2025/
```

# Panoramica

```
1 Introduzione
```

(^2) Dettagli di funzionamento
(^3) Interfacciamento
4 Verifica e valutazione
(^5) Materiale disponibile e consegna

# Introduzione

# Scenario: Trattoria

```
Ovvero un piccolo ristorante familiare con pochi tavoli e basse pretese.
Vi `e un personale della trattoria.
Vi sono multiple famiglie che rappresentano i clienti, in paziente coda
per ottenere uno dei pochi tavoli e ricevere il servizio.
```

# Obiettivo del progetto

```
Lo scenario `e una simulazione/gioco con:
giocatori (personale) in numero pari al numero di studenti del gruppo
+1;
tavoli in numero pari al numero di membri del personale +1;
avversari (famiglie) in numero totale pari al numero di tavoli per 3.
Gli studenti sviluppano un client C multi-threaded che controlla uno
ad uno i membri del personale della trattoria.
Il server pre-compilato, fornito agli studenti assieme all’header di
interfaccia, modella il comportamento di tutto ci`o che non `e il
personale.
```

# Versioni uniche del progetto per ogni gruppo

```
Ogni membro del personale ed ogni famiglia ha punteggi su un
insieme di parametri rilevanti per lo scenario.
Ogni gruppo di studenti ha una versione unica ma deterministica
dello scenario.
La versione dello scenario `e:
generata in modo pseudocasuale usando i numeri di matricola degli
studenti coinvolti;
si applica ai parametri e quindi
```

(^1) a come il personale `e in grado di svolgere il proprio compito, nonch ́e
(^2) a come le famiglie interagiscono col processo di ristorazione.

# Processo di ristorazione lato famiglia

```
Abbiamo le seguenti fasi per una nuova famiglia:
```

(^1) Ingresso in trattoria e presa di un tavolo appena liberato;
(^2) Attesa di un aiutante che pulisca il tavolo da piatti e sporcizia
precedenti;
(^3) Attesa di un cameriere per l’ordine;
(^4) Ordine del cibo;
(^5) Attesa della preparazione del cibo;
(^6) Attesa della consegna del cibo (perch ́e dalla cucina viene messo in
vista sul bancone);
(^7) Consumo del cibo;
(^8) Liberazione del tavolo ed attesa del turno in cassa;
(^9) Pagamento ed abbandono della trattoria.

# Processo di ristorazione lato personale

```
Per ogni famiglia abbiamo le seguenti attivit`a in ordine da dover compiere:
```

(^1) Pulire il tavolo;
(^2) Ricevere l’ordine;
(^3) Cucinare il cibo;
(^4) Consegnare il cibo;
(^5) Occuparsi del pagamento.
In pi`u di tanto in tanto `e necessario che qualcuno si occupi di lavare i
piatti, disponibili in quantit`a limitata (sufficiente per il numero di tavoli
presenti, nel caso di massima quantit`a di cibo ordinata).

# Come giudicare la qualit`a del processo

```
Per poter giudicare la bont`a del processo di ristorazione (e quindi il lavoro
degli studenti) verranno introdotte due metriche:
Profitto: inversamente proporzionale al tempo che la famiglia passa
nella trattoria; maggiore il tempo, minore il tasso di sostituzione dei
clienti al tavolo, minore il profitto;
Reputazione: relativa alle recensioni che le famiglie svolgono a seguito
della loro esperienza nella trattoria.
```

# Dettagli di funzionamento

# Principio di progettazione

## Non tutti i dettagli sono conoscibili dagli studenti

```
Le informazioni accessibili al client sono perlopi`u parziali e qualitative:
questo obbliga nello sviluppo del client ad osservare manualmente il
comportamento restituito dal server ed a pianificare di conseguenza la
risposta.
```

# Il personale - Abilit`a e tratti

```
Ogni membro del personale presenta 4 abilit`a e 4 tratti con valori casuali
ma bilanciati (la somma delle abilit`a e la somma dei tratti sono costanti).
Per ognuna `e possibile sapere solo se `e Bassa, Media o Alta.
```

## Abilit`a

```
Cameriere: prende gli ordini e consegna il cibo;
Aiutante: pulisce/sparecchia i tavoli e lava i piatti;
Cuoco: cucina;
Cassiere: gestisce i pagamenti.
```

## Tratti

```
Pazienza: si applica nella fase di ordine;
Socievolezza: si applica nella fase di pagamento;
Professionalit`a: si applica nelle fasi di ordine e di cucina;
Resistenza: si applica a qualunque ruolo svolto (dettagli in seguito).
```

# Il personale - Come usare abilit`a e tratti

## Abilit`a

```
Maggiore l’abilit`a, pi`u veloce `e il completamento del compito relativo al
ruolo a cui il membro del personale `e assegnato.
```

## Tratti

```
La pazienza, socievolezza e professionalit`a influenzano la qualit`a della
relazione con una famiglia e di conseguenza la valutazione che
quest’ultima vi d`a alla fine.
La resistenza si riferisce a quanto un membro del personale riesce
efficacemente a svolgere lo stesso ruolo per lungo tempo. Una bassa
resistenza si traduce in un decadimento pi`u rapido della abilit`a effettiva in
quel ruolo e quindi un tempo maggiore nel completare un compito.
```

# Le famiglie - Pretese e caratteristiche

```
Ogni famiglia presenta 4 pretese e 4 caratteristiche, i primi con valori
casuali ma bilanciati, i secondi casuali ma non bilanciati. Salvo alcune
eccezioni, non `e possibile conoscere nemmeno il livello di tali aspetti.
```

## Pretese

```
Tempo di attesa: sulla velocit`a di servizio in generale;
Qualit`a del cibo: sul cibo servito;
Qualit`a del servizio: sull’ordine e sulla consegna;
Cortesia: sull’ordine e sul pagamento.
```

## Caratteristiche

```
Velocit`a ad ordinare: quanto la famiglia perde tempo nel fare l’ordine;
Cibo ordinato: quanto cibo ordina;
Velocit`a a consumare: quanto veloce consuma il cibo^1 ;
Pulizia personale: quanto sporca consumando il cibo^1.
```

(^1) E comunque anche proporzionale al cibo ordinato.`

# Le famiglie - Come conoscere pretese e caratteristiche

## Pretese

```
All’uscita dalla trattoria la famiglia effettua una recensione fornendo un
giudizio (positivo, negativo o neutrale) su ognuno dei quattro aspetti
(tempo, cibo, servizio, cortesia). Ogni giudizio `e oggettivo ed emerge dal
confronto con la qualit`a che avremmo con valori medi di abilit`a e tratti del
personale allocato. La famiglia poi produce una valutazione qualitativa
finale in cui pesa questi giudizi secondo le proprie pretese soggettive.
```

## Caratteristiche

```
Le velocit`a ad ordinare/consumare possono essere osservate
approssimativamente dal tempo simulato richiesto per tali fasi;
Il cibo ordinato e la pulizia personale possono essere osservati
(qualitativamente) dopo l’ordine e dopo il consumo, rispettivamente.
```

# Interfacciamento

# Interfacciamento a grandi linee

## Simulazione di una istanza

```
La comunicazione programmata del client con il server avviene via IPC
System V tramite memorie condivise ed una coda di messaggi. Vanno
usati semafori IPC System V per sincronizzare l’accesso dove necessario.
```

## Ciclo di vita delle istanze

```
La comunicazione programmata del client con il server inizia tramite un
messaggio di saluto dal client, a cui segue dal server un messaggio di
benvenuto con le informazioni generali necessarie. Il server invia in ordine
una o pi`u istanze e notifica il completamento di ognuna nonch ́e la
terminazione del client.
```

# Simulazione - Memoria condivisa

## Come coordinare i membri del personale

```
Vi `e una memoria condivisa per la lavagna dove ogni membro pu`o andare
a scrivere chi si occupa di fare il cuoco, il lavapiatti ed il cassiere, e chi si
occupa di pulire, ordinare o servire ad ogni specifico tavolo.
```

## Come osservare lo stato di sala e cucina

```
Vi `e una memoria condivisa per entrambe, da intendersi in sola lettura dal
client. La prima indica qualitativamente la quantit`a di sporcizia e la
quantit`a di cibo relativi al tavolo. La seconda indica qualitativamente la
quantit`a di piatti puliti e di piatti sporchi presenti, il numero di ordini
pendenti e se il cibo di un determinato tavolo `e pronto per la consegna.
```

## Come osservare la stato della cassa

```
Vi `e una memoria condivisa da intendersi in sola lettura dal client. Essa
espone il numero di pagamenti pendenti.
```

# Simulazione - Coda di messaggi

## Come osservare la stanchezza del personale

```
Ogni volta che un membro del personale incrementa il livello qualitativo di
stanchezza, un messaggio viene mandato in una coda comune. L’mtype
del messaggio `e pari all’identificativo numerico del membro del personale +
1 (perch ́e mtype deve essere>0). Questo permette ad ogni filo di estrarre
solo i messaggi pertinenti. Inoltre alla fine di ogni istanza per ogni membro
del personale viene stampata la stanchezza nei ruoli in cui `e presente.
```

# Simulazione - Lato server

## Sala

```
Ogni tavolo `e gestito da un processo separato del server: appena il tavolo
`e libero dalla famiglia precedente, vi associa la prossima famiglia ed avanza
nelle fasi di (1) pulizia, (2) ordine cibo, (3) consegna cibo e (4) consumo
cibo. Le prime tre fasi non avanzano se non c’`e un membro del personale
che se ne occupi. Quando una fase inizia, essa procede per la durata
relativa fino a completamento (nessuna prelazione).
```

## Cucina

```
Vi sono un processo server per preparazione cibo ed uno per lavaggio
piatti. Il primo prepara una ad una le ordinazioni in ordine di arrivo, il
secondo lava i piatti sporchi a blocchi del 25% dei piatti totali disponibili.
```

## Cassa

```
Singolo processo server che gestisce le famiglie in ordine di arrivo.
```

# Cosa deve fare il client

```
Sviluppare due strategie specifiche, selezionabili
Strategia profitto: minimizzare la media del tempo totale speso da una
famiglia nella trattoria;
Strategia reputazione: massimizzare la media delle valutazioni
complessive delle famiglie.
Implementare un filo distinto per ogni membro del personale
Ogni filo prende decisioni operative (cambio di ruolo) per la strategia
interagendo con il server tramite una API fornita.
```

```
Per il resto l’implementazione `e completamente libera: `e sufficiente che
l’interazione con il server porti (per entrambe le strategie) al
completamento della gestione di tutte le famiglie.
```

# Interfacciamento con il server - Modalit`a

```
Il server pu`o operare in due modalit`a: normale e verifica.
Normale: viene eseguita una unica istanza di simulazione, con
possibilit`a di terminazione automatica del client alla fine
Il client nel messaggio di saluto deve specificare la strategia usata;
Il server stampa gli eventi che descrivono l’andamento della simulazione
della istanza.
Flag opzionale --speed <N> ed un valore intero (predefinito: 1) per
velocizzare di un fattore N la simulazione (N.B.: pu`o influenzare la
sincronizzazione in base al comportamento del client)
Verifica (flag --verify): vengono eseguite multiple istanze di
simulazione per poter recuperare tutti i dati per verificare in modo
programmatico il comportamento del client.
La strategia specificata dal client viene ignorata;
Flag opzionale aggiuntivo --verbose per stampare come in modalit`a
normale; normalmente nascosto a causa della grande quantit`a di
istanze eseguite.
```

# Interfacciamento con il server - Protocollo

```
Le fasi sono le seguenti:
```

(^1) Il client invia un messaggio di saluto con l’insieme dei numeri di
matricola degli studenti e l’eventuale strategia da imporre (se non
siamo in modalit`a di verifica)
(^2) Il server risponde con un messaggio di benvenuto con l’insieme delle
informazioni (qualitative) sui membri del personale
(^3) Il server manda, una alla volta, le istanze da eseguire
Ogni istanza specifica velocit`a, strategia usata e numero di famiglie
(^4) Quando una istanza viene completata, il server manda un messaggio
di completamento con i risultati di media di tempo trascorso dalle
famiglie e media di valutazioni delle famiglie
(^5) Quando tutte le istanze previste sono state completate, il server
manda un messaggio di fine per la terminazione del client

# Visione schematica interfacciamento

```
Client (C, multi-filo)• Un filo per membro del personale
```

- Decide cambi di ruolo
- Pu`o adottare due strategie

```
Gestione dello statoMemorie condivise
```

- • Lavagna (scrittura assegnazioni ruoli) Stato sala (solo lettura)
- • Stato cucina (solo lettura) Stato cassa (solo lettura)
  Coda di messaggi
- Notifiche stanchezza

```
Server (precompilato)• Simula tutto ci`o che non `e personale
```

- atti, cassa Processi: sala/tavoli, cucina, lavapi-
- Gestisce istanze e risultati

```
Saluto (matricole, strategia)
```

- Benvenuto (info qualitative personale)• Fine simulazione
- Inizio istanza (velocit`a, strategia, numero famiglie)• Fine istanza (metriche)

```
R/W stato, synch
```

```
Ciclo di vita delle istanze
1 Il server invia una istanza (parametri: velocit`a, strategia usata, numero famiglie);
2 Il client coordina i fili via IPC (lavagna + osservazione stato);
```

(^3) Il server invia completamento istanza (medie: tempo e valutazioni);
(^4) Dopo tutte le istanze, il server invia messaggio di fine (terminazione client).

# Verifica e valutazione

# Valutazione del modulo in generale

```
Il progetto arriva fino ad un potenziale di 33/30 punti. Tuttavia il voto
sulle domande dello scritto va a pesare i punti sopra il 18, quindi `e questo
extra (da 0 a 15) che conta veramente nel voto finale del corso. La
formula `e:
```

```
V = ceiling ((ST+ 18 + SL/30(P− 18))/2)
ossia la media fra il voto totale dello scritto STed il voto di progetto,
ottenuto pesando i punti del progetto oltre la sufficienza P (da 0 a 15
appunto) moltiplicati per il voto della parte dello scritto di laboratorio SL
diviso 30. Il tutto `e arrotondato per eccesso.
```

# Valutazione del progetto

```
In ogni caso il progetto deve raggiungere la sufficienza.
Il voto di partenza `e determinato da una valutazione (automatica) del
software sviluppato, che pu`o essere controllata dagli studenti in
qualunque momento usando la modalit`a di verifica del server;
A questo si sottrae la valutazione della relazione del lavoro svolto, che
pu`o confermare il voto del software o decrementarlo a discrezione del
docente. Maggiori dettagli pi`u avanti.
Nel seguito verranno indicati i requisiti software per la sufficienza, per un
voto potenziale di 30 e per arrivare a 33.
```

# Esecuzioni del client effettuate per la valutazione

```
Il server in modalit`a verifica effettua 100 esecuzioni per entrambe le
strategie, a velocit`a 1000, per i seguenti casi:
```

(^1) Numero di famiglie nominale (3 volte il numero di tavoli);
(^2) Numero di famiglie doppio (6 volte il numero di tavoli).
La variante con il doppio di famiglie serve a verificare una eventuale
implementazione non-dedicata ma `e considerata opzionale e dunque `e
pensata principalmente per superare una votazione di 30.

# Verifiche per la sufficienza

(^1) Completamento per profitto e per reputazione;
(^2) Strategia profitto: tempo di completamento minore della strategia
reputazione in oltre il 50% dei casi;
(^3) Strategia reputazione: punteggio migliore della strategia profitto in
oltre il 50% dei casi.

## Perch ́e una valutazione statistica?

```
Lo schedulatore pu`o influenzare l’ordine di assegnazione dei ruoli e quindi
impattare la strategia. Anche la velocizzazione estrema pu`o avere un
impatto in base a come `e realizzata la strategia. Una buona strategia `e
perlopi`u invariante alla velocit`a di simulazione (il cui valore `e accessibile
dal client) ed alla schedulazione.
```

# Verifiche per punti extra fino a 12

(^1) (2.5 punti) Strategia profitto: tempo di completamento minore
della strategia reputazione in oltre il 75% dei casi;
(^2) (2.5 punti) Strategia reputazione: punteggio maggiore della
strategia profitto in oltre il 75% dei casi.
(^3) (2 punti) Strategia profitto: tempo di completamento minore della
strategia reputazione in oltre il 90% dei casi;
(^4) (2 punti) Strategia reputazione: punteggio maggiore della strategia
profitto in oltre il 90% dei casi.
(^5) (1.5 punti) Strategia profitto: nessuna stanchezza alta in nessun
ruolo per nessun membro del personale in oltre il 90% dei casi;
(^6) (1.5 punti) Strategia reputazione: nessuna stanchezza alta in
nessun ruolo per nessun membro del personale in oltre il 90% dei casi.

# Verifiche per punti extra fino a 15

```
Svolte con un numero di famiglie doppio: poich ́e gli studenti non possono
simulare questa situazione, le strategie devono essere sufficientemente
generali e scalabili.
Le verifiche sono uguali a quelle per i punti extra fino a 12, ma con punti:
```

(^1) (0.75 punti) Profitto: tempo minore di reputazione> 75%;
(^2) (0.75 punti) Reputazione: punteggio maggiore di profitto> 75%;
(^3) (0.5 punti) Profitto: tempo minore di reputazione> 90%;
(^4) (0.5 punti) Reputazione: punteggio maggiore di profitto> 90%;
(^5) (0.25 punti) Profitto: nessuna stanchezza alta> 90%;
(^6) (0.25 punti) Reputazione: nessuna stanchezza alta> 90%.

# Indicazioni pratiche per un buon esito

```
Sfruttare le informazioni esposte dalla interfaccia, fare tentativi ed
osservare il comportamento (trial-and-error) per eventualmente
dedurre informazioni nascoste;
Progettare le due strategie in modo chiaramente distinguibile:
Profitto: d`a priorit`a al tempo di completamento dei compiti;
Reputazione: d`a priorit`a alla qualit`a del servizio (accettando tempi
peggiori).
Rispettare la stanchezza del personale.
Puntare alla robustezza: nonostante le 100 istanze, i risultati possono
essere molto variabili se non si privilegia la comunicazione fra i
membri del personale; si consideri che la verifica automatica del client
verr`a svolta una volta sola dal docente.
```

# La relazione

```
La relazione non aggiunge punti alla valutazione del software, ma pu`o
sottrarne.
I requisiti per una relazione con penalit`a nulle sul voto sono:
```

(^1) Descrizione delle caratteristiche del personale ottenuto;
(^2) Osservazioni sulle caratteristiche delle famiglie ottenute;
(^3) Spiegazione delle scelte fatte nella progettazione della strategia, con
specifico riferimento alle caratteristiche del personale;
(^4) Spiegazione dei criteri usati per soddisfare i vincoli di tempo di
completamento minore, di reputazione maggiore ed eventualmente di
assenza di stanchezza alta;
(^5) Descrizione delle problematiche incontrate e delle soluzioni adottate;
(^6) Spiegazione degli apporti specifici di ogni membro del gruppo di
studenti nello sviluppo del codice e della relazione.
L’assenza completa di uno o pi`u di questi aspetti pu`o impattare molto
negativamente sulla valutazione finale.

# Materiale disponibile e consegna

# Materiale messo a disposizione degli studenti

(^1) Progetto CMake vuoto con gli header di interfaccia
(^2) Eseguibile del server
(^3) Eseguibile di un client di esempio

# Progetto CMake

```
L’interfaccia `e fornita tramite due header:
scenario.h: fornisce le enumerazioni utilizzate nello scenario;
ipc.h: (include scenario.h) fornisce le strutture dati per la
comunicazione con il server.
```

```
Per il resto il client `e volutamente vuoto: finch ́e viene rispettata
l’interfaccia, qualunque realizzazione `e valida.
```

# Eseguibile del server

```
Correntemente fornito per le seguenti piattaforme:
macOS arm64 (indicato con ’macOS’ nel nome del file)^1 ;
Linux x86-64 (indicato con ’Linux’ nel nome del file).
Per quanto al momento non garantito, si prevede di verificare il supporto a
WSL (Windows Subsystem for Linux) x86-64.
```

```
Il server stampa testo in inglese per maggiore accessibilit`a agli studenti
stranieri.
```

```
All’esecuzione, il server mostra la versione dell’eseguibile: si prevedono
aggiornamenti nel caso emergano migliorie o correzioni da implementare.
Verr`a mantenuta su Moodle la versione pi`u recente per ogni piattaforma.
L’interfaccia non verr`a mai modificata.
```

(^1) Per macOS pu`o essere necessario dare il permesso di esecuzione degli eseguibili:
dopo averli lanciati una volta, andare nelle Impostazioni di Sistema sotto Privacy e
sicurezza, sul fondo della pagina abilitare gli eseguibili nella sezione Sicurezza.

# Eseguibile di un client di esempio

```
Fornito per le stesse piattaforme del server;
Si tratta di circa 1000 righe di codice tutto incluso;
Dimostra il comportamento per un gruppo fisso di 3 matricole;
In verit`a le strategie sviluppate non sono specializzate e funzionano per
1-3 matricole con qualunque numero di famiglie;
Con il flag --strategy <profit|reputation> si pu`o scegliere la
strategia;
Stampa solo i messaggi di interscambio con il server;
Restituisce successo tipicamente per tutti i controlli nella verifica.
```

# Invio del progetto

```
Prima di inviare il progetto:
Verificare la compilazione da zero del codice;
Verificare l’esecuzione prevista in modalit`a verifica del server;
Prima di creare l’archivio compresso, rimuovere file estranei (test
interni, cartella di build, ecc.): nessun file fuori dal codice per
l’eseguibile del client verr`a considerato nella valutazione.
Il progetto completato va inviato via email a luca.geretti@univr.it da un
solo membro del gruppo, mettendo in copia i restanti membri. Il progetto
deve essere un archivio contenente la relazione e la base di codice del
progetto CMake. Per quanto un invio di un allegato non sia proibito, si
considera preferenziale un collegamento OneDrive.
Il limite di validit`a del progetto `e il 31 gennaio 2027;
La consegna pu`o avvenire in qualunque momento.
```

# Verifica e consegna in presenza del docente

```
Poich ́e l’esecuzione del progetto in modalit`a verifica viene svolta una
singola volta da parte del docente, il gruppo di studenti se lo desidera
pu`o concordare un incontro con il docente in cui la verifica viene eseguita
su un laptop scelto dal gruppo in presenza del docente. Il risultato,
qualora sufficiente, viene annotato, dopodich ́e in presenza l’archivio del
progetto viene inviato via email al docente.
```
