#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>
#include <stdint.h>
#include <omp.h>
#include <x86intrin.h>
#include <emmintrin.h>
#include <tbb/tbb.h>
#include <tbb/parallel_for.h>

#define pixel(x,y) pixels[((y)*size)+(x)]
#define CHUNK_SIZE 1024
//#define DEBUG

int *pixels;
long long int size;

// - We use SSE instruction (128 bit) to achieve around 4 speedup
// - More speedup is possible if we other sse extension 
// - Now I assume that our processor has SSE and SSE2 support
// - use tbb simple partioner and chunk size of 1024 for partitioning into threads
// - I use single point precision float, so the output might be slightly different 
// from the double precision output.
class Rowtransform {
		
public:
	long long int mid;
	Rowtransform(long long int mid):mid(mid) { }
	void operator()(const tbb::blocked_range<long long int> & r) const {
		for(long long int y = r.begin(); y < r.end(); y++) {
			long long int x;
			int begin;
			float sqrt_2 = sqrt(2);
			float sqrt_t[4] = { sqrt_2, sqrt_2, sqrt_2, sqrt_2 };
			long long int val = mid / 4;
			if(val >= 1) {
				begin = 4*val;
				for (x = 0; x < mid; x += 4) {
					//__m128i c = _mm_load_si128((__m128i*)&(pixel(x,y)));
					__m128i c = _mm_loadu_si128((__m128i*)&(pixel(x,y)));
					__m128i d = _mm_loadu_si128((__m128i*)&(pixel(mid+x,y)));
					__m128 sq_no = _mm_loadr_ps(sqrt_t);
					__m128i sum_t = _mm_add_epi32(c,d);
					__m128i sub_t = _mm_sub_epi32(c,d);
					__m128 sum_f = _mm_cvtepi32_ps(sum_t);
					__m128 sub_f = _mm_cvtepi32_ps(sub_t);
					__m128 sum_div = _mm_div_ps(sum_f,sq_no);
					__m128 sub_div = _mm_div_ps(sub_f,sq_no);
					__m128i sum_res = _mm_cvtps_epi32(sum_div);
					__m128i sub_res = _mm_cvtps_epi32(sub_div);
					//_mm_stream_si128((__m128i *) &(pixel(x,y)), sum_res);
					_mm_storeu_si128((__m128i *) &(pixel(x,y)), sum_res);
					_mm_storeu_si128((__m128i *) &(pixel(mid+x,y)), sub_res);
				}		
			}
			else {
				begin = 0;
			}
			//very small loop ..max no of iterations = 0..3
			//Dont use sse, if no of rem elements is less than 3, as we try to load 4 values at a time in sse
			for(x = begin; x < mid; x++) {
				int a = pixel(x,y);	
				a = (a+pixel(mid+x,y)) / sqrt_2;
				int d = pixel(x, y);
				d = (d-pixel(mid+x,y)) / sqrt_2;
				pixel(x,y) = a;
				pixel(mid+x,y) = d;

			}	
			
		
		}
	}

};
class Columntransform {
public:
	long long int mid;
	Columntransform(long long int mid):mid(mid) { }
	void operator()(const tbb::blocked_range<long long int> & r) const {
		for(long long int y = r.begin(); y < r.end(); y++) {
			long long int x;
			int begin;
			float sqrt_2 = sqrt(2);
			float sqrt_t[4] = { sqrt_2, sqrt_2, sqrt_2, sqrt_2 };
			long long int val = mid / 4;
			if(val >= 1) {
				begin = 4*val;
				for (x = 0; x < mid; x += 4) {
					__m128i c = _mm_loadu_si128((__m128i*)&(pixel(x,y)));
					__m128i d = _mm_loadu_si128((__m128i*)&(pixel(x,mid+y)));
					__m128 sq_no = _mm_loadr_ps(sqrt_t);
					__m128i sum_t = _mm_add_epi32(c,d);
					__m128i sub_t = _mm_sub_epi32(c,d);
					__m128 sum_f = _mm_cvtepi32_ps(sum_t);
					__m128 sub_f = _mm_cvtepi32_ps(sub_t);
					__m128 sum_div = _mm_div_ps(sum_f,sq_no);
					__m128 sub_div = _mm_div_ps(sub_f,sq_no);
					__m128i sum_res = _mm_cvtps_epi32(sum_div);
					__m128i sub_res = _mm_cvtps_epi32(sub_div);
					_mm_storeu_si128((__m128i *) &(pixel(x,y)), sum_res);
					//_mm_streamu_si128((__m128i *) &(pixel(x,mid+y)), sub_res);
					_mm_storeu_si128((__m128i *) &(pixel(x,mid+y)), sub_res);
				}		
			}
			else {
				begin = 0;
			}
			//very small loop ..max no of iterations = 0..3
			//Dont use sse, if no of rem elemants is less than 3, as we try to load 4 values at a time in sse
			for(x = begin; x < mid; x++) {
				int a = pixel(x,y);	
				a = (a+pixel(x,mid+y)) / sqrt_2;
				int d = pixel(x, y);
				d = (d-pixel(x,mid+y)) / sqrt_2;
				pixel(x,y) = a;
				pixel(x,mid+y) = d;

			}
		}
	}
};

