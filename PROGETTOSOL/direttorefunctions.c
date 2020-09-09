#include "header.h"
#include "clientiheader.h"
typedef struct infodirettore_{
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

typedef struct tocatcher_{
	int chiusura;
	pthread_mutex_t infolock;
}tocatcher;
typedef struct direttorecassiere_{
	int cassa;
	int numeroclienticoda;
	int open;
	pthread_mutex_t accesslock;//da usare per accesso atomico ai campi open e numeroclienticoda
}direttorecassiere;
void direttorechiusuracliente(direttorecliente *collegamento);
int checkapertura(direttorecliente *collegamento);
int checkopen(tocatcher*info);
void permessiuscita(direttorecliente **collegamento,int C,unsigned char*mapclienti);
int verificaS1S2(int S1,int S2,int *index,direttorecassiere**cassa,int num,unsigned char*map);

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
int checkopen(tocatcher*info){
	int ris;
	MUTEXLOCK(&(info->infolock));
	ris=info->chiusura;
	MUTEXUNLOCK(&(info->infolock));
	return ris;
}
