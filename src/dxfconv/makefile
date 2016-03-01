### Makefile for the main source directory of torad/ddrad

# You should not need to edit this file.

### The following variables are supplied by the global Makefile:
### CC       the compiler
### LINT     the code checker
### CFLAGS   compilation flags
### LDFLAGS  linker flags
### INCDIR   include directories
### LIBDIR   library directories
### PROGRAM  the name of the final product

#PROJLIBS = ../geom/libgeom.a ../dll/libdll.a -lm

LIBNAME = dxfconv
LIBRARY = lib$(LIBNAME).a

SRCS    = readdxf.c \
		getopt.c \
		convert.c \
		tables.c

OBJS    = readdxf.o \
		getopt.o \
		convert.o \
		tables.o

all: $(LIBRARY)

library: $(LIBRARY)

.c.o: ;
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@

$(LIBRARY): $(OBJS)
	@rm -f $@
	@ar ru $(LIBRARY) $(OBJS)

clean:;
	rm -f *.o *.a *.out


lint:
	$(LINT) $(LINTARGS) $(INCDIR) $(PROINC) $(SRCS)



readdxf.o: ../geom/geomtypes.h ../geom/geomdefs.h ../geom/geomproto.h
readdxf.o: readdxf.h convert.h
convert.o: readdxf.h ../geom/geomtypes.h convert.h tables.h
convert.o: ../geom/geomdefs.h ../geom/geomproto.h ../dxf2rad/writerad.h
convert.o: ../dll/dlltypes.h
tables.o: ../dll/dlltypes.h ../dll/dllproto.h ../geom/geomtypes.h tables.h
