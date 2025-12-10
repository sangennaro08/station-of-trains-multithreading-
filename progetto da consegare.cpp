#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <vector>
#include <memory>
#include <chrono>
#include <condition_variable>
#include <random>
#include <unordered_map>

using namespace std;
//---------------------------------variabili utilizzati dall'utente per vedere cosa succede in ogni situazione---------------------------------//
int treni_in_entrata=140;
constexpr int binari_disp=15;
double priority_threshold=30;
double limit_of_starvation=3;

//---------------------------------strutture per la concorrenza tra thread---------------------------------//
mutex scrittura_info;
mutex continua_main;
mutex controllo_priorita;
mutex returned_id_mutex;

condition_variable continua;
condition_variable controllo_arrivo_treno;
condition_variable treno_spostato;

//---------------------------------VARIABILI DA NON MODIFICARE---------------------------------//
int controllo_treni_in_stazione=0;
int treni_completati=0;
int returned_id=-1;

//---------------------------------classe treno---------------------------------//
void controllo_var_globali();
class StazioneTreni;

class Treno{

    public:

    int ID;
    int vagoni;
    int tempo_in_stazione;
    int tempo_di_arrivo;
    int tempo_giro_largo=0;
    bool scarica=false;
    bool entrata_di_priorita=false;

    double priorita;
    int starving=0;

    mt19937 generatore;
    thread th;

    Treno(int id):
    ID(id),
    generatore(chrono::system_clock::now().time_since_epoch().count() + ID)
    {
        uniform_int_distribution<int> distribution(0, 15);

        vagoni = distribution(generatore) % 10 + 1;
        tempo_di_arrivo = distribution(generatore) % 15 + 1;
        tempo_in_stazione = this->vagoni * 2;
        priorita = vagoni * tempo_di_arrivo;
    }
};

//---------------------------------stazione dei treni---------------------------------//
class StazioneTreni{

    counting_semaphore <binari_disp> binari{binari_disp};

    vector <shared_ptr<Treno>> treni;
    unordered_map<int, shared_ptr<Treno>> treni_in_stazione;

    public:
    
    //----------------------creiamo i treni per il programma--------------------------//
    void crea_treni()
    {
        treni.reserve(treni_in_entrata);
        treni_in_stazione.reserve(binari_disp);

        for(int i=0;i<treni_in_entrata;i++)
            treni.emplace_back(make_shared<Treno>(i));
    }

    //-----------------vengono lanciati i thread dentro a ogni treno------------------//
    void inizializza_treni()
    {
        for(int i=0; i<treni.size(); i++)
            treni.at(i)->th=thread(&StazioneTreni::controlla_treni_in_stazione,this,treni.at(i));
    }

    //--------------------simula il treno che arriva in stazione----------------------//
    void simula_treno(shared_ptr<Treno> t) {
        // Simula arrivo
        {
            lock_guard<mutex> lock(scrittura_info);
            cout << "Treno " << t->ID << " arrivera in " << t->tempo_di_arrivo << " secondi (" 
                 << t->vagoni << " vagoni, priorita: " << t->priorita << ")\n\n";
        }
        
        this_thread::sleep_for(chrono::seconds(t->tempo_di_arrivo + t->tempo_giro_largo));
    }

