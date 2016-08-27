#include <stdlib.h>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <string>
#include "bucketsort.h"
#include <omp.h>
#include <vector>
#include <parallel/algorithm>
#include <fstream>
#define N_BUCKETS 94

using namespace std;
namespace asio = boost::asio; 


typedef boost::packaged_task<int> task;
typedef boost::shared_ptr<task> ptask;

int sort_string(vector<string>* str_vec) {
	//__gnu_parallel::sort(str_vec->begin(), str_vec->end());
	std::sort(str_vec->begin(), str_vec->end());
	//for(auto& str_t:str_vec)
	//	cout << str_t << endl;
	return 0;
}

// Add task to the queue, thread takes task from the queue.
void add_task(boost::asio::io_service& ioservice, \
		std::vector<boost::shared_future<int> >& future, \
		vector<string> *str_vec) {
	ptask _ptask = boost::make_shared<task>(boost::bind(&sort_string, str_vec));
	boost::shared_future<int> fut(_ptask->get_future());
	future.push_back(fut);
	ioservice.post(boost::bind(&task::operator(), _ptask));
}

void bucket_sort(char *a, int length, long int size, char *out) {
	
	ofstream fout;
	long int i;
	char temp[length+1];
	int bucketno;
	vector<string> str_vec[N_BUCKETS];
	
	//create pool of threads. one thread per core
	boost::asio::io_service ioservice;
	boost::thread_group threads;
	boost::asio::io_service::work work(ioservice);
	int hardware_threads = boost::thread::hardware_concurrency(); 
	for(int i = 0; i < hardware_threads; i++) {
		threads.create_thread(boost::bind(&boost::asio::io_service::run,
					&ioservice));
	}
	

	
	for (i = 0; i < size; i++) {
		bucketno = *(a + i * length) - 0x21;
		memset(temp, 0, strlen(temp));
		memcpy(temp, a+ i*length, length);
		temp[length] = '\0';
		str_vec[bucketno].push_back(temp);
	}

	std::cout << "Done copying the keys" << std::endl;

	std::vector<boost::shared_future<int> > future;
	double st = omp_get_wtime();	

	for (i = 0; i < N_BUCKETS; i++) {
		add_task(ioservice, future, &str_vec[i]);
	}
	boost::wait_for_all(future.begin(), future.end());

	double ed = omp_get_wtime();
	printf("time to sort %.4lfs\n", ed - st);

	fflush(stdout);	
	fout.open(out);
	for(auto &str_vec_arr: str_vec) {
		for(auto& str_bucket: str_vec_arr) {
			//fprintf(fout, "%s\n", str_bucket);
			fout << str_bucket << endl;
		}
	}
	fout.close();

}
