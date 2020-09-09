typedef struct direttorecassiere_{
	int cassa;
	int numeroclienticoda;
	int open;
	int intervalloavvisi;
	pthread_mutex_t accesslock;//da usare per accesso atomico ai campi open e numeroclienticoda
}direttorecassiere;
typedef struct argcomunicatore_{
	direttorecassiere*info;
	struct timespec intervallo;
	int internalcounter;
	pthread_mutex_t countermutex;
}argcomunicatore;

typedef struct instancetime_{
	double time;
	struct instancetime_*next;
}instancetime;
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