    //-----------------funzione in cui il treno simula il suo stato---------------------//
    void controlla_treni_in_stazione(shared_ptr<Treno> t)
    {   
        while(true)
        {   simula_treno(t);  
            {
                lock_guard<mutex> lock0(controllo_priorita);

                //controlliamo se il treno è di alta priorità
                if((t->priorita > priority_threshold || t->starving >= limit_of_starvation) && (controllo_treni_in_stazione == binari_disp))
                {   

                    //trova il primo candiato da cacciare dalla stazione
                    find_first_candidate(t);

                    if (returned_id != -1)
                    {   
                        t->entrata_di_priorita=true;
                        // Attendi che il selezionato resetti returned_id (cioè che vada via)
                        //si evita anche che nel mutex principale ci siano 2 treni diversi che richiedono uno singolo nella stazione
                        //di andarsene contemporaneamente
                        {
                            unique_lock<mutex> lock(returned_id_mutex);
                            bool ok = treno_spostato.wait_for(lock, chrono::seconds(6),
                            [&]{ return returned_id == -1;});
                        }
                    }
                }

                //modifica l'indicatore starvation e di priorità in base alla situazione
                modify_limit_of_starvation();
            }     
            //tenta di entrare in stazione   
            if(!binari.try_acquire())
            {   
                {   
                    lock_guard<mutex> lock(scrittura_info);

                    cout<<"il treno con ID "<<t->ID
                    <<" deve fare un giro largo,stazione piena\n\n";
                    t->tempo_giro_largo += 2;
                    t->starving += 1;//= min(t->starving + 1, limit_of_starvation);
                    t->priorita = t->vagoni * t->tempo_di_arrivo + 2 * t->starving;     
                }
                continue;

            }else{

                {
                    lock_guard<mutex> lock(controllo_priorita);
                    treni_in_stazione[t->ID] = t;//treni_in_stazione.push_back(t);
                    controllo_treni_in_stazione++;
                }
                
                {
                    lock_guard<mutex> lock(scrittura_info);
                    cout<<" il treno con ID "<<t->ID
                    <<" CONTROLLA che un treno con priorità elevata stia entrando in stazione\n\n";
                }     
                
                //controlla se lui verrà cacciato dalla stazione o meno.
                if(controll_priorities(t))
                {
                    {
                        lock_guard<mutex> lock(controllo_priorita);
                        t->scarica = false;
                        controllo_treni_in_stazione--;   
                        t->tempo_giro_largo += 2;
                        t->priorita = t->vagoni * t->tempo_di_arrivo + 4 * t->starving;
                        //priority_threshold *= 1.01; // aumenta la soglia di priorità del 0.1%
                    }
                    continue;                 
                }
                
                //simula lo scarico merci
                inizio_scarico_merci(t);
                binari.release();

                //se sono completati tutti i treni allora continuiamo nel main
                if(treni_completati >= treni_in_entrata)continua.notify_one();
                return;
            }
        }
    }
    
    //simulazione dello scarico merci
    void inizio_scarico_merci(shared_ptr<Treno> t)
    {
        {
            lock_guard<mutex> lock(controllo_priorita);
            t->scarica=true;
        }

        {
            lock_guard<mutex> lock(scrittura_info);
            cout<<"il treno con ID "<<t->ID<<" HA PASSATO IL CONTROLLO,resterà per "
            <<t->tempo_in_stazione<<" secondi per scaricare le merci\n\n";
        }
            this_thread::sleep_for(chrono::seconds(t->tempo_in_stazione));
        {
            lock_guard<mutex> lock(scrittura_info);
            cout<<"il treno con ID "<<t->ID
            <<" HA FINITO di stare in stazione adesso andrà via\n\n";
        }

        {
            lock_guard<mutex> lock(controllo_priorita);
                    
            t->scarica=false;
            t->entrata_di_priorita=false;
            controllo_treni_in_stazione--;
            treni_completati++;
            treni_in_stazione.erase(t->ID);//erase_from_station(t);
        }
    }


    //controllo del treno se verrà cacciato o meno
    bool controll_priorities(shared_ptr<Treno> t)
    {      

        //IL TRENO ASPETTA CHE RICEVA IL SEGNALE DI ANDARE AVANTI.
        unique_lock<mutex> lock(returned_id_mutex);
        bool selezionato = controllo_arrivo_treno.wait_for(
        lock,
        chrono::seconds(6),
        [&]{ return returned_id == t->ID;}
        );

        //SE è IL SELEZIONATO VA VIA DALLA STAZIONE
        if(selezionato && returned_id == t->ID)
        {
            {   
                lock_guard<mutex> lock(scrittura_info);
                cout<<"il treno con ID "<<t->ID
                <<" VA VIA per lasciare il posto a un treno con priorità piu elevata\n\n";
            }

            t->starving += 1;
            returned_id = -1;
            treno_spostato.notify_one();
            binari.release();
            treni_in_stazione.erase(t->ID);//ELIMINATO DALLA STAZIONE

            return true;
        }    
        return false;
    }

