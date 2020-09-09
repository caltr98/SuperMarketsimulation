CC		=  gcc
CFLAGS= -Wall -pedantic
LDFLAGS 	= -L.
AR = ar
ARFLAGS= rvs
SUFFIX= -g -pthread

.PHONY:all clean test
TARGET		= supermercato
supermercato: supermercato.c libsupermercato.a
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) -lsupermercato $(SUFFIX)

libsupermercato.a:direttorefunctions.o clientefunctions.o cassafunctions.o coda.o 
	$(AR) $(ARFLAGS) $@ $^

direttorefunctions.o: direttorefunctions.c
	$(CC) $(CFLAGS) $< -c -o $@ $(SUFFIX)

clientefunctions.o: clientefunctions.c
	$(CC) $(CFLAGS) $< -c -o $@ $(SUFFIX)

cassafunctions.o: cassafunctions.c
	$(CC) $(CFLAGS) $< -c -o $@ $(SUFFIX)

coda.o: coda.c
	$(CC) $(CFLAGS) $< -c -o $@ -g

test :
	timeout --preserve-status -s HUP 10 ./supermercato

all : $(TARGET)
	

clean:
	-rm -f *.o *.a *.log supermercato
