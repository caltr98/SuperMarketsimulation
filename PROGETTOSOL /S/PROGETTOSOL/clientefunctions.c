#include "header.h"
#include "coda.h"
#include "cassaheader.h"
typedef struct cliente_{/*struttura che racchiude le informazioni di un cliente*/
	int idcliente;
	struct timespec tempoacquisti;//tempo cliente nel fare gli acquisti(nanosleep)
	double tempototaleacquisti;//tempo effettivo del cliente nel fare gli acquisti
	int numeroprodotti;
	double tempopermanenza;//tempo misurato totale di permanenza
	double tempoincoda;//tempo misurato in coda
	int countcambiocoda;
	int maxnumcasse;
	int pagacassa;//se pagacassa=1 il cliente ha fatto acquisti, altrimenti prodotti acquistati=0 o supermercato chiuso prima
}cliente;	

typedef struct direttorecliente_{
	int indexcliente;
	int clienteattivo;//variabile che indica se il cliente è ancora nel supermercato o no(da accedere in mutua esclusione)
	int T;
	int P;
	int K;
	pthread_cond_t attesapermesso;//variabile condizione in cui il cliente è sospeso in attesa di permesso
	int permessouscita;//settato ad 1 quando il direttore permette al cliente di uscire senza acquisti
	int chiusuraimmediata;//informa il cliente che non potrà pagare e di uscire dalla coda o terminare acquisti
	pthread_mutex_t permessomutex;//mutex per accesso in mutua esclusione a permessouscita
	pthread_mutex_t chiusuramutex;//mutex per accesso in mutua esclusione a chiusuraimmediata E ANCHE di clienteattivo
}direttorecliente;
typedef struct oldclientdata_{/*struttura che mantiene le informazioni di clienti terminati*/
	cliente *oldclient;
	struct oldclientdata_* next;
}oldclientdata;
typedef struct infoaffluenza_{/*racchiude le informazioni di tutti i clienti e statistiche su essi*/
	int maxclienti;
	int clientitotali;//campo che viene aggiornato dal direttore stesso
	int acquistitotali;
	oldclientdata *clientreport;
	oldclientdata *clientreportlast;
	pthread_mutex_t reportmutex;//acesso in mutua esclusione su acquistitotali
}infoaffluenza;


cliente* initcliente(int T,int P,int K,int indexcliente);
void clienteacquisti(struct timespec tempoacquisti,double *tempopermanenza,int *chiusuraimmediata,pthread_mutex_t *accesscontrol);
void clienteatexit(void *arg);
int clienteincassa(cliente *me,int *chiusuraimmediata,pthread_mutex_t *accesscontrol);
void clienteatexit2(void *arg);
void uscitasenzaacquisti(direttorecliente *collegamento);
void clientatexit3(void *arg);
void freereport(oldclientdata**l);
extern infoaffluenza infoclienti;
extern infocassa*cassa;

void insertOPoldclientdata(cliente *me,oldclientdata **lis,oldclientdata **last){//inserisco in fondo info su cliente terminato
	oldclientdata *new;
	//int i;
	MALLOC(new,1,oldclientdata);
	new->oldclient=me;
	new->next=NULL;
	if((*lis)==NULL){
		
		*lis=new;
	}
	else{
		(*last)->next=new;
	}
	*last=new;
}


