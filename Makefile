CC 			= mpicc
DMRFLAGS 	= -ldmr
FLAGS  		= -Wall -g -lm

SOURCES		= src/test.c src/test_functions.c
OBJECTS		= $(SOURCES:.c=.o)

all: test

test: $(OBJECTS)
	$(CC) $(FLAGS) $(DMRFLAGS) -DDYNRES $(OBJECTS) -o test

%.o: %.c src/test.h
	$(CC) $(FLAGS) -DDYNRES -c $< -o $@

clean:
	rm -f test src/*.o slurm-*.out checkpoints/counters*
