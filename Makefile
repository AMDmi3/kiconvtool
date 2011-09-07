CC?=		cc
CFLAGS+=	-Wall
LIBS=		-lkiconv -lmemstat

PREFIX?=	/usr/local

all: kiconvtool

kiconvtool: kiconvtool.c
	${CC} ${CFLAGS} $> ${LIBS} -o $@

kiconv.sh: kiconv.sh.in
	sed -e 's|%%PREFIX%%|${PREFIX}|' < $> > $@

README: kiconvtool.8
	groff -S -man -Tascii < $> | col -b > $@

clean:
	rm -f kiconvtool *.o kiconv.sh
