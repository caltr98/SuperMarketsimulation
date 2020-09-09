#include "header.h"
#include "coda.h"
#define MINWAIT 20000000
#define MAXWAIT 80000000 

typedef struct instancetime_{
	double time;
	struct instancetime_*next;
}instancetime;
typedef struct direttorecassiere_{
	int cassa;
	int numeroclienticoda;
	int open;
	pthread_mutex_t accesslock;//da usare per accesso atomico ai campi open e numeroclienticoda
}direttorecassiere;


typedef struct argcomunicatore_{
	direttorecassiere*info;
	struct timespec intervallo;
	int internalcounter;
	pthread_mutex_t countermutex;
}argcomunicatore;
typedef struct cassacliente_{
	int opencassa;//importante che check apertura fino a spostamento siano atomici di modo che cassa non chiuda mentre mi ci sposto
	//importante anche per sapere se cassa è ancora aperta o è chiusa, una brodcast risveglierà i clienti e controlleranno.
	coda codacassa;
	pthread_cond_t attesacoda;
	pthread_cond_t attesaservizio;
	pthread_mutex_t mutexcoda;//uso per accedere alle altre strutture in modo esclusivo
	int idclienteservito;
	int nprodclienteservito;
	int servizioeffettuato;
	int numincoda;
}cassacliente;
typedef struct tempiclienticassa_{
	double tempo;
	struct tempiclienticassa_*next;
}tempiclienticassa;
typedef struct infocassa_{
	int idcassa;
	int countprod;
	int countclients;
	int tempoprodotto;
	instancetime *optime;//tempo di ogni apertura
	instancetime *lastoptime;
	instancetime *pctime;//tempo di servizio per ogni cliente
	instancetime *lastpctime;
	int closurecount;
	int countincoda;
	cassacliente filaclienti;
	tempiclienticassa *recordtempi;
}infocassa;




infocassa* initcasse(int maxcasse,int tempoprodotti,int maxclienti);
int cassaserviocliente(infocassa* cassacorrente,struct timespec waitprod,struct timespec waitfissa,double *temposerv,int*countcoda);
void insertOPtime(double val,instancetime **lis,instancetime **last);
void initcassieretime(int attesaprod,int avvisi,struct timespec* waitprod,struct timespec* waitfissa,struct timespec*waitavvisi);
void attesacassaop(struct timespec waittime);
void insertrecordtempi(double tempo,tempiclienticassa**lis);
void freecorrtempi(tempiclienticassa **l);
void freeopentime(instancetime**l);
extern infocassa*cassa;



void insertrecordtempi(double tempo,tempiclienticassa**lis){
	tempiclienticassa *new;
	MALLOC(new,1,tempiclienticassa);
	new->next=*lis;
	new->tempo=tempo;
	*lis=new;
}
int cassaserviocliente(infocassa* cassacorrente,struct timespec waitprod,struct timespec waitfissa,double *temposerv,int *countcoda){
	int i;
	cassacliente*filacassa=&(cassacorrente->filaclienti);
	int prodottiprocessati=0;
	struct timespec start;
	struct timespec end;

	MUTEXLOCK(&(filacassa->mutexcoda));
	if(filacassa->numincoda>0){
		filacassa->idclienteservito=decoda(&(filacassa->codacassa));
		clock_gettime(CLOCK_BOOTTIME,&start);

		filacassa->nprodclienteservito=-1;
		CONDBROADCAST(&(filacassa->attesacoda));//risveglio tutti i clienti in modo che possano vedere se è il loro turno
		while(filacassa->nprodclienteservito==-1){
			CONDWAIT(&(filacassa->attesaservizio),&(filacassa->mutexcoda));
		}
		filacassa->servizioeffettuato=0;
		attesacassaop(waitfissa);//tempo fisso di servizio
		for(i=0;i<filacassa->nprodclienteservito;i++){
			attesacassaop(waitprod);//tempo prodotti di servizio
		}
		prodottiprocessati+=filacassa->nprodclienteservito;
		filacassa->servizioeffettuato=1;
		CONDSIGNAL(&(filacassa->attesaservizio));
		while(filacassa->servizioeffettuato){
			CONDWAIT(&(filacassa->attesaservizio),&(filacassa->mutexcoda));
		}
		--filacassa->numincoda;
		filacassa->nprodclienteservito=-1;
		filacassa->idclienteservito=-1;
	}
	*countcoda=filacassa->numincoda;
	MUTEXUNLOCK(&(filacassa->mutexcoda));
	clock_gettime(CLOCK_BOOTTIME,&end);
	*temposerv=((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000);
	return prodottiprocessati;
}
void attesacassaop(struct timespec waittime){
	int fine=0;
	struct timespec residuo;
	while(!fine){
		if(nanosleep(&waittime,&residuo)==-1){
			if(errno!=EINTR){
				perror("during nanosleep");
				exit(EXIT_FAILURE);
			}
		}
		else{fine=1;}
	}
}
void insertOPtime(double val,instancetime **lis,instancetime **last){
	instancetime *new;
	instancetime *corr;
	MALLOC(new,1,instancetime);
	new->time=val;
	new->next=NULL;
	if((*lis)==NULL){

		*lis=new;
	}
	else{
		(*last)->next=new;
	}
	*last=new;	
}
infocassa* initcasse(int maxcasse,int tempoprodotti,int maxclienti){
	int i;
	infocassa* cassa;
	MALLOC(cassa,maxcasse,infocassa);
	for(i=0;i<maxcasse;i++){
		cassa[i].idcassa=i;
		cassa[i].countprod=0;
		cassa[i].countclients=0;
		cassa[i].optime=NULL;
		cassa[i].lastoptime=NULL;
		cassa[i].pctime=NULL;
		cassa[i].lastpctime=NULL;
		cassa[i].closurecount=0;
		cassa[i].tempoprodotto=tempoprodotti;
		cassa[i].filaclienti.nprodclienteservito=0;
		cassa[i].filaclienti.idclienteservito=-1;
		cassa[i].filaclienti.opencassa=0;
		cassa[i].filaclienti.numincoda=0;
		cassa[i].filaclienti.servizioeffettuato=0;
		cassa[i].recordtempi=NULL;
		initcoda(&(cassa[i].filaclienti.codacassa),maxclienti/maxcasse);
		CONDINIT(cassa[i].filaclienti.attesacoda);
		CONDINIT(cassa[i].filaclienti.attesaservizio);
		MUTEXINIT(cassa[i].filaclienti.mutexcoda);
	}
	return cassa;
}
void initcassieretime(int attesaprod,int avvisi,struct timespec* waitprod,struct timespec* waitfissa,struct timespec*waitavvisi){
	struct timespec timeforseed;
	clock_gettime(CLOCK_BOOTTIME,&timeforseed);
	unsigned int seed=(timeforseed.tv_sec)+(timeforseed.tv_nsec);
	long long randnum;
	randnum=rand_r(&seed);
	waitfissa->tv_nsec=(randnum%(MAXWAIT+1-MINWAIT))+MINWAIT;
	waitfissa->tv_sec=0;
	waitprod->tv_sec=0;
	waitprod->tv_nsec =attesaprod*1000000;	
	waitavvisi->tv_sec=0;
	waitavvisi->tv_nsec=avvisi*1000000;
	return;
}
void freecorrtempi(tempiclienticassa **l){
	if(*l==NULL){
		return;
	}
	freecorrtempi(&((*l)->next));
	free(*l);
}
void freeopentime(instancetime**l){
	if(*l==NULL){
		return;
	}
	freeopentime(&((*l)->next));
	free(*l);
}
	
