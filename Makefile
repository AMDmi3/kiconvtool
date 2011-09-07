CC?=		cc
CFLAGS+=	-Wall
LIBS=		-lkiconv -lmemstat

PREFIX?=	/usr/local

BSD_INSTALL_PROGRAM?=	install -s -m 555
BSD_INSTALL_SCRIPT?=	install -m 555
BSD_INSTALL_MAN?=	install -m 444

all: kiconvtool kiconv.sh

install: kiconvtool kiconv.sh
	mkdir -p ${PREFIX}/sbin/ ${PREFIX}/etc/rc.d/ ${PREFIX}/man/man8/
	${BSD_INSTALL_PROGRAM} kiconvtool ${PREFIX}/sbin/
	${BSD_INSTALL_SCRIPT} kiconv.sh ${PREFIX}/etc/rc.d/kiconv
	${BSD_INSTALL_MAN} kiconvtool.8 ${PREFIX}/man/man8/

kiconvtool: kiconvtool.c
	${CC} ${CFLAGS} $> ${LIBS} -o $@

kiconv.sh: kiconv.sh.in
	sed -e 's|%%PREFIX%%|${PREFIX}|' < $> > $@

README: kiconvtool.8
	groff -S -man -Tascii < $> | col -b > $@

clean:
	rm -f kiconvtool kiconv.sh
