# Relazione Tecnica - Trattoria Client

Questo documento descrive le caratteristiche della simulazione e le scelte progettuali implementate nel client della Trattoria.

## 1. Descrizione delle caratteristiche del personale
Il personale assegnato alla trattoria presenta un mix eterogeneo di abilità e tratti caratteriali:

- **Giulia**: Personale versatile (tutte le abilità MEDIUM). Spicca per l'alta **Pazienza**, ma ha bassa Socievolezza e Professionalità. È adatta a ruoli di contatto solo in emergenza, preferendo ruoli di background se altri sono disponibili.
- **Matteo**: Specialista in **Cassa** (HIGH) ma scarso in cucina (Cook=LOW). I suoi tratti sono mediamente bilanciati, rendendolo il candidato ideale per la gestione dei pagamenti e dei ruoli operativi (lavapiatti/pulizia) per non sprecare i tratti di contatto degli altri.
- **Gabriele**: Eccellente **Cameriere** (HIGH) con alta **Pazienza**. È uno dei pilastri per la strategia di reputazione grazie alla sua abilità nel servire e alla sua resistenza allo stress.
- **Beatrice**: Ottima **Cameriera** (HIGH) con buona **Professionalità**, ma con **Resistenza LOW**. Richiede rotazioni frequenti per evitare che la stanchezza raggiunga livelli critici (HIGH).

## 2. Osservazioni sulle caratteristiche delle famiglie
Le famiglie che frequentano la trattoria mostrano comportamenti che influenzano pesantemente le metriche:

- **Volume degli ordini**: Molte famiglie (es. Rossi, Russo, Ferrari) tendono a fare ordini "Large", mettendo sotto pressione la cucina e esaurendo rapidamente le scorte di piatti puliti.
- **Sensibilità all'attesa**: I log mostrano che i tempi di attesa per il cibo e per il pagamento sono i fattori critici. Recensioni negative (es. "wait='negative'") compaiono non appena il personale viene distratto da troppi ruoli contemporaneamente.
- **Importanza della cortesia**: Anche con tempi di attesa lunghi, un personale con alta pazienza (Giulia, Gabriele, Beatrice) riesce a mitigare parzialmente il danno alla reputazione (es. "courtesy='positive'").

## 3. Scelte nella progettazione della strategia
La strategia è stata progettata con un sistema di **Priorità Dinamiche** e **Persistenza**:

- **Assegnazione basata sulle Skill**: Nella strategia `PROFIT`, le persone vengono assegnate ai ruoli dove hanno skill >= MEDIUM. Matteo viene evitato in cucina se possibile.
- **Filtraggio per Tratti (Reputation)**: Nella strategia `REPUTATION`, i ruoli di Cameriere e Cassiere sono riservati prioritariamente al personale con tratti "Good Contact" (Pazienza, Socievolezza o Professionalità HIGH).
- **Persistenza del Ruolo**: Per evitare "l'abbandono del posto", un lavoratore che ha iniziato a cucinare o a lavare i piatti rimarrà in quel ruolo finché il compito non è terminato o il carico di lavoro non scende, garantendo continuità operativa.

## 4. Criteri per il soddisfacimento dei vincoli
- **Tempo di completamento (Profit)**: Si massimizza il parallelismo. Non si aspetta il "personale perfetto", ma si assegna il primo disponibile al primo compito utile (Cucina e Consegna cibo hanno priorità massima).
- **Reputazione maggiore**: Si privilegia la qualità del servizio. Il personale con alta professionalità è mantenuto in sala, anche se questo significa che i tavoli vengono puliti un po' più lentamente da personale meno socievole.
- **Gestione Stanchezza**: Ogni thread monitora costantemente il proprio livello. Se la stanchezza è **HIGH**, il thread entra forzatamente in stato `ROLE_NONE` (Riposo) finché il server non invia una notifica di recupero. Se è **MEDIUM** e la resistenza è bassa, si attiva una rotazione preventiva.

## 5. Problematiche incontrate e soluzioni adottate
- **Deadlock della Cucina**: Inizialmente la cucina si bloccava per mancanza di piatti puliti. **Soluzione**: Introdotta la priorità `DISHWASHER` assoluta quando la cucina è completamente bloccata (`clean_plates == NONE`).
- **Abbandono del Cuoco**: Il cuoco cambiava ruolo se non c'erano nuovi ordini pendenti, lasciando a metà i piatti sul fuoco. **Soluzione**: Introdotta la funzione `cooking_in_progress()` per mantenere il cuoco al suo posto finché il cibo non è effettivamente pronto.
- **Race Conditions sulla Lavagna**: Più thread cercavano di occupare lo stesso ruolo unico (es. Cassiere). **Soluzione**: Implementato un controllo atomico e l'uso di semafori System V per proteggere l'accesso alla memoria condivisa `shm_blackboard`.
- **Stanchezza di Beatrice**: Data la sua bassa resistenza, Beatrice tendeva a bloccare il servizio. **Soluzione**: Bilanciamento del carico che la sposta su compiti meno gravosi quando la sua stanchezza è media.
