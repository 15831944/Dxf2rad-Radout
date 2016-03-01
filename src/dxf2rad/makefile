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

PROJLIBS = ../dxfconv/libdxfconv.a ../geom/libgeom.a ../dll/libdll.a -lm

SRCS    = dxf2rad.c \
		writerad.c

OBJS    = dxf2rad.o \
		writerad.o

all: $(PROGRAM)

.c.o: ;
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@

$(PROGRAM): $(OBJS) ../dll/libdll.a ../geom/libgeom.a ../dxfconv/libdxfconv.a 
	$(CC) $(CFLAGS) $(OBJS) $(PROJLIBS) -o dxf2rad

clean:;
	rm -f *.o $(PROGRAM) a.out


lint:
	$(LINT) $(LINTARGS) $(INCDIR) $(PROINC) $(SRCS)



dxf2rad.o: ../dxfconv/readdxf.h ../geom/geomtypes.h ../dxfconv/convert.h
dxf2rad.o: ../dxfconv/tables.h
writerad.o: ../geom/geomtypes.h ../geom/geomdefs.h ../dll/dlltypes.h
writerad.o: ../dll/dllproto.h ../geom/geomproto.h
