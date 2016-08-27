#include <stdio.h>
#include <stdlib.h>
#include "bucketsort.h"

#define LENGTH 8

FILE *fin, *fout;

char *strings;
long int N;

void openfiles(char* in, char* out) {
	fin = fopen(in, "r+");
	if (fin == NULL) {
		perror("fopen fin");
		exit(EXIT_FAILURE);
	}

}

void closefiles(void) {
	fclose(fin);
}

int main(int argc, char* argv[]) {
	long int i, *r;

    if (argc != 3) {
        printf("usage: %s [input_file] [output_file]\n", argv[0]);
        return 0;
    }

	openfiles(argv[1], argv[2]);

	fscanf(fin, "%ld", &N);
	strings = (char*) malloc(N * LENGTH);
	if (strings == NULL) {
		perror("malloc strings");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < N; i++)
		fscanf(fin, "%s", strings + (i * LENGTH));

	fflush(stdout);

	bucket_sort(strings, LENGTH, N, argv[2]);

	free(strings);
	closefiles();

	return EXIT_SUCCESS;
}
