programmazione multithreading:simulazione di una stazione ferroviaria.

DESCRIZIONE DEL PROBLEMA:

QUAL'è IL PROBLEMA DA RISOLVERE?
Creazione di una stazione ferroviaria,bisogna tener conto di vari aspetti come:
a)Treni partono insieme, di conseguenza bisogna manovrarli secondo un certo ordine di arrivo,priorità e 
attesa dello scarico merci(starvation).
b)I treni devono manovrarsi autonomamente in modo tale che riescano a gestire le entrare ed uscite di treni
in stazione,quindi devono essere consci del fatto che ci sono treni di priorità elevata e altri che non 
hanno accesso alla stazione da troppo tempo.

MOTIVO DELLA CREAZIONE DI QUESTO SOFTWARE:
Ho deciso di fare questo tipo di software in quanto era presenta nella lista a noi mostrata,
lo sviluppo del software sarà più sofisticato da quanto richiesto dalla traccia,
infatti,verranno aggiunte caratteristiche come controlli di priorità e starving da parte dei treni assieme ad
algortimi per l'aumento graduale di queste ultime.

TRACCIA:
Simulatore di stazione ferroviaria
Treni (thread) devono accedere a un numero limitato di binari; implementare una politica di
accesso con eventuali priorità.

---------------------------------------------------------------------------------------------
LIBRERIE IMPORTATE:

per il funzionamento dei thread:
1)<thread>

2)<mutex>

3)<condition_variable>

4)<semaphore>

per la simulazione dello scarico merci:
1)<chrono>

per il settaggio di variabili dei treni con valore randomico (vagoni,tempo di arrivo...)
1)<random>

per lavorare con strutture dati e puntatori:
1)<memory>

2)<vector>

3)<unordered_map> (hash table)

---------------------------------------------------------------------------------------------
ARCHITETTURA DEL SISTEMA:

VARIABILI PUBBLICHE PRINCIPALI (no mutex e variable condition spiegate in futuro):
a)variabili modificabili dall'utente
treni_in_entrata = dice quanti treni sono stati creati nel programma.
binari_disp = binari all'interno della stazione,sarà la lunghezza massima della hash table.
priority_threshold = indicatore la soglia di priorità che se superata il treno sceglierà il primo candidato da spostare fuori.
limit_of_starvation = stessa cosa della priority_treshold ma è utilizzato per controllare se il treno è riuscito a scaricare le merci o meno.

b)variabli da non modificare da parte dell'utente
int controllo_treni_in_stazione=0 \\controlla se i binari sono pieni.
int treni_completati=0 \\conta quanti treni hanno scaricato le merci.
int returned_id=-1 \\utilizzato per dire quale è il treno da spostare in stazione per far spazio a quello con priorità maggiore.

CLASSI UTILIZZATE:
Classe Treno:
contiene informazioni importanti che sono

ID
vagoni
tempo_di_arrivo
tempo_giro_largo = incrementa ogni volta che il treno non può entrare in stazione o doveva andare via per un treno con priorità elevata
tempo_in_stazione
priorità
starving = serve per controllare quante volte il treno non abbia avuto accesso allo scarico merci.
scarica = controlla se scarica le merci in stazione
generatore numeri randomici = utlizzato per dare un valore randomico a vagoni,tempo_di_arrivo/stazione

La classe ha un costruttore per inizializzareil treno prima di far partire i thread al suo interno.


Classe StazioneTreni:
è il cuore del software...prima delle funzioni al suo interno possiamo vedere 2 strutture dati.

vector<shared_ptr<Treno>> treni = al suo interno ci sono TUTTI i treni del programma.
unordered_map<int,Treno> treni_in_stazione = rappresenta i treni in stazione.
              ^
              |
Perchè l'utilizzo di una hash table?
la hash table treni_in_stazione è utlizzare per inserire,trovare il candidato da portare fuori dalla stazione e togliere treni.
Differentemente da un vettore l'inserimento e il cancellamento di un elemento dell'hash table è sempre costante 0(1),invece il vettore
deve iterare in base alla quantità di binari,perdendo tempo.


FUNZIONI PRESENTI NEL PROGRAMMA:

1)FUNZIONI PER L'INIZIO DEL PROGRAMMA,CONTROLLO DELLA MEMORIA E STAMPA DELLE INFORMAZIONI FINALI:
a)crea_treni = vengono inseriti i treni all'interno del vettore,la quantità è decisa dall'utente grazie alla variabile treni_in_entrata
b)inizializza_treni = qua vengono startati tutti i thread di ogni treno cosi si inzia la simulazione.
c)join_threads = vengono definitivamente chiusi i thread,fondamentale visto che abbiamo istanziato nell'heap i thread e vanno chiusi prima che 
la instanza creata viene distrutta.
d)info_trains = stampa le informazioni dei treni.

