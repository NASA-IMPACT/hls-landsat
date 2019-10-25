# Note: before run make, do in the shell:
# source $HOME/code/share/makeenv/makeenv.sh
TGT = addFmaskSDS
OBJ = 	addFmaskSDS.o \
	lsat.o \
	lsatmeta.o \
	util.o \
	hdfutility.o 

$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB) $(GCTPLINK) $(HDFLINK)  

addFmaskSDS.o: addFmaskSDS.c 
	$(CC) $(CFLAGS) -c addFmaskSDS.c -I$(HDFINC) -I$(GCTPINC)

lsat.o: lsat.c
	$(CC) $(CFLAGS) -c lsat.c -I$(HDFINC)

lsatmeta.o: lsatmeta.c
	$(CC) $(CFLAGS) -c lsatmeta.c -I$(HDFINC)

util.o: util.c
	$(CC) $(CFLAGS) -c  util.c -I$(HDFINC)

hdfutility.o: hdfutility.c
	$(CC) $(CFLAGS) -c  hdfutility.c -I$(HDFINC)

install:
	install -m 755 $(TGT) /usr/bin
clean:
	rm -f *.o


