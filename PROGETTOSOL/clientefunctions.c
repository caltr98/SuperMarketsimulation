#include "header.h"
#include "coda.h"
#include "cassaheader.h"
typedef struct cliente_{
	int idcliente;
	struct timespec tempoacquisti;
	double tempototaleacquisti;
	int numeroprodotti;
	double tempopermanenza;
	double tempoincoda;
	int countcambiocoda;
	int maxnumcasse;
	int pagacassa;
}cliente;	

typedef struct direttorecliente_{
	int indexcliente;
	int clienteattivo;
	int T;
	int P;
	int K;
	pthread_cond_t attesapermesso;
	int permessouscita;
	int chiusuraimmediata;
	pthread_mutex_t permessomutex;
	pthread_mutex_t chiusuramutex;
}direttorecliente;
typedef struct oldclientdata_{
	cliente *oldclient;
	struct oldclientdata_* next;
}oldclientdata;
typedef struct infoaffluenza{
	int maxclienti;
	int clientitotali;
	int acquistitotali;
	oldclientdata *clientreport;
	oldclientdata *clientreportlast;
	pthread_mutex_t reportmutex;
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











void insertOPoldclientdata(cliente *me,oldclientdata **lis,oldclientdata **last){
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
	clock_gettime(CLOCK_BOOTTIME,&start);
	int chiudi=0;
	int acquistieffettuati=0;
	while(!acquistieffettuati){
		if(nanosleep(&tempoacquisti,&residuo)==-1){
			if(errno==EINTR){
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
	clock_gettime(CLOCK_BOOTTIME,&end);
	*tempopermanenza=((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000);
	return;
}
void clienteatexit(void *arg){
	//printf("in\n");

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
	int Tnanosec=T*1000000; 
	clock_gettime(CLOCK_BOOTTIME,&timeforseed);
	unsigned int seed=(timeforseed.tv_sec)+(timeforseed.tv_nsec);
	long long randnum;
	//srand((unsigned)(time(NULL)));
	randnum=rand_r(&seed);
	MALLOC(me,1,cliente);
	MUTEXLOCK(&(infoclienti.reportmutex));//necessario per accesso univoco al campo clienti totali.
	++infoclienti.clientitotali;
	MUTEXUNLOCK(&(infoclienti.reportmutex));
	me->idcliente=indexcliente;
	me->tempototaleacquisti=0;

	me->tempopermanenza=0;
	me->tempoincoda=0;
	me->countcambiocoda=0;
	me->tempoacquisti.tv_sec=0;
	me->pagacassa=0;
	me->maxnumcasse=K;
	me->tempoacquisti.tv_nsec=(randnum%(Tnanosec+1-TENISMINTIME))+TENISMINTIME;
	me->numeroprodotti=rand_r(&seed)%P;
	
	return me;
}
int clienteincassa(cliente *me,int *chiusuraimmediata,pthread_mutex_t *accesscontrol){
	int i,numcasse,indexcassa;
	i=0;
	numcasse=me->maxnumcasse;
	indexcassa=-1;
	cassacliente *cassascelta;
	int successo=0;
	int chiudi=0;
	struct timespec timeforseed;
	clock_gettime(CLOCK_BOOTTIME,&timeforseed);
	unsigned int seed=(timeforseed.tv_sec)+(timeforseed.tv_nsec);
	int restaopen;
	struct timespec start;
	struct timespec end;
	clock_gettime(CLOCK_BOOTTIME,&start);
	i=rand_r(&seed)%me->maxnumcasse;

	fflush(stdout);
	while(!successo){
		cassascelta=&(cassa[i].filaclienti);
		MUTEXLOCK(&(cassascelta->mutexcoda));
		if(cassascelta->opencassa){
			++cassascelta->numincoda;
			accoda(&(cassascelta->codacassa),me->idcliente);
			while((restaopen=cassascelta->opencassa) && cassascelta->idclienteservito!=me->idcliente){
				CONDWAIT(&(cassascelta->attesacoda),&(cassascelta->mutexcoda));
			}
			if(!restaopen){
				printf("cassa %d chiusa mentre in coda\n",i);
			}
			else if(cassascelta->idclienteservito==me->idcliente){
				cassascelta->nprodclienteservito=me->numeroprodotti;//invio alla cassa numero di acquisti
				CONDSIGNAL(&(cassascelta->attesaservizio));//notifico alla cassa di aver inviato il numero
				while(!cassascelta->servizioeffettuato){
						CONDWAIT(&(cassascelta->attesaservizio),&(cassascelta->mutexcoda));
				}
				cassascelta->servizioeffettuato=0;
				CONDSIGNAL(&(cassascelta->attesaservizio));
				MUTEXUNLOCK(&(cassascelta->mutexcoda));
				successo=1;
				indexcassa=i;
			}
		}
		if(!successo){
			MUTEXUNLOCK(&(cassascelta->mutexcoda));
			MUTEXLOCK(accesscontrol);
			chiudi=*chiusuraimmediata;
			MUTEXUNLOCK(accesscontrol);
			if(chiudi){

				clock_gettime(CLOCK_BOOTTIME,&end);
				me->tempoincoda= ((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000);
				pthread_exit(NULL);
			}
			i=rand_r(&seed)%me->maxnumcasse;	
		}
	}
	clock_gettime(CLOCK_BOOTTIME,&end);
	me->tempoincoda= ((end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/(double)1000000000);

	return successo;
}
void uscitasenzaacquisti(direttorecliente *collegamento){
	//richiedi permesso
	MUTEXLOCK(&(collegamento->permessomutex));
	collegamento->permessouscita=-1;
	while(collegamento->permessouscita==-1){
		CONDWAIT(&(collegamento->attesapermesso),&(collegamento->permessomutex));
	}
	MUTEXUNLOCK(&(collegamento->permessomutex));
	pthread_exit(NULL);
}
void freereport(oldclientdata**l){
	if(*l==NULL){
		return;
	}
	//printf("vorrei cancellare:%d\n",todelete->idcliente);
	//fflush(stdout);
	freereport(&((*l)->next));
	free((*l)->oldclient);
	free(*l);
}
	
