#
# Make file for MPI_Profiler
CC = gcc
MPICC = mpicc
CFLAGS  = -g -Wall -w
Pflag = - lpthread
EXECFILES = app

# typing 'make' will invoke the first target entry in the file 
# (in this case the default target entry)
# you can name this target entry anything, but "default" or "all"
# are the most commonly used names by convention
#
default: app

# Make for MPI red Black code
#
app:  app.c 
	$(MPICC) $(CFLAGS)  -c MPI_Profiler.c -o libprofiler.a
	$(MPICC) $(CFLAGS) -o app app.c -L. -lprofiler
# To start over from scratch, type 'make clean'.  This
# removes the executable file, as well as old .o object
# files and *~ backup files:
#
clean: 
	$(RM) count *.o *~
	$(RM) count *.a *~
	$(RM) $(EXECFILES)

