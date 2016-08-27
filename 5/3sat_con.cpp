#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <omp.h>
#include <semaphore.h>
boost::atomic<long> atom{-1};
boost::atomic<int> count{0};
sem_t sync_t;
// Just to check if the solution is valid or not for debugging purpose
void solution_check(short **clauses, long number, int nClauses, int nVar) {
	short var;
	int i, c;
	long *iVar = (long *) malloc(nVar * sizeof(long));
	for (i = 0; i < nVar; i++)
		iVar[i] = exp2(i);
	for (c = 0; c < nClauses; c++) {

		var = clauses[0][c];
		if (var > 0 && (number & iVar[var - 1]) > 0)
			continue; // clause is true
		else if (var < 0 && (number & iVar[-var - 1]) == 0)
			continue; // clause is true

		var = clauses[1][c];
		if (var > 0 && (number & iVar[var - 1]) > 0)
			continue; // clause is true
		else if (var < 0 && (number & iVar[-var - 1]) == 0)
			continue; // clause is true

		var = clauses[2][c];
		if (var > 0 && (number & iVar[var - 1]) > 0)
			continue; // clause is true
		else if (var < 0 && (number & iVar[-var - 1]) == 0)
			continue; // clause is true

		break; // clause is false
	}
	if(c == nClauses)
		std::cout << "SOLUTION IS VALID" << std::endl;
}

// Solves the 3-SAT system using an exaustive search
// It finds all the possible values for the set of variables using
// a number. The binary representation of this number represents 
// the values of the variables. Since a long variable has 64 bits, 
// this implementations works with problems with up to 64 variables.

void solveClauses(short **clauses, int nClauses, int nVar, int core, int inc) {

	long *iVar = (long *) malloc(nVar * sizeof(long));
	int i;
	for (i = 0; i < nVar; i++)
		iVar[i] = exp2(i);

	unsigned long maxNumber = exp2(nVar);
	long number;
	short var;
	int c;

	for (number = core; number < maxNumber; number += inc) {

		for (c = 0; c < nClauses; c++) {

			var = clauses[0][c];
			if (var > 0 && (number & iVar[var - 1]) > 0)
				continue; // clause is true
			else if (var < 0 && (number & iVar[-var - 1]) == 0)
				continue; // clause is true

			var = clauses[1][c];
			if (var > 0 && (number & iVar[var - 1]) > 0)
				continue; // clause is true
			else if (var < 0 && (number & iVar[-var - 1]) == 0)
				continue; // clause is true

			var = clauses[2][c];
			if (var > 0 && (number & iVar[var - 1]) > 0)
				continue; // clause is true
			else if (var < 0 && (number & iVar[-var - 1]) == 0)
				continue; // clause is true

			break; // clause is false
		}

		if (c == nClauses) {
			// If this not atomic, then there is a chance that 
			// two threads might right into number variable at the
			// same time and might result in inconsistency
			atom.store(number, boost::memory_order_release);
			sem_post(&sync_t);
			break;
		}

	}
	if(c != nClauses) {
		count++;
	// this is required if we dont find a solution and want to signal to 
	// main thread that no of count == no of processors and no thread
	// was able to find a solution
		if(count == inc)
			sem_post(&sync_t);
	}

}

// Read nClauses clauses of size 3. nVar represents the number of variables
// Clause[0][i], Clause[1][i] and Clause[2][i] contains the 3 elements of the i-esime clause.
// Each element of the caluse vector may contain values selected from:
// k = -nVar, ..., -2, -1, 1, 2, ..., nVar. The value of k represents the index of the variable.
// A negative value remains the negation of the variable.
short **readClauses(int nClauses, int nVar) {

	short **clauses = (short **) malloc(3 * sizeof(short *));
	clauses[0] = (short *) malloc(nClauses * sizeof(short));
	clauses[1] = (short *) malloc(nClauses * sizeof(short));
	clauses[2] = (short *) malloc(nClauses * sizeof(short));

	int i;
	for (i = 0; i < nClauses; i++) {
		scanf("%hd %hd %hd", &clauses[0][i], &clauses[1][i], &clauses[2][i]);
	}

	return clauses;
}

int main(int argc, char *argv[]) {

	int nClauses;
	int nVar;
	boost::asio::io_service ioservice;
	boost::thread_group threads;
	boost::asio::io_service::work work(ioservice);
	int hardware_threads = boost::thread::hardware_concurrency();
	for(int i = 0; i < hardware_threads; i++) {
		threads.create_thread(boost::bind(&boost::asio::io_service::run, &ioservice));
	}
	sem_init(&sync_t, 0, 0);

	scanf("%d %d", &nClauses, &nVar);

	short **clauses = readClauses(nClauses, nVar);



	for(int core = 0; core < hardware_threads; core++) {
		ioservice.post(boost::bind(solveClauses, clauses, nClauses, nVar, core, hardware_threads));
	}
	sem_wait(&sync_t);
	ioservice.stop();
	//threads.join_all();
	long solution = atom.load(boost::memory_order_consume);
	int i;
	if (solution >= 0) {
		printf("Solution found [%ld]: ", solution);
		for (i = 0; i < nVar; i++)
			printf("%d ", (int) ((solution & (long) exp2(i)) / exp2(i)));
		printf("\n");
		//solution_check(clauses, solution, nClauses, nVar);
	} else
		printf("Solution not found.\n");
	//free(clauses);
	
	return EXIT_SUCCESS;
}

