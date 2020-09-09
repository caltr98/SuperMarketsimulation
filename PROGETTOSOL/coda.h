typedef struct _node{
	int info;
	struct _node* next;
}node;
typedef struct codalista_{
	node* head;
	node* tail;
	int numelementi;
}codalista;
typedef struct _coda{
	int	initialcapacity;
	int *valori;
	codalista lista;
	int head;
	int tail;
	int elements;
	int countinarray;
	int listaattiva;//se listaattiva==1 inserisco in coda solo nella parte coda lista, restituisco valori quando arrayvuoto==1
	int arrayvuoto;//se arrayvuoto==1 inizio a restituire valori dalla head della codalista
}coda;
void accoda(coda*q,int value);
void initcoda(coda*q,int initialcapacity);
int decoda(coda*q);
int codavuota(coda*q);
void freecoda(coda *q);

void accodalista(codalista*q,int value);
void initlista(codalista*q);
int decodalista(codalista*q,int*isempty);
int codavuotalista(codalista*q);
void liberacodalista(codalista *q);
int codapienalista(codalista *q);
void resetcoda(coda *q);