    //funzione che trova il primo candidato accettabile per essere cacciato
    void find_first_candidate(shared_ptr<Treno> t)
    {
        lock_guard<mutex> lock(returned_id_mutex);

        for(auto& [id, treno] : treni_in_stazione)
        {
            if((treno->priorita < t->priorita || treno->starving < t->starving) &&
                (!treno->scarica && !treno->entrata_di_priorita))
            {   
                returned_id=treno->ID;
                controllo_arrivo_treno.notify_all();
                {
                    lock_guard<mutex> lock(scrittura_info);
                    cout<<"il treno con ID "<<t->ID
                    <<" HA PRIORITÀ ELEVATA quindi fa SPOSTARE il treno con ID "<<returned_id<<"\n\n";
                }               
                break;
            }
        }
    }

    //funzione finale che dice la media dei punti starving totali.
    void media_starving()
    {
        double media_starving = 0;
        for(auto& treno : treni) media_starving += treno->starving;

        media_starving /= treni.size();

        cout<<"la media di starvation accumulata dai treni è di: "
        <<media_starving<<"\n\n";
    }

    //FUNZIONE CHIAMATA DAL TRENO DI ALTA PRIORITà PER MODIFICARE L'INDICATORE DI STARVATION E DI PRIORITà
    void modify_limit_of_starvation()
    {
        double media_starving = 0;
        for(auto& treno : treni) media_starving += treno->starving;

        media_starving /= treni.size();
                
        if (media_starving > limit_of_starvation) {
            
            //CREAZIONE DELLA PERCENTUALE IN PIù DA ASSOCIARE ALL'INDICATORE STARVATION
            double occ = controllo_treni_in_stazione / binari_disp;
            double sat = media_starving / (1.0 + media_starving); // saturazione per outlier
            double factor = 1.0 + 0.15 * occ * sat; // aumento fino ~+15% in condizioni critiche

            //CREAZIONE DEL NUOVO LIMITE STARVATION
            int new_limit = max(static_cast<int>(limit_of_starvation), static_cast<int>(floor(limit_of_starvation * factor)));
            limit_of_starvation = new_limit;
            priority_threshold = min(priority_threshold * 1.01, 1000.0);
        }
    }
    

    //joinati i thread
    void join_threads()
    {
        for(auto& treno : treni)
            if(treno->th.joinable())treno->th.join();
    }

    //stampate tutte e info dei treni del programma
    void info_trains()
    {
        for(int i=0;i<treni.size();i++)
        {
            cout<<"treno ID "<<treni.at(i)->ID<<":\n";
            cout<<"vagoni:"<<treni.at(i)->vagoni<<"\n";
            cout<<"tempo di arrivo:"<<treni.at(i)->tempo_di_arrivo
            <<" e tempo in stazione:"<<treni.at(i)->tempo_in_stazione<<"\n";
            cout<<"priorità finale:"<<treni.at(i)->priorita<<"\n";
            cout<<"punti di starvation accumulati:"<<treni.at(i)->starving<<"\n";
            cout<<"----------------------------------------\n\n\n";
        }
    }  
};

int main(){

    controllo_var_globali();

    this_thread::sleep_for(chrono::seconds(5));

    StazioneTreni stazione;

    //creazione treni ed inizializzazione dei thread all'interno
    stazione.crea_treni();
    stazione.inizializza_treni();

    //controllo che TUTTI i treni abbiano scaricato le merci.
    {
        unique_lock<mutex> lock(continua_main);
        continua.wait(lock,[]{return treni_completati>=treni_in_entrata;});
    }

    //INFORMAZIONI FINALI DA SCRIVERE ALL'UTENTE
    stazione.join_threads();
    stazione.info_trains();

    stazione.media_starving();

    return 0;
}

//CONTROLLIAMO CHE LA CONFIGUARAZIONE DELL'UTENTE SIA APROPRIATA
void controllo_var_globali(){

    if(priority_threshold<40)
    {
        priority_threshold=40;
        cout<<"la soglia di priorità era troppo bassa,ora è stata impostata a 40\n\n";
    }

    if(treni_in_entrata<=0)
    {
        treni_in_entrata=binari_disp;
        cout<<"il numero di treni in entrata era troppo basso,ora è stato impostato a "<<binari_disp
        <<"pari ai binari in stazione\n\n";
    }

    if(limit_of_starvation<3)
    {
        limit_of_starvation=3;
        cout<<"il \"limite\" di starvation era troppo basso,ora è stato impostato a 3\n\n";
    }

    controllo_treni_in_stazione=0;
    treni_completati=0;
    returned_id=-1; 
}
