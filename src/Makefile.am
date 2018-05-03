ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
AM_CFLAGS=${common_CFLAGS}
AM_LDFLAGS=${common_LDFLAGS}
bin_PROGRAMS=hiveads dump1090d

hiveads_SOURCES=\
	hiveads.c \
	../src/devicehive-logger/src/data_logger_helper.c \
	../src/devicehive-logger/src/devicehive.c \
	../src/devicehive-logger/src/timestamp.c

dump1090d_SOURCES=\
	dump1090/dump1090.c \
	dump1090/anet.c
dump1090d_LDFLAGS=${common_LDFLAGS} -lrtlsdr
