#include "header.h"
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
void resetcoda(coda *q);
int codapienalista(codalista *q);
void initcoda(coda *q,int initialcapacity){
	MALLOC(q->valori,initialcapacity,int);
	//q->valori=malloc(dim*sizeof(int));
	q->initialcapacity=initialcapacity;
	q->countinarray=0;
	q->head=0;
	q->tail=0;
	q->elements=0;
	initlista(&(q->lista));
	q->listaattiva=0;
	q->arrayvuoto=1;
}	
void resetcoda(coda *q){
	q->countinarray=0;
	q->head=0;
	q->tail=0;
	q->elements=0;
	q->listaattiva=0;
	q->arrayvuoto=1;
	liberacodalista(&(q->lista));
	initlista(&(q->lista));
}
void accoda(coda*q,int value){
	if(q->listaattiva==1){//metto valore solo e soltanto in lista se è attivata,fino a quando non sarà vuota
		accodalista(&(q->lista),value);
	}	
	else if(q->elements==q->initialcapacity){
		q->listaattiva=1;
		accodalista(&(q->lista),value);
	}
	else{
		if(q->arrayvuoto==1)
			q->arrayvuoto=0;
		q->valori[q->tail]=value;
		q->tail=(q->tail+1)%(q->initialcapacity);
		++q->countinarray;
	}
	++q->elements;
}

int decoda(coda*q){
	int ret;
	int nowempty=0;
	if(q->elements==0 && q->listaattiva==0){
		//printf("non posso decodare dato che non ci sono elementi\n");
		return -1;//SOLO INTERI POSITIVI NELLA MIA CODA
	}
	else if(!q->arrayvuoto){
		ret=q->valori[q->head];
		q->head=(q->head+1)%(q->initialcapacity);
		--q->countinarray;
		if(q->countinarray==0){
			q->arrayvuoto=1;
		}
	}
	else if(q->listaattiva==1){
		ret=decodalista(&(q->lista),&nowempty);
		q->listaattiva=!nowempty;
	}
	q->elements--;
	return ret;
}
int codavuota(coda*q){
	if(q->head==q->tail && q->elements==0 && q->listaattiva==0){
		return 1;
	}
	return 0;
}
void freecoda(coda *q){
    free(q->valori);
    q->head=q->tail=0;
	q->listaattiva=q->arrayvuoto=0;
	liberacodalista(&(q->lista));
}
void initlista(codalista*q){
	q->head=NULL;
	q->tail=NULL;
}
void accodalista(codalista*q,int value){
	node* new;//malloc(sizeof(node));
	MALLOC(new,1,node);
	new->info=value;
	new->next=NULL;
	if(q->head==NULL && q->tail==NULL){
		q->head=new;
		q->tail=new;
	}
	else{
		q->tail->next=new;
		q->tail=new;
	}
}
int decodalista(codalista*q,int *isempty){
	if(q->head==NULL && q->tail==NULL){
		*isempty=1;
		return -1;
	}
	int tr=q->head->info;
	node *tofree;
	if(q->head==q->tail){
		*isempty=1;
		q->tail=NULL;
		free(q->head);
		q->head=NULL;
	}
	else{
		isempty=0;
		tofree=q->head;
		q->head=q->head->next;
		free(tofree);
	}
	return tr;
}
void liberacodalista(codalista *q){
	node* corr=q->head;
	node*tofree;
	while(corr!=NULL){
		tofree=corr;
		corr=corr->next;
		free(tofree);
	}
}
