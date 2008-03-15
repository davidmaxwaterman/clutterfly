LIBS=`pkg-config --libs clutter-0.6`
INCS=`pkg-config --cflags clutter-0.6`

all: clutterfly

clutterfly: clutterfly.c

.c:
	$(CC) $(INCS) $@.c $(LIBS) -o $@

.c.o:
	$(CC) -g -Wall $(CFLAGS) $(INCS) -c $*.c

clean:
	rm -fr *.o
