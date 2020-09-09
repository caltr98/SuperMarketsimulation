#include "header.h"
#include "clientiheader.h"
typedef struct infodirettore_{/*struttura che contiene informazioni fondamentali per il thread direttore*/
	int K;//limite K:numero di casse
	int C;//limite C:numero di clienti attivi
	int E;//soglia C-E:dopo la quale far entrare nuovi clienti
	int T;//tempo da 10ms a T>10ms di acquisti cliente
	int P;//numero da 0 a P>0 di acquisti cliente
	int intervalloavvisicoda;
	int S1;
	int S2;
	int casseaperteinizio;
}infodirettore;

typedef struct tocatcher_{/*struttura per il thread posix catcher del direttore per comunicare segnale al direttore */
	int chiusura;
	pthread_mutex_t infolock;//per accesso in mutua esclusione a variabile chiusura
}tocatcher;
typedef struct direttorecassiere_{/*struttura comunicazione tra thread direttore e thread cassiere*/
	int cassa;//indica id della cassa
	int numeroclienticoda;//indica i clienti in coda della cassa
	int open;//indica se per il direttore la cassa debba rimanere aperta o chiusa(se chiusa=0 , se aperta =1)
	pthread_mutex_t accesslock;//da usare per accesso in mutua esclusione ai campi open e numeroclienticoda
}direttorecassiere;
void direttorechiusuracliente(direttorecliente *collegamento);
int checkapertura(direttorecliente *collegamento);
int checkopen(tocatcher*info);
void permessiuscita(direttorecliente **collegamento,int C,unsigned char*mapclienti);
int verificaS1S2(int S1,int S2,int *index,direttorecassiere**cassa,int num,unsigned char*map);
void impostatimeravvisi(int intervalloavvisi,void(*fun)(int));
void permessiuscitaimmediata(direttorecliente **collegamento,int C,unsigned char*mapclienti);
void offtimer();
void direttorechiusuracliente(direttorecliente *collegamento){
	MUTEXLOCK((&collegamento->chiusuramutex));
	collegamento->chiusuraimmediata=1;
	MUTEXUNLOCK(&(collegamento->chiusuramutex));
}
int checkcliente(direttorecliente *collegamento){
	int ris;
	MUTEXLOCK((&collegamento->chiusuramutex));
	ris=collegamento->clienteattivo;
	MUTEXUNLOCK(&(collegamento->chiusuramutex));
	return ris;
}


int verificaS1S2(int S1,int S2,int *index,direttorecassiere**cassa,int num,unsigned char*map){//ritorna 1 trova prima limite S1,2 se vale trova prima limite S2
//mette in index cassa da chiudere se vale S1
	int i,numcoda,counter;
	counter=0;
	for(i=0;i<num;i++){
		if(map[i]){
			MUTEXLOCK(&(cassa[i]->accesslock));
			numcoda=cassa[i]->numeroclienticoda;
			MUTEXUNLOCK(&(cassa[i]->accesslock));
			if(numcoda<2){
				if(++counter==S1){
					*index=i;
					return 1;
				}
			}
			if(numcoda>S2){
				return 2;
			}
		}
	}
	return 0;
}

void permessiuscita(direttorecliente **collegamento,int C,unsigned char*mapclienti){
	int i;
	for(i=0;i<C;i++){
		if(mapclienti[i]){
			MUTEXLOCK(&(collegamento[i]->permessomutex));
			if(collegamento[i]->permessouscita==-1){
				collegamento[i]->permessouscita=1;
				CONDSIGNAL(&(collegamento[i]->attesapermesso));
			}
			MUTEXUNLOCK(&(collegamento[i]->permessomutex));
		}
	}
}
void permessiuscitaimmediata(direttorecliente **collegamento,int C,unsigned char*mapclienti){//caso:SIGQUIT
	int i;
	for(i=0;i<C;i++){
		if(mapclienti[i]){
			MUTEXLOCK(&(collegamento[i]->permessomutex));
			collegamento[i]->permessouscita=1;//imposto il permesso di uscita ad 1 di dafault per ogni cliente
			CONDSIGNAL(&(collegamento[i]->attesapermesso));
			MUTEXUNLOCK(&(collegamento[i]->permessomutex));
		}
	}
}
int checkopen(tocatcher*info){
	int ris;
	MUTEXLOCK(&(info->infolock));
	ris=info->chiusura;
	MUTEXUNLOCK(&(info->infolock));
	return ris;
}
void impostatimeravvisi(int intervalloavvisi,void (*fun)(int)){
	sigset_t setmask;
	struct sigaction sa;
	struct itimerval timeval;
	SIGSETCHECK(sigemptyset(&setmask));
	SIGSETCHECK(sigaddset(&setmask,SIGALRM));
	THREADSIGMASK(SIG_UNBLOCK,&setmask,NULL);
	SIGACTION(SIGALRM,NULL,&sa);
	sa.sa_mask=setmask;
	sa.sa_handler=fun;
	SIGACTION(SIGALRM,&sa,NULL);
	timeval.it_interval.tv_usec=intervalloavvisi;//uso stesso intervallo delle casse per microsecondi
	timeval.it_interval.tv_sec=1;//aspetto in totale 1 secondo + microsecondi intervallo avvisi casse
	timeval.it_value=timeval.it_interval;
	SETITIMER(ITIMER_REAL,&timeval,NULL);
	return;
}
void offtimer(){
	struct itimerval timeval;
	SETITIMER(ITIMER_REAL,NULL,&timeval);
	timeval.it_value.tv_usec=0;
	timeval.it_value.tv_sec=0;
	SETITIMER(ITIMER_REAL,&timeval,NULL);
	return;
}
	
