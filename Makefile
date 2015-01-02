CC=gcc
CFLAGS=-I.
DEPS = http.h
OBJ = http.o srv.o
LIBS = -lev
EXE = ksrv

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ksrv: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f *.o *~ $(EXE)