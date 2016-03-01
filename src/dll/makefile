# Makefile for the linked list library to radout/ddrad 

# You should not need to edit this file.

### The following variables are supplied by the global Makefile:
### CC       the compiler
### LINT     the code checker
### CFLAGS   compilation flags
### LDFLAGS  linker flags
### INCDIR   include directories
### LIBDIR   library directories
### PROGRAM  the name of the final product 

 

DEFS  = -DDLL_LIB

LIBNAME = dll
LIBRARY	      = lib$(LIBNAME).a

TESTPROG  = dltest


OBJS	      = dll.o  


SRCS	      = dll.c 


all: $(LIBRARY) # $(TESTPROG)


library: $(LIBRARY)


.c.o: ;
	$(CC) -c $(CFLAGS) $(INCDIR) $(DEFS) $< -o $@


$(TESTPROG): $(OBJS)
	@echo "Linking $(TESTPROG) ... "
	@$(CC) $(LDFLAGS) $(OBJS) -o $(TESTPROG).out
	@mv $(TESTPROG).out $(TESTPROG)


$(LIBRARY):	$(OBJS)
	@rm -f $@
	@ar ru $(LIBRARY) $(OBJS)


clean:;
	rm -f *.o *.a *.out $(TESTPROG)



lint:
	$(LINT) $(LINTARGS) $(INCDIR) $(PROINC) $(SRCS)




dll.o: dlltypes.h dllproto.h
