DOCUMENTAZIONE CODICE STAZIONE DEI TRENI(C++):

DESCRIZIONE DEL PROBLEMA:

Simulazione di una stazione dei treni con quantità di treni e binari decisi dall’utente all’inizio.
Si ha un tempo di permanenza proporzionale alla quantità di vagoni.

Problema principale:
Gestione l’accesso in stazione da parte dei treni,che entrano in base alla loro priorità e
quantità di volte che non hanno avuto la possibilità di scaricare le merci.

Si implementa quindi:
1)Controllo priorità dinamico,vale la stessa cosa per lo starvation dei treni.
2)Meccanismo di espulsione dei treni in caso entri uno più importante.
3)Modifica degli indicatori di priorità e starvation per definire un treno di “alta priorità”.

ARCHITETTURA DEL SISTEMA:

COMPONENTI PRINCIPALI:
Classe Treno:rappresenta di fatto un treno,con variabili di priorità .
Classe Stazione Treno:rappresenta la stazione dei treni dove loro stimoleranno i loro
cambiamenti di stato.
mutex,semafori e condition variables:utilizzati per far coordinare e comunicare i vari
Thread-treno.
Variabili globali:sono globali così che l’utente possa decidere ogni tipo di situazione della
simulazione.


SCELTA DI CONCORRENZA E SINCRONIZZAZIONE:

MUTEX:
scrittura info:utilizzata per scrivere cosa sta accadendo nel momento e per evitare
overlapping di messaggi,leggendo messaggi incomprensibili.

continua main:utilizzata per fermare il thread main fino a quando tutti i treni non hanno finito
di scaricare le merci;altrimenti il software si chiude subito.

controllo priorità:mutex che è utilizzato per accedere,iterare,inserire ed eliminare treni
all'interno della hashmap che contiene i treni dentro la stazione.

return id mutex:usato per permettere di modificare la variabile globale returned id che
permette l’espulsione del treno desiderato.

SEMAFORI:
binari:usato per dire quanti binari sono occupati in stazione.

CONDITION_VARIABLE:
continua:usato per aspettare il segnale per continuare il main e finire definitivamente il
programma.

controllo arrivo treno:i treni dentro la stazione controllano grazie a questo mutex per tot
tempo se viene un treno di alta priorità così per vedere se il loro treno è quello cacciato o
meno.

treno spostato:usato dal treno di alta priorità per rilasciare tutti i treni dal condizione varia
controllo arrivo treno,evitando di aspettare di più e si ha una risposta del treno espulso più
veloce.

PERCHÉ QUESTA SCELTE?
1)I mutex DEVONO controllare risorse differenti per evitare che esse possano creare dati
errati,l’utilizzo di 4 mutex è dato a causa dell’alta complessità del software che avviene
grazie agli algoritmi e variabili del controllo implementati.

2)i condition variable sono con un time out per evitare deadlock e treni che non finiscono a
causa del segnale che non arriverà mai,in più sono usati per dare un ordine delle
tempistiche corretto e non spericolato.

3)L'utilizzo della hash table è data grazie alla sua capacità di inserire e togliere con tempo
costante 0(1) è stato di conseguenza preferito da un vettore in quanto non serve iterare in
esso per trovare il treno da togliere,si risparmiano computazioni e tempo nella ottenimenti di
vari segnali delle VC.

ALGORITMI E STRUTTURE DATI UTILIZZATI:

ALGORITMI:
ALGORITMO DELLA SCELTA DEL CANDIDATO DA ESPELLERE:
favorisce quelli con alta priorità,non vengono espulsi coloro che stanno scaricando le merci
o hanno una starvation elevata.

ALGORITMI PER LA MODIFICA DEGLI INDICATORI DI STARVING E PRIORITà
Se la media starving dei treni è più alta della media la aumentiamo all'incirca del 15%
assieme ad un leggero aumento dell’indicatore della priorità.

STRUTTURE DATI UTILIZZATI PER IL SOFTWARE:
treni: all'interno tutti i treni del programma.
treni in stazione:contiene tutti i treni che sono presenti in stazione,ha una lunghezza
massima SEMPRE PARI ai binari della stazione.

TEST EFFETTUATI:
1)Molti treni,pochi binari.
assenza di deadlock totale,media dei punti starvation minore del rapporto tra i treni totali e i
binari 90% delle volte è la metà del rapporto.
2)quantità treni 100+ e binari 20+.
assenza di deadlock, starvation sempre pari alla metà del rapporto dei treni totali e binari.
3)indicatore della priorità e dello starvation basso con setting come il punto 1.
stessi risultati del punto 1.
4)treni 100+ binari 10+ ma minori di 20 con indicatori priorità e starving basso.
assenza di deadlock,e starvation ha un risultato sempre pari alla metà del rapporto tra treni
totali e binari.

DIFFICOLTà:
1)creazione di algoritmi tali da non alzare troppo gli indicatori.
2)creazione di una priorità dinamica dei treni.
3)l'utilizzo corretto delle strutture dati.
