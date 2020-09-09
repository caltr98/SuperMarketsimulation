CC		=  gcc
CFLAGS= -Wall -pedantic
LDFLAGS 	= -L.
AR = ar
ARFLAGS= rvs
SUFFIX= -g -pthread

.PHONY:all clean test
TARGET		= supermercato
supermercato: supermercato.c libsupermercato.a	header.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) -lsupermercato $(SUFFIX)

libsupermercato.a:utils.o direttorefunctions.o clientefunctions.o cassafunctions.o coda.o 
	$(AR) $(ARFLAGS) $@ $^

utils.o:utils.c utils.h
	$(CC) $(CFLAGS)	$< -c -o $@ -g

direttorefunctions.o: direttorefunctions.c direttoreheader.h
	$(CC) $(CFLAGS) $< -c -o $@ $(SUFFIX)

clientefunctions.o: clientefunctions.c clientiheader.h
	$(CC) $(CFLAGS) $< -c -o $@ $(SUFFIX)

cassafunctions.o: cassafunctions.c cassaheader.h
	$(CC) $(CFLAGS) $< -c -o $@ $(SUFFIX)

coda.o: coda.c coda.h
	$(CC) $(CFLAGS) $< -c -o $@ -g

test : supermercato
	timeout -s HUP --preserve-status 10 valgrind --leak-check=full ./supermercato &&\
	chmod +rx analisi.sh &&\
	./analisi.sh report.log 

all : $(TARGET)
	

clean:
	-rm -f *.o *.a *.log supermercato
