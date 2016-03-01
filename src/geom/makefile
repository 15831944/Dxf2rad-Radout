# Makefile for the geometry library to radout/dxf2rad

# You should not need to edit this file.

### The following variables are supplied by the global Makefile:
### CC       the compiler
### LINT     the code checker
### CFLAGS   compilation flags
### LDFLAGS  linker flags
### INCDIR   include directories
### LIBDIR   library directories
### ADSDIR   ads library directory 
### PROGRAM  the name of the final product 



BASENAME = geom
LIBRARY = lib$(BASENAME).a


HDRS    = geomdefs.h \
		geomproto.h \
		geomtypes.h

SRCS    = m4geom.c \
		m4inv.c \
		m4mat.c \
		m4post.c \
		poly.c \
		polycheck.c \
		bulge.c \
		v3vec.c

LIBOBJS = m4geom.o \
		m4inv.o \
		m4mat.o \
		m4post.o \
		poly.o \
		polycheck.o \
		bulge.o \
		v3vec.o



all: $(LIBRARY)


.c.o: ;
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@


$(LIBRARY): $(LIBOBJS)
	@rm -f $@
	@ar ru $(LIBRARY) $(LIBOBJS)


clean:;	rm -f *.o *.a



lint:
	$(LINT) $(LINTARGS) $(INCDIR) $(PROINC) $(SRCS)





m4geom.o: geomtypes.h 
m4geom.o: geomdefs.h geomproto.h
m4inv.o: geomtypes.h geomdefs.h geomproto.h
m4mat.o: geomtypes.h geomdefs.h geomproto.h
m4post.o: geomtypes.h geomdefs.h geomproto.h
poly.o: geomtypes.h
poly.o: geomdefs.h geomproto.h
polycheck.o: geomtypes.h geomdefs.h geomproto.h
bulge.o: geomdefs.h geomproto.h
v3vec.o: geomtypes.h geomdefs.h geomproto.h
