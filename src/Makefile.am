bin_PROGRAMS = openrabbit
openrabbit_SOURCES = openrabbit.c myio.c rabbit.c rabdata.c rabio.c rabmap.c

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
openrabbit_LDFLAGS = $(all_libraries) 
noinst_HEADERS = myio.h mytypes.h rabbit.h rabdata.h rabio.h rabmap.h mytypes.h
openrabbit_LDADD = -lncurses
