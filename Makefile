CC?=		cc
CFLAGS+=	-Wall
LIBS=		-lkiconv -lmemstat

PREFIX?=	/usr/local

all: kiconvtool

kiconvtool: kiconvtool.c
	${CC} ${CFLAGS} kiconvtool.c ${LIBS} -o kiconvtool

kiconv.sh: kiconv.sh.in
	sed -e 's|%%PREFIX%%|${PREFIX}|' <$> >$@

README: kiconvtool.8
	groff -S -man -Tascii < kiconvtool.8 | col -b > README

clean:
	rm -f kiconvtool *.o
