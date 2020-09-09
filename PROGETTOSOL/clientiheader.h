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
void clienteatexit3(void *arg);
void uscitasenzaacquisti(direttorecliente *collegamento);
void freereport(oldclientdata**l);


