# Relazione Tecnica - Trattoria Client

Questo documento descrive le caratteristiche della simulazione e le scelte progettuali implementate nel client della Trattoria.

## 1. Descrizione delle caratteristiche del personale

Il personale assegnato alla trattoria presenta un mix eterogeneo di abilità e tratti caratteriali, riassunto nella seguente tabella:

| Membro       | Abilità (Skill)                                                         | Tratti Caratteriali                                                                    | Osservazioni e Ruolo Ideale                                                                                                                                                |
| :----------- | :---------------------------------------------------------------------- | :------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Giulia**   | Cameriere: `MED`<br>Cuoco: `MED`<br>Aiutante: `MED`<br>Cassiere: `MED`  | Pazienza: `HIGH`<br>Socievolezza: `LOW`<br>Professionalità: `LOW`<br>Resistenza: `MED` | Personale versatile ma carente in socievolezza e professionalità. È adatta a ruoli di background (cucina, lavapiatti), coprendo i ruoli di contatto solo in emergenza.     |
| **Matteo**   | Cameriere: `MED`<br>Cuoco: `LOW`<br>Aiutante: `MED`<br>Cassiere: `HIGH` | Pazienza: `MED`<br>Socievolezza: `MED`<br>Professionalità: `LOW`<br>Resistenza: `MED`  | Specialista in Cassa (`HIGH`), ma scarso in cucina. I suoi tratti bilanciati lo rendono il candidato ideale per la gestione dei pagamenti e i ruoli operativi di supporto. |
| **Gabriele** | Cameriere: `HIGH`<br>Cuoco: `MED`<br>Aiutante: `MED`<br>Cassiere: `MED` | Pazienza: `HIGH`<br>Socievolezza: `MED`<br>Professionalità: `LOW`<br>Resistenza: `MED` | Eccellente Cameriere (`HIGH`) con alta Pazienza. È uno dei pilastri per la strategia _Reputation_ grazie all'abilità nel servire ai tavoli.                                |
| **Beatrice** | Cameriere: `HIGH`<br>Cuoco: `MED`<br>Aiutante: `LOW`<br>Cassiere: `MED` | Pazienza: `HIGH`<br>Socievolezza: `LOW`<br>Professionalità: `MED`<br>Resistenza: `LOW` | Ottima Cameriera (`HIGH`) con buona professionalità, ma con Resistenza `LOW`. Richiede rotazioni frequenti (es. verso la cassa o riposo) per evitare stanchezza critica.   |

## 2. Osservazioni sulle caratteristiche delle famiglie

L'analisi dei log generati durante le simulazioni ha permesso di isolare tre macro-comportamenti delle famiglie clienti, i quali influenzano pesantemente le metriche di _Profitto_ e _Reputazione_. Di seguito le osservazioni strutturate:

| Vettore Comportamentale                    | Impatto sul Sistema (Simulazione)                                                                                                                                                                                               | Contromisura Adottata nel Client                                                                                                                                       |
| :----------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Volume degli Ordini (Dimensione Pasto)** | Numerose famiglie (es. _Rossi, Ferrari_) generano frequentemente ordini di dimensione `LARGE`. Questo satura rapidamente la coda ordini e prosciuga lo stack dei piatti puliti (`clean_plates`).                                | Implementazione di un trigger preventivo sul ruolo `ROLE_DISHWASHER` per evitare il blocco architetturale della cucina.                                                |
| **Sensibilità ai Tempi di Attesa**         | La tolleranza all'attesa è limitata. Il ritardo nella consegna del cibo o nel processamento del pagamento alla cassa genera rapidamente feedback negativi (`wait='negative'`) che affossano il punteggio finale.                | Nella variante _Profit_, il ruolo `ROLE_CASHIER` e il `ROLE_WAITER` vengono attivati immediatamente con un approccio _greedy_ per massimizzare il turnover dei tavoli. |
| **Sensibilità alla Cortesia**              | L'algoritmo di valutazione del server è compensativo: un servizio erogato da personale con tratto _Pazienza_ e _Professionalità_ elevato (es. Gabriele) mitiga le penalità di un'attesa prolungata (es. `courtesy='positive'`). | Nella variante _Reputation_, si applica il _pinning_ esclusivo del personale migliore ai ruoli di contatto, relegando gli altri in cucina.                             |

## 3. Scelte nella progettazione della strategia

La logica decisionale (modulo `strategy.c`) implementa un'architettura duale per adattarsi ai requisiti della simulazione. Entrambe le strategie condividono un meccanismo di **Persistenza del Ruolo**: un thread assegnato a compiti prolungati (es. cucina o lavaggio piatti) mantiene il blocco sul ruolo fino al completamento del task locale, garantendo continuità operativa ed evitando contest-switch eccessivi.

