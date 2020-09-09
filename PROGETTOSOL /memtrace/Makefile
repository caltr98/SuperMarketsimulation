
#
# set CPPFLAGS and LDFLAGS to the locations of <bfd.h>, libbfd and libiberty
#

CC       = gcc
CFLAGS   = -g -fno-omit-frame-pointer
CPPFLAGS = -I/local/include
LDFLAGS  = -L/local/lib
LIBS     = -lbfd -liberty

all:		leak

clean:
	rm -rf leak core *.o *~

leak:	leak.o memtrace.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

leak.o:	leak.c memtrace.h
memtrace.o: memtrace.c memtrace.h