void clienteacquisti(struct timespec tempoacquisti,double *tempopermanenza,int *chiusuraimmediata,pthread_mutex_t *accesscontrol){
	struct timespec residuo;
	struct timespec start;
	struct timespec end;
	struct sigaction sa;
	sigset_t handlermask;
	sigemptyset(&handlermask);
	sigaddset(&handlermask,SIGUSR1);//SIGUSR blocca cliente durante acquisti per farlo uscire
	SIGACTION(SIGUSR1,NULL,&sa);
	sa.sa_mask=handlermask;
	sa.sa_handler=SIG_IGN;//ignoro qualsiasi handler pre installato per SIGUSR1
	SIGACTION(SIGUSR1,&sa,NULL);
	THREADSIGMASK(SIG_UNBLOCK,&handlermask,NULL);
	clock_gettime(CLOCK_BOOTTIME,&start);
	int chiudi=0;
	int acquistieffettuati=0;
	while(!acquistieffettuati){
		if(nanosleep(&tempoacquisti,&residuo)==-1){
			if(errno==EINTR){//se è arrivata SIGUSR1  da direttore e ricevu chiudi==1 cliente esce prima di terminare tempo
				MUTEXLOCK(accesscontrol);
				chiudi=*chiusuraimmediata;
				MUTEXUNLOCK(accesscontrol);
				if(chiudi==0){
					tempoacquisti=residuo;
				}
				else{
					clock_gettime(CLOCK_BOOTTIME,&end);
					*tempopermanenza=((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000);
					pthread_exit(NULL);
				}	
			}
			else{
				perror("during nanosleep");
				exit(EXIT_FAILURE);
			}
		}
		else{acquistieffettuati=1;}
	}
	THREADSIGMASK(SIG_BLOCK,&handlermask,NULL);//non è più necessario aspettare segnali
	clock_gettime(CLOCK_BOOTTIME,&end);
	*tempopermanenza=((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000);
	return;
}
void clienteatexit(void *arg){
	cliente *me=*(cliente**)arg;
	MUTEXLOCK(&(infoclienti.reportmutex));
	infoclienti.acquistitotali+=me->numeroprodotti;
	insertOPoldclientdata(me,&(infoclienti.clientreport),&(infoclienti.clientreportlast));
	MUTEXUNLOCK(&(infoclienti.reportmutex));
	return;
}
void clienteatexit2(void *arg){
	direttorecliente* collegamento=*(direttorecliente**)arg;
	MUTEXLOCK((&collegamento->chiusuramutex));
	collegamento->clienteattivo=0;
	MUTEXUNLOCK(&(collegamento->chiusuramutex));
	return;
}
void clienteatexit3(void *arg){
	cliente*me=(cliente*)arg;
	me->tempopermanenza+=me->tempoincoda;
	return;
}
cliente* initcliente(int T,int P,int K,int indexcliente){
	cliente *me;
	struct timespec timeforseed;
	int Tnanosec=T*1000000L; //da microsecondi a nanosecondi
	clock_gettime(CLOCK_BOOTTIME,&timeforseed);
	unsigned int seed=(timeforseed.tv_sec)+(timeforseed.tv_nsec);
	long long randnum;
	//srand((unsigned)(time(NULL)));
	randnum=rand_r(&seed);
	MALLOC(me,1,cliente);
	me->idcliente=indexcliente;
	me->tempototaleacquisti=0;
	me->tempopermanenza=0;
	me->tempoincoda=0;
	me->countcambiocoda=0;
	me->tempoacquisti.tv_sec=0;
	me->pagacassa=0;
	me->maxnumcasse=K;
	me->tempoacquisti.tv_nsec=(randnum%(Tnanosec+1-TENISMINTIME))+TENISMINTIME;
	me->numeroprodotti=(rand_r(&seed))%(P+1);
	
	return me;
}
int clienteincassa(cliente *me,int *chiusuraimmediata,pthread_mutex_t *accesscontrol){
	int i;
	i=0;
	cassacliente *cassascelta;
	int successo=0;
	int chiudi=0;
	struct timespec timeforseed;
	clock_gettime(CLOCK_BOOTTIME,&timeforseed);
	unsigned int seed=(timeforseed.tv_sec)+(timeforseed.tv_nsec);//per scelta causale di cassa
	int restaopen=1;
	int accodato=0;
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_BOOTTIME,&start);
	while(!successo){
		i=rand_r(&seed)%me->maxnumcasse;	
		cassascelta=&(cassa[i].filaclienti);
		MUTEXLOCK(&(cassascelta->mutexcoda));//controllo apertura e accodamento in mutua esclusione
		if((restaopen=cassascelta->opencassa)){
			++cassascelta->numincoda;
			accodato=1;
			accoda(&(cassascelta->codacassa),me->idcliente);
			while((restaopen=cassascelta->opencassa) && cassascelta->idclienteservito!=me->idcliente){
				CONDWAIT(&(cassascelta->attesacoda),&(cassascelta->mutexcoda));//aspetto che sia il mio turno
			}//anche se svegliato il thread potrebbe non essere quello servito attualmente o la cassa potrebbe essere chiusa
			if(restaopen&&cassascelta->idclienteservito==me->idcliente){
				cassascelta->nprodclienteservito=me->numeroprodotti;//invio alla cassa numero di acquisti
				CONDSIGNAL(&(cassascelta->attesaservizio));//notifico alla cassa di aver inviato il numero
				while(!cassascelta->servizioeffettuato){//aspetto che cassa abbia servito il cliente
						CONDWAIT(&(cassascelta->attesaservizio),&(cassascelta->mutexcoda));
				}
				cassascelta->servizioeffettuato=0;//dico a cassa di aver finito 
				CONDSIGNAL(&(cassascelta->attesaservizio));
				MUTEXUNLOCK(&(cassascelta->mutexcoda));
				successo=1;
			}
		}
		if(!successo){
			MUTEXUNLOCK(&(cassascelta->mutexcoda));
			if(!restaopen){//cliente può essere rimasto vittima di una cassa chiusa con supermercato aperto
				MUTEXLOCK(accesscontrol);
				chiudi=*chiusuraimmediata;
				MUTEXUNLOCK(accesscontrol);
				if(chiudi){//tutte le casse sono state chiuse da direttore che ha dato ordine di chiusura
					clock_gettime(CLOCK_BOOTTIME,&end);
					me->tempoincoda= ((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000L);
					successo=-1;//assegnamento per uscire dal while
				}
				else if (accodato){
					++me->countcambiocoda;
					accodato=0;
				}
			}
		}
		else{
			clock_gettime(CLOCK_BOOTTIME,&end);
			me->tempoincoda= ((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000L);
		}
	}


	return successo;
}
void uscitasenzaacquisti(direttorecliente *collegamento){
	//richiedi permesso
	MUTEXLOCK(&(collegamento->permessomutex));
	if(collegamento->permessouscita!=1){//in caso di chiusura immediata SIGQUIT clienti non debbono neanche chiedere permesso
		collegamento->permessouscita=-1;
		while(collegamento->permessouscita==-1){
			CONDWAIT(&(collegamento->attesapermesso),&(collegamento->permessomutex));
		}
	}
	MUTEXUNLOCK(&(collegamento->permessomutex));
	pthread_exit(NULL);
}
void freereport(oldclientdata**l){
	if(*l==NULL){
		return;
	}
	freereport(&((*l)->next));
	free(*l);
}
	
