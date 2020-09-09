#include "header.h"
#include "coda.h"
#include "clientiheader.h"
#include "cassaheader.h"
#include "direttoreheader.h"	


void raccoglidati(int numcasse);
infodirettore readconfiginitall(int argc,char*argv[]);
int getnum(char* string);
void checkarg(int val,int max,int min,char*param);
infodirettore initall(int k,int c,int e,int t,int p,int casinit,int tempprod,int intrv,int s1,int s2);
void freeall(int maxcasse,int maxclienti);

extern infoaffluenza infoclienti;
extern infocassa*cassa;	
void raccoglidati(int numcasse){
	FILE *fd;
	oldclientdata *l=infoclienti.clientreport;
	cliente *client;
	tempiclienticassa *record;
	instancetime *tempiapertura;
	int i;
	int err;
	FOPEN(fd,"./report.log","w+");
	err=fprintf(fd,"Statistiche generali\n");
	CHECKFPRINTF(err);
	fprintf(fd,"Clienti totali:%d-Prodotti acquistati:%d\n\nStatistiche clienti\n",infoclienti.clientitotali,infoclienti.acquistitotali);
	while(l!=NULL){
		cliente *client=l->oldclient;
		err=fprintf(fd,"cliente id:%d-tempo di permanenza:%fs-tempo in coda:%fs-cambi casse:%d\n",client->idcliente,client->tempopermanenza,client->tempoincoda,client->countcambiocoda);
		CHECKFPRINTF(err);
		err=fprintf(fd,"cliente id:%dnumero acquisti%d\n",client->idcliente,client->numeroprodotti);
			CHECKFPRINTF(err);
		//fprintf(fd,"cliente id:%d infoextra-tempoacquisti:%fs,tempoacquistidichiarato:%ld,acquistieffettuati:%d\n",client->idcliente,client->tempototaleacquisti,client->tempoacquisti.tv_nsec,client->pagacassa);
		//free(client);
		l=l->next;
	}
	err=fprintf(fd,"\n\nStatistiche delle %d casse",numcasse);
	CHECKFPRINTF(err);
	for(i=0;i<numcasse;i++){
		err=fprintf(fd,"\ncassa:%d-numero di clienti serviti:%d-tempi di servizio:\n",cassa[i].idcassa,cassa[i].countclients);
		CHECKFPRINTF(err);
		record=cassa[i].recordtempi;
		while(record!=NULL){
			err=fprintf(fd,"%f-",record->tempo);
			CHECKFPRINTF(err);
			record=record->next;
		}
		err=fprintf(fd,"\noccorrenze chiusura:%d\n",cassa[i].closurecount);
		CHECKFPRINTF(err);
		tempiapertura=cassa[i].optime;
		while(tempiapertura!=NULL){
			err=fprintf(fd,"%f-",tempiapertura->time);
			CHECKFPRINTF(err);
			tempiapertura=tempiapertura->next;
		}
		
	}	
	FCLOSE(fd);
	return;
}


infodirettore readconfiginitall(int argc,char*argv[]){
	FILE *fd;
	int sizebuf=256;
	char*s;
	MALLOC(s,sizebuf,char);
	int k,c,e,t,p,casinit,tempprod,intrv,s1,s2;
	if(argc>2){
		fprintf(stderr,"Numero di argomenti non ammesso\n");
		exit(EXIT_FAILURE);
	}
	if(argc==2){
		FOPEN(fd,argv[1],"r");
	}
	else FOPEN(fd,"config.txt","r");
	FGETS(s,sizebuf,fd);
	checkarg( (k=getnum(s),k) ,-1,0,"K") ;
	FGETS(s,sizebuf,fd);
	checkarg( (c=getnum(s)) ,-1,0,"C") ;
	FGETS(s,sizebuf,fd);
	checkarg( (e=getnum(s)) ,c,0,"E") ;
	FGETS(s,sizebuf,fd);
	checkarg( (t=getnum(s)) ,-1,10,"T") ;
	FGETS(s,sizebuf,fd);
	checkarg( (p=getnum(s)) ,-1,0,"P") ;
	FGETS(s,sizebuf,fd);
	checkarg( (casinit=getnum(s)) ,-1,1,"CASSEINIZIO") ;
	FGETS(s,sizebuf,fd);
	checkarg( (tempprod=getnum(s)) ,-1,0,"TEMPOGESTIONEPRODOTTO") ;
	FGETS(s,sizebuf,fd);
	checkarg( (intrv=getnum(s)) ,-1,0,"INTERVALLOAVVISI") ;
	FGETS(s,sizebuf,fd);
	checkarg( (s1=getnum(s)) ,-1,0,"S1") ;
	FGETS(s,sizebuf,fd);
	checkarg( (s2=getnum(s)) ,-1,0,"S2") ;
	free(s);
	FCLOSE(fd);
	return initall(k,c,e,t,p,casinit,tempprod,intrv,s1,s2);
}
infodirettore initall(int k,int c,int e,int t,int p,int casinit,int tempprod,int intrv,int s1,int s2){
	infodirettore toDirettore;
	toDirettore.T=t;
	toDirettore.P=p;
	toDirettore.C=c;
	toDirettore.E=e;
	toDirettore.K=k;
	toDirettore.S1=s1;
	toDirettore.S2=s2;
	toDirettore.intervalloavvisicoda=intrv;//in microsecondi
	toDirettore.casseaperteinizio=casinit;
	cassa=initcasse(k,tempprod,c);
	infoclienti.maxclienti=k;
	infoclienti.clientitotali=0;
	//MALLOC(infoclienti.clienti,infoclienti.maxclienti,cliente*);
	infoclienti.clientreport=NULL;
	infoclienti.clientreportlast=NULL;
	MUTEXINIT(infoclienti.reportmutex);
	return toDirettore;
}
int getnum(char* string){
	while(string[0]!='=' && string[0]!='\0'){
		++string;
	}
	if(string[0]=='\0'){
		CHECKFPRINTF(fprintf(stderr,"formattazione non rispettata\n"));
	}
	return strtol(++string,NULL,10);
}
void checkarg(int val,int max,int min,char*param){
	if(val<min|| (val>max && max!=-1) ){
		CHECKFPRINTF(fprintf(stderr,"Valore in'incongruente di %s\n",param));
		exit(EXIT_FAILURE);
	}
}
void freeall(int maxcasse,int maxclienti){
	int i;
	for(i=0;i<maxcasse;i++){
		freecorrtempi(&(cassa[i].recordtempi));
		freecoda(&(cassa[i].filaclienti.codacassa));
		freeopentime(&(cassa[i].optime));
	}
	free(cassa);
	freereport(&(infoclienti.clientreport));
	return;
}

