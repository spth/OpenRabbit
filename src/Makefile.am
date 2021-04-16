bin_PROGRAMS = openrabbit
dist_bin_SCRIPTS = openrabbitfu
openrabbit_SOURCES = openrabbit.c myio.c rabbit.c rabdata.c rabio.c rabmap.c ihex.c

# set the include path found by configure
AM_CPPFLAGS= $(all_includes)

openrabbit_CFLAGS = -pedantic

# the library search path.
openrabbit_LDFLAGS = $(all_libraries) 
noinst_HEADERS = myio.h mytypes.h rabbit.h rabdata.h rabio.h rabmap.h mytypes.h bios/tc_defs.lib bios/dkcore.lib
openrabbit_LDADD = -lncurses

