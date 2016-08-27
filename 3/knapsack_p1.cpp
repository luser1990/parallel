#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <omp.h>

typedef struct _item_t {
	int value;   
	int weight;  
} item_t;


int knapsack_f(const int n, const int M, const item_t * const itens) {
	
	int i = 0, w = 0, k = 0;
	int knap[M+1];
	int max;
	knap[0] = 0;
	omp_lock_t lock[M+1];
	#pragma omp parallel for
	for(int i=0;i<=M;i++) {
		knap[i] = 0;
	}
	// Use bottom up appraoch, Now here we find the max profit for each weight by iterating
	// through all items(for each weight). So if the no of items >= MAX Capacity , 
	// this scales linearly
	for(w = 1; w <= M; w++) {
		max = 0;
		#pragma omp parallel for reduction(max : max)
		for(i = 0; i < n; i++) {
			if(w >= itens[i].weight) {
				int val = knap[w - itens[i].weight] + itens[i].value;
				if(val > max)
					max = val;
			}
		}
		knap[w] = max;
		

	}

	return knap[M];

}

/* 2nd approach, has some inconsistency for large inputs, works good for medium input
Reason: This method is thread-safe for concurrent reads, and also while growing the vector, 
as long as the calling thread has checked that index
int knapsack_new(const int n, const int M, const item_t * const itens) {
    tbb::concurrent_vector<int> knap;
    int i = 0;
    for(i = 0; i <= M; i++) {
	knap.push_back(0);
    }
    tbb::parallel_for(tbb::blocked_range<int>(0,n), 
		    [&](tbb::blocked_range<int> r) {
    for(int i = r.begin(); i < r.end(); i++)
    for(int j = itens[i].weight;j <= M; j++) {
      knap[j] = std::max(knap[j], knap[j - itens[i].weight]+itens[i].value);
    }
    });
    return knap[M];
}
*/

int main(int argc, const char *argv[]) {

	int n;
	int M;
	item_t *itens;

	int i;

	scanf("%d %d", &n, &M);
	itens = (item_t*) calloc(n, sizeof(item_t));

	for (i = 0; i < n; ++i) {
		scanf("%d %d", &(itens[i].value), &(itens[i].weight));
	}

	std::cout << knapsack_f(n, M, itens) << std::endl;

	free(itens);

	return 0;

}


