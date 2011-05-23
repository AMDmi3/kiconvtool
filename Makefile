CC?=		cc
CFLAGS+=	-Wall
LIBS=		-lkiconv -lmemstat

all: kiconvtool

kiconvtool: kiconvtool.c
	${CC} ${CFLAGS} kiconvtool.c ${LIBS} -o kiconvtool

clean:
	rm -f kiconvtool *.o
