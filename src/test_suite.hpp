#ifndef TEST_SUITE_CPP
#define TEST_SUITE_CPP

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>

#include "cache.h"
#include "cache_seq.h"

using namespace std;
using namespace chrono;

using boost::iostreams::mapped_file_source;

typedef	unordered_map<int,int>							db_map;
typedef	unordered_map<int,string>						dir_map;
typedef	pair<double, unsigned>							c_res;
typedef	boost::iostreams::stream<mapped_file_source>	mem_str;

namespace tests
{

double run_experiment_dir(int* ordering, dir_map& mapper, int file_size)
{
	// Set up
	high_resolution_clock::time_point ts, tf;
	ifstream		input_file;
	char*			read_here = new char[file_size];

	ts	= system_clock::now();

	// Read stuff
	for ( int* i = ordering ; *i != -1 ; i++ )
	{
		input_file.open(mapper[*i]);
		#ifdef DEBUG
		if (!input_file.good())
		{
			cerr << "Bad dir file" << endl;
			exit(1);
		}
		#endif
		input_file.read(read_here, file_size);
		#ifdef DEBUG
		if (!input_file)
		{
			cerr << "Bad read (dir)" << endl;
			exit(1);
		}
		for (int i = 0 ; i < file_size ; i++)
			if (read_here[i] != 0)
			{
				cerr << "Incorrect read (dir)" << endl;
				exit(1);
			}
		#endif
		input_file.close();
	}

	tf	= system_clock::now();

	// Clean up
	delete[] read_here;

	return (duration_cast< duration<double> >(tf - ts)).count()*1000.0;
}

double run_experiment_db(int* ordering, db_map& mapper, int file_size, string& db_loc)
{
	high_resolution_clock::time_point ts, tf;
	ifstream		input_file;
	char*			read_here = new char[file_size];

	ts	= system_clock::now();

	input_file.open(db_loc.c_str());
	#ifdef DEBUG
	if (!input_file.good())
	{
		cerr << "Bad db file" << endl;
		exit(1);
	}
	#endif

	// Read stuff
	for ( int* i = ordering ; *i != -1 ; i++ )
	{
		input_file.seekg(mapper[*i], input_file.beg);
		#ifdef DEBUG
		if (!input_file)
		{
			cerr << "Bad DB seek" << endl;
			exit(1);
		}
		#endif
		input_file.read(read_here, file_size);
		#ifdef DEBUG
		if (!input_file)
		{
			cerr << "Bad read (db)" << endl;
			exit(1);
		}
		for (int i = 0 ; i < file_size ; i++)
			if (read_here[i] != 0)
			{
				cerr << "Incorrect read (db)" << endl;
				exit(1);
			}
		#endif
	}

	input_file.close();

	tf	= system_clock::now();

	delete[] read_here;

	return (duration_cast< duration<double> >(tf - ts)).count()*1000.0;
}

c_res run_experiment_cache(int* ordering, db_map& mapper, int file_size,
							string& db_loc, unsigned line_size,
							unsigned lines_stored)
{
	high_resolution_clock::time_point ts, tf;
	cache_seq db(mapper, file_size, line_size, lines_stored);
	int req_ctr(0);
	int prev_fetch(0);

	ts	= system_clock::now();

	#ifdef DEBUG
	if (!db.open(db_loc))
	{
		cerr << "Bad db open (cache)" << endl;
		exit(1);
	}
	#else
	db.open(db_loc);
	#endif
	
	int cnt(0);

	for (int* i = ordering ; *i != -1 ; i++)
	{
		cout << "I " << *i << endl;
		#ifdef DEBUG
		char* ptr = db[*i];
		if (ptr == nullptr)
		{
			cerr << "Cache returned null" << endl;
			exit(1);
		}
		for (int i = 0 ; i < file_size ; i++)
			if (ptr[i] != 0)
			{
				cerr << "Incorrect read (cache)" << endl;
				exit(1);
			}
		#else
		db[*i];
		#endif
		if (++req_ctr % 5000 == 0)
		{
			int cur_fetch(db.get_num_fetches());
			cout << (double)(cur_fetch - prev_fetch)/5000.0 << endl;
			prev_fetch = cur_fetch;
		}
		cnt = (cnt < db.get_size()-15000) ? db.get_size()-15000 : cnt;
		cout << "O " << *i << endl;
	}

	tf	= system_clock::now();

	//db.clear();

	unsigned	misses(db.get_num_fetches());
	double		time((duration_cast< duration<double> >(tf - ts)).count()*1000.0);
	
	cout << "Max overage: " << cnt << endl;

	return c_res(time, misses);
}

double run_experiment_ram(int* ordering, db_map& mapper, int file_size, int num_ids, string& db_loc)
{
	high_resolution_clock::time_point ts, tf;
	
	char* read_here = new char[file_size];

	ts	= system_clock::now();

	mapped_file_source db(db_loc.c_str());
	mem_str db_wrap(db, std::ios::in);

	#ifdef DEBUG
	if (!db.is_open())
	{
		cerr << "Could not open (mem)" << endl;
		exit(1);
	}
	if (!db_wrap.good())
	{
		cerr << "Could not wrap (mem)" << endl;
		exit(1);
	}
	#endif

	//int		f_id(open(db_loc.c_str(), O_RDONLY));
	//char*	m_id((char*)mmap(0, num_ids, PROT_READ, MAP_SHARED, f_id, 0));

	for (int* i = ordering ; *i != -1 ; i++)
		//memcpy(read_here, m_id + mapper[*i], file_size);
		//m_id[mapper[i]];
	{
		db_wrap.seekg(mapper[*i], db_wrap.beg);
		#ifdef DEBUG
		if (!db_wrap)
		{
			cerr << "Bad mem seek" << endl;
			exit(1);
		}
		#endif
		db_wrap.read(read_here, file_size);
		#ifdef DEBUG
		if (!db_wrap)
		{
			cerr << "Bad read (mem)" << endl;
			exit(1);
		}
		for (int i = 0 ; i < file_size ; i++)
			if (read_here[i] != 0)
			{
				cerr << "Incorrect read (mem)" << endl;
				exit(1);
			}
		#endif
	}

	tf	= system_clock::now();
	
	//munmap(m_id, num_ids);
	//close(f_id);
	
	db_wrap.close();
	db.close();
	
	delete read_here;

	return (duration_cast< duration<double> >(tf - ts)).count()*1000.0;
}
}

#endif

