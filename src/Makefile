# Generated automatically from Makefile.in by configure.
#!/bin/make -f
# $Header: /home/foxen/CR/FB/src/RCS/Makefile.in,v 1.1 1996/06/12 03:17:02 foxen Exp $

# #######################################################################
#                  Makefile for TinyMUCK 2.2fb5.x
# #######################################################################

# #######################################################################
#			User tunable options
# #######################################################################

#
# Prepend line.  used for compiling with things like nice.
#
# PRE= nice -10

#
# Optimizations
# -O			General optimizations, recomended.
# -O2			Heavier optimizations, use at own risk.
# -g			Generate debugging information, suggested, you can
#			alwase "strip netmuck" if you want the space.
# -Wall -pedantic	Good way to get spammed with warnings from GCC.
#
OPTIM= -O -g -Wno-write-strings

#
# Paths
#

# Right below the src/include directories
ROOT= ..

# Where the include files are
INCLUDE= ${ROOT}/include
LUA_INCLUDE= ${ROOT}/lua-5.2.0/src

# Where the lua libraries are
LUA_LIB= ${ROOT}/lua-5.2.0/src

# Destination for binaries
DEST=${ROOT}/bin

# #######################################################################
# 		Variables set by the configure script.
# #######################################################################
CC=g++
DEFS=-DHAVE_CONFIG_H
LIBR= -lm -lnsl -lssl -lxcrypt -L${LUA_LIB} -llua

# #######################################################################
#  	   Usualy don't need to change anything past here.
# #######################################################################

#
# Include path, Cflags...
#
CFLAGS= -Wall ${OPTIM} ${DEFS} -I${INCLUDE} -I${LUA_INCLUDE}

#
# for saving old copies.
#
MV= -mv -f
RM= -rm -f

#
# Misc stuff for odd targets.
#
SRC_PREFIX=
MKDEPEND= makedepend
MAKE= make
LINT= lint
MKTAGS= ctags
# for RCS only
GET= co
CLEAN= rcsclean
LINTFLAGS= -I${INCLUDE}

##########################################################################
#            You should not have to change anything below here           #
##########################################################################

MISCSRC= Makefile ChangeLog COPYING mkversion.sh

PROTSRC= boolexp.c case.c compile.c compress.c create.c db.c \
	debugger.c disassem.c diskprop.c edit.c events.c game.c hashtab.c \
	help.c inst.c interp.c log.c look.c \
	msgparse.c move.c crt_malloc.c oldcompress.c player.c predicates.c \
	p_connects.c p_db.c p_math.c p_misc.c p_props.c p_stack.c p_strings.c \
	property.c propdirs.c props.c rob.c rwho.c sanity.c set.c signal.c \
	smatch.c speech.c strftime.c stringutil.c timequeue.cpp timestamp.c \
	tune.c unparse.c utils.c wiz.c

CSRC= ${PROTSRC} \
	balloc.c \
	match.c \
	mfuns.c mfuns2.c \
	interpeter.cpp

LSRC= copyright.c
MSRC= olddecomp.c reconst.c interface.c

SRC= ${MISCSRC} ${CSRC} ${LSRC} ${MSRC}


COBJ= balloc.o boolexp.o case.o compile.o compress.o create.o db.o \
	debugger.o disassem.o diskprop.o edit.o events.o game.o hashtab.o \
	help.o inst.o interp.o interpeter.o log.o look.o match.o mfuns.o mfuns2.o \
	mlua.o msgparse.o move.o crt_malloc.o oldcompress.o player.o predicates.o \
	p_connects.o p_db.o p_math.o p_misc.o p_props.o p_stack.o p_strings.o \
	property.o propdirs.o props.o rob.o rwho.o sanity.o signal.o set.o \
	smatch.o speech.o strftime.o stringutil.o timequeue.o timestamp.o \
	tune.o unparse.o utils.o wiz.o

LOBJ= copyright.o
MOBJ= olddecomp.o sanity.o reconst.o interface.o resolver.o topwords.o
EOBJ= oldcompress.o diskprop.o property.o propdirs.o props.o stringutil.o \
	unparse.o utils.o predicates.o db.o boolexp.o log.o case.o

OBJ= ${COBJ} ${ROBJ} ${LOBJ} ${MOBJ}
TARGETS= netmuck resolver olddecompress topwords

.c.o:
	${PRE} ${CC} ${CFLAGS} -c $*.c

.cpp.o:
	${PRE} ${CC} ${CFLAGS} -c $*.cpp

all:    ${TARGETS} 

netmuck:        ${P} ${COBJ} interface.o ${LOBJ} mkversion.sh
	sh mkversion.sh
	${PRE} ${CC} ${CFLAGS} -c version.c
	${MV} netmuck netmuck~
	${PRE} ${CC} ${CFLAGS} -o netmuck ${COBJ} interface.o version.o \
	 ${LIBR} ${LOBJ}

resolver:	resolver.o ${LOBJ}
	${PRE} ${CC} ${CFLAGS} -o resolver resolver.o ${LIBR} ${LOBJ}

extract: ${P} extract.o ${EOBJ} ${LOBJ}
	 ${RM} extract
	 ${CC} ${CFLAGS} -o extract ${EOBJ} extract.o ${LOBJ}

topwords: topwords.o hashtab.o
	${RM} topwords
	${CC} ${CFLAGS} -o topwords topwords.o hashtab.o

olddecompress: olddecomp.o oldcompress.o ${LOBJ}
	${RM} olddecompress
	${CC} ${CFLAGS} -o olddecompress olddecomp.o oldcompress.o ${LOBJ}

#############################################################
# Funky stuff for debugging and coding work.
#

saber_src:
	#load ${CFLAGS} ${CSRC}

saber_obj: ${CSRC} interface.o
	#load ${OBJ} interface.o

tags: ${CSRC} ${MSRC}
	${MKTAGS} ${CSRC} interface.c ../include/*.h

lint: ${CSRC} ${MSRC}
	${RM} netmuck.lint
	${LINT} ${LINTFLAGS} ${CSRC} ${MSRC} > netmuck.lint

${SRC}:
	${GET} $@

sources: ${SRC}

names:
	@for i in ${SRC}; do echo ${SRC_PREFIX}$$i; done

depend:
	${MKDEPEND} -- ${CFLAGS} -- ${CSRC} ${MSRC} ${LSRC}
	echo chmod -w Makefile
	chmod -w Makefile

prototype:
	cproto -DNOPROTO -I ../include/ -o ../include/proto.h ${PROTSRC} interface.c version.c

########################################################
# Cleanup stuff.
#

clean:
	${RM} ${OBJ} core version.o ${LOBJ} ${SOBJ} resolver.o

cleaner: clean
	${RM} Makefile config.status ${INCLUDE}/autoconf.h ${TARGETS}
	cp Makefile.cfg Makefile

clobber: cleaner
	${RM} tags *~ *.bak *.orig

nuke:
	${RM} ${OBJ} core version.o
	${RM} ${TARGETS} tags *~ *.bak *.orig
	${RM} Makefile config.status ${INCLUDE}/autoconf.h
	cp Makefile.cfg Makefile
	-${CLEAN} *

install: all
	${MV} ${TARGETS} ${DEST}

# #######################################################################
# #######################################################################

# DO NOT DELETE THIS LINE -- make depend depends on it.
