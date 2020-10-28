TGT = addFmaskSDS
OBJ = 	addFmaskSDS.o \
	dilation.o \
	lsat.o \
	lsatmeta.o \
	util.o \
	hdfutility.o

$(TGT): $(OBJ)
	$(CC) $(CFLAGS) -o $(TGT) $(OBJ) -L$(GCTPLIB) -L$(HDFLIB) -L$(ZLIB) -L$(SZLIB) -L$(JPGLIB) -L$(PROJLIB) $(GCTPLINK) $(HDFLINK)

addFmaskSDS.o: addFmaskSDS.c
	$(CC) $(CFLAGS) -c addFmaskSDS.c -I$(HDFINC) -I$(GCTPINC) -I$(SRC_DIR)

dilation.o: ${SRC_DIR}/dilation.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/dilation.c -I$(HDFINC) -I$(SRC_DIR)

lsat.o: ${SRC_DIR}/lsat.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/lsat.c -I$(HDFINC) -I$(SRC_DIR)

lsatmeta.o: ${SRC_DIR}/lsatmeta.c
	$(CC) $(CFLAGS) -c ${SRC_DIR}/lsatmeta.c -I$(HDFINC) -I$(SRC_DIR)

util.o: ${SRC_DIR}/util.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/util.c -I$(HDFINC) -I$(SRC_DIR)

hdfutility.o: ${SRC_DIR}/hdfutility.c
	$(CC) $(CFLAGS) -c  ${SRC_DIR}/hdfutility.c -I$(HDFINC) -I$(SRC_DIR)

install:
	install -m 755 $(TGT) /usr/bin
clean:
	rm -f *.o