2)FUNZIONI PER IL FUNZIONAMENTO PRINCIPALE PER IL PROGRAMMA:
a)controlla_treni_in_stazione = è la funzione più corposa del programma,per questa è stata dovuta suddivide in 5 sotto-funzioni
per migliorare la leggibilità e rendere il codice meno innestato.
In questa funzione vengono simulati:

arrivo dei treni in stazione

trovati i primi candidati da togliere dalla ferrovia

il controllo di priorità

lo scarico merci

incremento e decremento degli indicatori globali

Tutte queste caratteristiche sono state raggruppate in delle funzioni elencate qua sotto:

simula_treno(): simula l'arrivo del treno in stazione attraverso un this_thread::sleep_for(...)
verranno poi scritte le sue informazioni di arrivo.

Adesso i treni hanno 2 possibilità:
entrare in stazione:prima di effettivamente scaricare le merci controllano che un treno di alta priorità stia cercando di entrare 
in stazione...
Per capire come verranno selezionati andare a vedere il funzionamento della funzione find_first_candidate()

SE la stazione è piena e sono di alta priorità allora usano la funzione qua sotto descritta

find_first_candidate(): viene utilizzata questa funzione SE il treno è di alta priorità (oppure ha un alto starving) e i binari 
sono tutti occupati.

all'interno viene selezionato IL PRIMO CANDIDATO ACCETTABILE che verrà poi spostato fuori dall stazione sotto queste condizioni:
ha priorità minore del treno corrente e non sta scaricando le merci OPPURE ha una starving minore del treno corrente e non sta scaricando le merci

SE SCELTO allora si fermerà la funzione e returned_id diventerà dell'ID del treno scelto e tutti i treni saranno di liberi di continuare,tranne 
quello scelto che lascierà il posto a quello in entrata.


controll_priorities(): funziona chiamata dai treni dentro la stazione per controllare che un treno di alta priorità stia entrando.
è una funzione che ritorna true o false in base al treno scelto.
ritornerà true al treno che è stato scelto altrimenti false e loro andranno a scaricare le merci.

inizio_scarico_merci(): funziona che simula lo scarico delle merci,qua scarica=true e il treno no potrà essere scelto come candidato in find_first_candidate().

modify_limit_of_starvation():in base allo starving medio che è presente in stazione verrà aumentato gradualmente l'indicatore di starving globale.


TUTTI GLI ALGORTIMI VERRANNO SPECIFICATI NELLA SEZIONE DEDICATA!!!

---------------------------------------------------------------------------------------------
SCELTE DI CONCORRENZA:
Per questo software sono stati usati:

mutex : 4

condition_variable : 3

counting_semaphore : 1

MUTEX:
1)scrittura_info : scrive tutte le informazioni per tener aggiornato delle scelte dei treni e le loro azioni.
2)controllo_priorita : è utlizzato per la scelta del candidato,utilizzo della hash table,inserimento ed inserimento di treni in quest'ultimo.
3)returned_id_mutex : utlizzato per modificare il valore di retured_id,utilizzato per definire il treno da spostare.
4)continua_main : accoppiato a un condition variable per il continuamento del main in caso tutti i treni abbiano finito di scaricare le merci.

CONDITION_VARIABLE:
1)continua : accoppiato con il mutex continua_main,aspetta che tutti i treni abbiano finito di scaricare le merci.
se non viene inserito il programma finirebbe subito senza la posibilità di vedere il programma.

2)controllo_arrivo_treni : accoppiato con il mutex returned_id_mutex,infatti è utlizzato dai treno in stazione per controllare che il treno di priorità alta
scelga il treno da spostare,altrimenti vanno a scaricare le merci.
viene notificato quando viene trovato il candidato,se non viene ricevuta la notifica allora loro escono senza problemi.

3)treno_spostato : accoppiato con il mutex returned_id_mutex per aspettare che returned_it ritorni al valore -1 cosi che sia corretto per l'utlizzo da parte del prossimo 
treno di alta priorità
verrà notificato dal treno che è verrà spostato.

COUNTING_SEMAPHORE:
1)binari:indica la quantità di binari in stazione decisa dalla variabile binari_disp

