#
# Make file for REB BLACK PROGRAM
CC = gcc
MPICC = mpicc
CFLAGS  = -g -Wall
Pflag = - lpthread
EXECFILES = seq-rb mt-rb dist-rb hybrid-rb

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: all

# To create the executable file count we need the object files
# countwords.o, counter.o, and scanner.o:
#
all: seq-rb

# Make for sequential red black code
#
seq-rb:  seq-rb.c
	$(CC) -o0 -o seq-rb seq-rb.c -w

# Make for multithreaded red black code
#
mt-rb:  mt-rb.c 
	$(MPICC) -o0 -o mt-rb mt-rb.c -lpthread -lm -w

# Make for MPI red Black code
#
dist-rb:  dist-rb.c 
	$(MPICC) -o0 -o dist-rb dist-rb.c  -lm -w

# Make for Hybrid RedBlack Code
#
hybrid-rb :  hybrid-rb.c 
	$(MPICC) -o0 -o hybrid-rb hybrid-rb.c  -lm -w -fopenmp

# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) count *.o *~
	$(RM) $(EXECFILES)

