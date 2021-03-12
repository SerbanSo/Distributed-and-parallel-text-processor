CC=mpic++

efile = main
ofile = main.o MPI_workers.o processing.o

build: $(ofile)
	$(CC) $(ofile) -o $(efile)

%.o: %.cpp 
	$(CC) -c $<

run: $(efile)
	mpirun -np 5 main

make clean:
	rm *.o $(efile)
