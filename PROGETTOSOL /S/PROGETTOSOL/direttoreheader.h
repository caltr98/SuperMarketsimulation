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
int checkcliente(direttorecliente *collegamento);
void direttorechiusuracliente(direttorecliente *collegamento);
int checkopen(tocatcher*info);
void permessiuscita(direttorecliente **collegamento,int C,unsigned char*mapclienti);
int verificaS1S2(int S1,int S2,int *index,direttorecassiere**cassa,int num,unsigned char*map);
void impostatimeravvisi(int intervalloavvisi,void(*fun)(int));
void offtimer();
void permessiuscitaimmediata(direttorecliente **collegamento,int C,unsigned char*mapclienti);