void print(long long int size) {
	long long int x, y;
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			printf("%10d ", pixel(x,y));
		}
		printf("\n");
		fflush(stdout);
	}
}

int main(int argc, char *argv[]) {
    	if (argc != 3) {
		printf("usage: %s [input_file] [output_file]\n", argv[0]);
		return 0;
    	}

	FILE *in;
	FILE *out;

    	clock_t start;
    	clock_t end;

	in = fopen(argv[1], "rb");
	if (in == NULL) {
		perror(argv[1]);
		exit(EXIT_FAILURE);
	}

	out = fopen(argv[2], "wb");
	if (out == NULL) {
		perror(argv[2]);
		exit(EXIT_FAILURE);
	}

	long long int s, mid;
	long long int x, y;
	long long int a, d;

	fread(&size, sizeof(size), 1, in);

	fwrite(&size, sizeof(size), 1, out);

	pixels = (int *) malloc(size * size * sizeof(int));

    	start = clock();
	if (!fread(pixels, size * size * sizeof(int), 1, in)) {
		perror("read error");
		exit(EXIT_FAILURE);
	}
    	end = clock();
    	printf("time to read: %ju (%f s)\n", (uintmax_t)(end - start), (float)(end - start) / CLOCKS_PER_SEC);

#ifdef DEBUG
	printf("origin: %lld\n", size);
	print(size);
#endif

	// haar
	double st = omp_get_wtime();
	for (s = size; s > 1; s /= 2) {
		mid = s / 2;
		// row-transformation
		tbb::parallel_for(tbb::blocked_range<long long int>(0,mid,CHUNK_SIZE),
				Rowtransform(mid), tbb::simple_partitioner());
#ifdef DEBUG
		printf("after row-transformation: %lld\n", mid);
		print(size);
#endif

		// column-transformation
		tbb::parallel_for(tbb::blocked_range<long long int>(0,mid,CHUNK_SIZE),
				Columntransform(mid), tbb::simple_partitioner());
#ifdef DEBUG
		printf("after column-transformation: %lld\n", mid);
		print(size);
#endif

	}
	double ed = omp_get_wtime();
	printf("time to haar %.4lfs\n", ed - st);


	start = clock();

	fwrite(pixels, size * size * sizeof(int), 1, out);

	end = clock();
	printf("time to write: %ju (%f s)\n", (uintmax_t)(end - start), (float)(end - start) / CLOCKS_PER_SEC);

	free(pixels);

	fclose(out);
	fclose(in);

	return EXIT_SUCCESS;
}
