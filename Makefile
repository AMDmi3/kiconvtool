CC?=		cc
CFLAGS+=	-Wall
LIBS=		-lkiconv -lmemstat

all: kiconvtool

kiconvtool: kiconvtool.c
	${CC} ${CFLAGS} kiconvtool.c ${LIBS} -o kiconvtool

README: kiconvtool.8
	groff -S -man -Tascii < kiconvtool.8 | col -b > README

clean:
	rm -f kiconvtool *.o
