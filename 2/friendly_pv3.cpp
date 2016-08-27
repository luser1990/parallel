#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <string.h>
#include <boost/math/common_factor.hpp>
#include <boost/unordered_map.hpp>
#include <omp.h>
#include <boost/lexical_cast.hpp>
#include <tbb/concurrent_unordered_map.h>
using namespace std;

typedef boost::packaged_task<int> task;
typedef boost::shared_ptr<task> ptask;


int factor(long int ii, long int the_num, long int *num, long int *den, \
		tbb::concurrent_unordered_multimap<string,long int> *hashmap) {
	long int max = sqrt(the_num);
	long int sum = 1;
	long int i, n;
	

	for(i = 2; i <= max; i++)
	{
		if(the_num % i == 0)
		{
			sum += i;
			int d = the_num / i;
			if(d != i)
				sum += d;
		}
	}
	*num = sum;
	*den = the_num;
	n = boost::math::gcd(*num, *den);
	*num /= n;
	*den /= n;
	std::stringstream str1;
	str1 << *num << "," << *den;
	hashmap->insert(std::pair<string,long int>(str1.str(), ii));
	return 0;
}

/* we have already added entries into hash table, Now check the array the_num w.r.t hashmap if there is entry */
int hashmap_compute(long int *the_num, long int *num, long int *den, long int last, int core, int hardware_threads, \
		tbb::concurrent_unordered_multimap<string,long int> *hashmap, \
		tbb::concurrent_unordered_map<string, long int> *counts) {
	for (long int i = core; i < last; i += hardware_threads) { 
		std::stringstream str1;
		string str2;
		str1 << num[i] << "," << den[i];
		string s1 = str1.str();
		if(hashmap->count(s1) > 1) {
			// equal_range based on num[i]+","den[i]
			str2 = "";
			auto range = hashmap->equal_range(s1);
			for(auto it = range.first; it != range.second; ++it) {
				for(auto it1 = it ; it1 != range.second; ++it1) {
					if(it->second != it1->second) {
						str2.append(boost::lexical_cast<string>(the_num[it->second]));
				                str2.append(" and ");
				                str2.append(boost::lexical_cast<string>(the_num[it1->second]));
				                str2.append(" are FRIENDLY\n");
				        }
				}
			}
			//*counts[str2]++;
			counts->insert(std::pair<string,long int>(str2, 1));
		}
	}
	return 0;
}
/* add factor task */
void add_task(boost::asio::io_service& ioservice, \
		std::vector<boost::shared_future<int> >& future, \
		long int ii, long int the_num, long int *num, \
	        long int *den, tbb::concurrent_unordered_multimap<string,long int> *hashmap) {	

	ptask _ptask = boost::make_shared<task>(boost::bind(&factor, \
				ii, the_num, num, den, hashmap));
	boost::shared_future<int> fut(_ptask->get_future());
	future.push_back(fut);
	ioservice.post(boost::bind(&task::operator(), _ptask));
}

void add_task_compute(boost::asio::io_service& ioservice, \
		std::vector<boost::shared_future<int> >& future, \
		long int *the_num, long int *num, long int *den, long int last, int core, int hardware_threads, \
		tbb::concurrent_unordered_multimap<string,long int> *hashmap, \
		tbb::concurrent_unordered_map<string, long int> *counts) {
	ptask _ptask = boost::make_shared<task>(boost::bind(&hashmap_compute, \
				    	the_num, num, den, last, core, hardware_threads, hashmap, counts));
	boost::shared_future<int> fut(_ptask->get_future());
	future.push_back(fut);
	ioservice.post(boost::bind(&task::operator(), _ptask));
}




void friendly_numbers(long int start, long int end, \
			boost::asio::io_service& ioservice, \
			std::vector<boost::shared_future<int> >& future) {
	long int last = end - start + 1;

	long int *the_num;
	the_num = (long int*) malloc(sizeof(long int) * last);
	long int *num;
	num = (long int*) malloc(sizeof(long int) * last);
	long int *den;
	den = (long int*) malloc(sizeof(long int) * last);



	long int i, j, ii, n;
	tbb::concurrent_unordered_multimap<string,long int> hashmap;
	
	// calculate num and den, gcd and insert those entries into hashmap	
	for (i = start; i <= end; i++) {
		ii = i - start;
		the_num[ii] = i;
		add_task(ioservice, future, ii, the_num[ii], &num[ii], &den[ii],
				&hashmap);

	}
	boost::wait_for_all(future.begin(), future.end());

	tbb::concurrent_unordered_map<string, long int> counts;
	// go through the entire hashmap and check if we a multiple hash entry, if it was linear search , then we would
	// have needed O(n^2)
	int hardware_threads = boost::thread::hardware_concurrency();	
	for(int core = 0; core < hardware_threads; core++) {
		add_task_compute(ioservice, future, the_num, num, den, \
			last, core, hardware_threads, &hashmap, &counts);
	}

	boost::wait_for_all(future.begin(), future.end());

	// we have added all the friendly nos into this unordered_map in hash_compute() so now its time to print
	for(auto count : counts) {
		cout << count.first;
	} 
	

	free(the_num);
	free(num);
	free(den);
}

int main(int argc, char **argv) {
	long int start;
	long int end;

	boost::asio::io_service ioservice;
	boost::thread_group threads;
	boost::asio::io_service::work work(ioservice);
	int hardware_threads = boost::thread::hardware_concurrency();
	for(int i = 0; i < hardware_threads; i++) {
		threads.create_thread(boost::bind(&boost::asio::io_service::run,
					&ioservice));
	}

	std::vector<boost::shared_future<int> > future;

	while (1) {
		scanf("%ld %ld", &start, &end);
		if (start == 0 && end == 0)
			break;
		printf("Number %ld to %ld\n", start, end);
		friendly_numbers(start, end, ioservice, future);
	}

	return EXIT_SUCCESS;
}