| Variante       | Architettura di Assegnazione       | Logica Operativa                                                                                                                                                              | Obiettivo Tecnico                                                    |
| :------------- | :--------------------------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------- |
| **Profit**     | **Dinamica a Priorità (Pipeline)** | Qualsiasi worker disponibile viene assegnato al primo task critico libero (es. _sblocco lavapiatti > servire cibo > cucinare_). I tratti e le skill passano in secondo piano. | Massimizzare il throughput e minimizzare i tempi morti di sistema.   |
| **Reputation** | **Statica (Hardcoded Binding)**    | I worker sono ancorati ai ruoli ideali tramite ID: Giulia (Cucina), Matteo (Cassa), Gabriele/Beatrice (Sala).                                                                 | Massimizzare la _qualità del servizio_ a discapito del parallelismo. |

## 4. Criteri per il soddisfacimento dei vincoli

I vincoli di progetto sono stati approcciati combinando controlli di stato continui e rotazioni, in modo da soddisfare rigorosamente i tre requisiti fondamentali richiesti dalla simulazione:

| Vincolo Richiesto                                                      | Metodologia di Risoluzione (Implementazione)                                                                                                                                                      |
| :--------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Tempo di completamento minore**<br>`time(profit) < time(reputation)` | Identificazione proattiva dei colli di bottiglia (es. attivazione preventiva del lavapiatti per non fermare il _cook_). Assegnazione puramente avida (_greedy_) nella strategia Profit.           |
| **Reputazione maggiore**<br>`score(reputation) > score(profit)`        | Isolamento dei worker ad alto potenziale di contatto (Pazienza/Professionalità `HIGH`) esclusivamente nei ruoli operativi di Cameriere e Cassiere durante la strategia Reputation.                |
| **Assenza di stanchezza alta**<br>`Nessun worker HIGH`                 | Lettura periodica della coda messaggi (`msgrcv`). Se `livello == HIGH`, fallback forzato a `ROLE_NONE`. Se `livello == MEDIUM` su worker fragili (es. Beatrice), trigger di rotazione preventiva. |

## 5. Problematiche incontrate e soluzioni adottate

Durante il ciclo di sviluppo e testing, l'architettura multithreading ha richiesto la risoluzione dei seguenti edge-case tecnici:

| Problema Tecnico              | Causa Identificata                                                                                                                        | Soluzione Implementata                                                                                                                                   |
| :---------------------------- | :---------------------------------------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Deadlock della Cucina**     | Il cuoco smetteva di produrre poiché il contatore dei `clean_plates` scendeva a zero bloccando la logica del server.                      | Inserimento della priorità assoluta (`ROLE_DISHWASHER`) nella pipeline di _Profit_ quando la variabile `clean_plates == NONE`.                           |
| **Starvation dei Piatti**     | Il cuoco abbandonava la postazione tra un ordine e l'altro, lasciando pietanze in stato di transizione senza terminare la cottura.        | Sviluppo della funzione di validazione `cooking_in_progress()` per vincolare il worker al ruolo finché l'operazione asincrona non è terminata.           |
| **Race Conditions (Lavagna)** | Più thread worker tentavano simultaneamente di sovrascrivere lo stesso ruolo concorrente (es. due thread si autoproclamavano _Cassiere_). | Sincronizzazione dell'accesso alla `shm_blackboard` tramite acquisizione atomica di semafori System V (`semop`).                                         |
| **Stallo per Stanchezza**     | Beatrice (`Resistenza LOW`) accumulava rapidamente fatica, raggiungendo lo stato critico e bloccando le mansioni di sala.                 | Implementazione di un bilanciatore di carico dinamico che esenta preventivamente il worker dalle zone calde al raggiungimento della stanchezza `MEDIUM`. |

## 6. Apporti specifici dei membri del gruppo

La suddivisione dei compiti per lo sviluppo del codice e la stesura di questa relazione è stata la seguente:

- **Samuele Orazio Durante**: Si è occupato dell'Infrastruttura e del Protocollo. Ha sviluppato il parsing degli argomenti CLI, la comunicazione di base con il server (messaggi HELLO, WELCOME, INSTANCE, END nei file `server_comm.c/.h`) e la gestione delle primitive IPC (`ipc_manager.c/.h`), configurando la memoria condivisa, i semafori e le code di messaggi.
- **Edoardo Francioli**: Si è dedicato alla gestione dei Worker Thread e dello Stato (`worker.c/.h`, `state.c/.h`). Ha implementato il ciclo di vita dei thread, la lettura dello stato e della stanchezza dalle code, la costruzione dello snapshot globale sincronizzato e la registrazione sulla lavagna condivisa.
- **Filippo Piva**: Ha curato interamente il modulo della Strategia (`strategy.c/.h`). Si è occupato della logica decisionale per le varianti Profitto e Reputazione, implementando l'assegnazione dinamica dei ruoli, la mitigazione della stanchezza e la gestione delle priorità in base alle caratteristiche del personale e alle richieste delle famiglie.

Ognuno dei membri del gruppo si è inoltre occupato della stesura della documentazione relativa al proprio modulo di competenza, mentre le restanti sezioni della relazione e l'assemblaggio finale sono stati redatti in maniera collaborativa e congiunta.
