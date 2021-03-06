# See LICENSE file for copyright and license details.

include config.mk

BIN = \
	punch \
	stat \
	wrkhr

OBJ = ${BIN:=.o} utils.o

MAN1 = ${BIN:=.1}
MAN5 = wd-syntax.5

all: options ${BIN}

${BIN}: ${@:=.o}
${BIN}: utils.o

punch.o: punch.c arg.h color.h config.h utils.h
stat.o: stat.c arg.h config.h utils.h
utils.o: utils.c config.h utils.h
wrkhr.o: wrkhr.c arg.h config.h utils.h

options:
	@echo ${NAME} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

clean:
	@echo cleaning
	@rm -f ${BIN} ${OBJ} ${NAME}-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p ${NAME}-${VERSION}
	@cp -R Makefile config.mk LICENSE README \
		${SRC} ${HDR} ${NAME}-${VERSION}
	@tar -cf ${NAME}-${VERSION}.tar ${NAME}-${VERSION}
	@gzip ${NAME}-${VERSION}.tar
	@rm -rf ${NAME}-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@for m in ${BIN} ; do cp -f "$$m" ${DESTDIR}${PREFIX}/bin/${NAME}"$$m" ; done
	@chmod 755 $(patsubst %, ${DESTDIR}${PREFIX}/bin/${NAME}%, ${BIN})
	@mkdir -p ${MANPREFIX}1
	@mkdir -p ${MANPREFIX}5
	@for m in ${MAN1} ; do cp -f "$$m" ${MANPREFIX}1/${NAME}"$$m" ; done
	@cp ${MAN5} ${MANPREFIX}5

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f $(patsubst %, ${DESTDIR}${PREFIX}/bin/${NAME}%, ${BIN}) \
		${MANPREFIX}5/${MAN5} \
		$(patsubst %, ${MANPREFIX}1/${NAME}%, ${MAN1})

.PHONY: all options clean dist install uninstall
