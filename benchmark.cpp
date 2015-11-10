#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cache.h"

using namespace std;
using namespace chrono;

typedef	unordered_map<int,int>		db_map;
typedef	unordered_map<int,string>	dir_map;
typedef	pair<double, unsigned>		c_res;

double	run_experiment_dir		(int* indices, dir_map& mapper, int file_size);
double	run_experiment_db		(int* indices, db_map& mapper, int file_size,
								string& db_loc);
c_res	run_experiment_cache	(int* ordering, db_map& mapper, int file_size,
								string& db_loc, unsigned line_size,
								unsigned lines_stored);

template<typename T>
double	get_average				(T* times, int num_trials);
template<typename T>
double	get_stddev				(T* times, int num_trials, double average);

int*	get_indices				(const char* index_path);
db_map	mk_db_map				(int* id_list, int file_size);
dir_map	mk_dir_map				(int* id_list, int hash_mod,
									const string& base_dir);


enum op_mode { OP_CONT, OP_RAND, OP_TRACE };
enum rd_mode { RD_DIR, RD_DB };

int main(int argc, char** argv)
{
	/// Start argument parsing

	if ( argc < 7 || argc > 11 )
	{
		cerr << "Usage is: " << argv[0] << "<read mode> <operating mode> "
			<< "<num trials> <id list> <file size> "
			<< "<base dir || db file> [hash modulus] [trace file] "
			<< "[<cache> [<line length> <num lines>]]" << endl;

		return 1;
	}

	// Read necessary arguments
	rd_mode read;	// Read mode

	// Determine read mode
	switch (atoi(argv[1]))
	{
		case 0:		read = RD_DIR;		break;
		case 1:		read = RD_DB;		break;
		default:
			cerr << "Input correct read mode (0, or 1)" << endl;
			return 1;
	}

	op_mode	mode;	// Operation mode

	// Determine operation mode
	switch (atoi(argv[2]))
	{
		case 0:		mode = OP_CONT;		break;
		case 1:		mode = OP_RAND;		break;
		case 2:		mode = OP_TRACE;	break;
		default:
			cerr << "Input correct operating mode (0, 1, or 2)" << endl;
			return 1;
	}

	int		num_trials	= atoi(argv[3]);	// Number of trials
	string	id_file		= string(argv[4]);	// List of all entry ids
	int		file_size	= atoi(argv[5]);	// Length of all files/entries

	// Read potential arguments
	int			at_arg(6);
	
	int			hash_mod;	// Modulus for directory hash
	string		base_dir;	// Directory where RD_DIR files are stored
	string		db_loc;		// Location of the RD_DB file
	string		trace_path;	// Location of the trace for OP_TRACE
	
	bool		use_cache;	// Whether or not we're using cache
	unsigned	line_len;	// Length of a cache line
	unsigned	num_lines;	// Number of lines to store

	if ( read == RD_DIR )
	{
		base_dir = string(argv[at_arg++]);
		hash_mod = atoi(argv[at_arg++]);

		if ( mode == OP_TRACE )
			trace_path = string(argv[at_arg++]);
	}

	else if ( read == RD_DB )
	{
		db_loc	= string(argv[at_arg++]);

		if ( mode == OP_TRACE )
			trace_path = string(argv[at_arg++]);

		// Use the cache?
		use_cache = atoi(argv[at_arg++]);

		if (use_cache) 
		{
			line_len	= atoi(argv[at_arg++]);
			num_lines	= atoi(argv[at_arg++]);
		}
	}

	/// End argument parsing

	// Store the ids in memory
	int*	ids(get_indices(id_file.c_str()));

	// Map the ids to their locations on disk
	db_map	db_mapper;
	dir_map	dir_mapper;

	if ( read == RD_DB )
		db_mapper	= mk_db_map(ids, file_size);
	else
		dir_mapper	= mk_dir_map(ids, hash_mod, base_dir);

	// Generate the necessary indices
	int*	indices;

	if ( mode != OP_TRACE )
	{
		indices	= ids;
		ids		= nullptr;
	}

	else
	{
		indices	= get_indices(trace_path.c_str());
		delete[] ids;
	}

	// We need to calculate this if we're doing random read or using cache
	int num_indices(0);

	if ( mode == OP_RAND || use_cache )
	{
		int* ct = indices;
		while (*ct != -1)
		{
			num_indices++;
			ct++;
		}
	}

	// Run the experiment
	double		times[num_trials];
	double		misses[num_trials];

	for ( int i = 0 ; i < num_trials ; i++)
	{
		// If we're random, shuffle
		if ( mode == OP_RAND )
		{
			unsigned	seed = chrono::system_clock::now().time_since_epoch().count();
			shuffle(indices, indices + num_indices, mt19937_64(seed));
		}

		// Perform respective function
		if ( read == RD_DIR )
			times[i]	= run_experiment_dir(indices, dir_mapper, file_size);

		else if ( read == RD_DB )
		{
			if (use_cache)
			{
				c_res res(run_experiment_cache(indices, db_mapper, file_size,
							db_loc, line_len, num_lines));
				times[i]	= res.first;
				misses[i]	= (double)res.second/num_indices;
			}
			else
				times[i]	= run_experiment_db(indices, db_mapper, file_size,
												db_loc);
		}
	}

	delete indices;

	// Do stats
	double av(get_average<double>(times, num_trials));
	double sd(get_stddev<double>(times, num_trials, av));

	cout << av << "\t" << sd;
	
	if (use_cache)
	{
		av	= get_average<double>(misses, num_trials);
		sd	= get_stddev<double>(misses, num_trials, av);
		
		cout << "\t" << av << "\t" << sd;
	}
	
	cout << endl;

	return 0;
}

int* get_indices(const char* index_path)
{
	ifstream	index(index_path);
	vector<int>	index_list;
	int			cur_index;
	int*		indices;

	// Read all things in file
	while (!index.eof())
	{
		index >> cur_index;
		index_list.push_back(cur_index);
	}

	// Turn it into an array terminated by -1
	indices	= new int[index_list.size()];

	for ( unsigned i = 0 ; i < index_list.size() ; i++ )
		indices[i] = index_list[i];

	indices[index_list.size()-1] = -1;

	return indices;
}

db_map mk_db_map(int* id_list, int file_size)
{
	db_map mapper;

	int count = 0;

	for (int* cur_id = id_list ; *cur_id != -1 ; cur_id++)
	{
		// Generate the database offset of cur_id
		mapper[*cur_id] = count * file_size;
		count++;
	}

	return mapper;
}

dir_map mk_dir_map(int* id_list, int hash_mod, const string& base_dir)
{
	dir_map			mapper;
	stringstream	strstream;

	int		mod;
	int		one;
	int		ten;
	int		hun;
	string	location;

	for (int* cur_id = id_list ; *cur_id != -1 ; cur_id++)
	{
		// Generate the file path of cur_id
		mod	= *cur_id % hash_mod;
		one	= mod % 10;
		ten	= (mod / 10) % 10;
		hun	= (mod / 100) % 10;

		strstream << base_dir << '/' << hun << '/' << ten << '/' << one << '/'
			<< *cur_id << ".txt" << endl;
		strstream >> location;

		mapper[*cur_id]	= location;
	}

	return mapper;
}

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
		input_file.read(read_here, file_size);
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

	// Read stuff
	for ( int* i = ordering ; *i != -1 ; i++ )
	{
		input_file.seekg(mapper[*i], input_file.beg);

		input_file.read(read_here, file_size);
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
	cache db(mapper, db_loc, file_size, line_size, lines_stored);

	ts	= system_clock::now();

	for (int* i = ordering ; *i != -1 ; i++)
		db[*i];

	tf	= system_clock::now();

	unsigned	misses(db.get_num_fetches());
	double		time((duration_cast< duration<double> >(tf - ts)).count()*1000.0);

	return c_res(time, misses);
}

template <typename T>
double get_average(T* set, int num_trials)
{
	double sum(0.0);

	for (int i = 0 ; i < num_trials ; i++)
		sum += set[i];

	return sum / num_trials;
}

template <typename T>
double get_stddev(T* set, int num_trials, double average)
{
	double stddev(0.0);

	for (int i = 0 ; i < num_trials ; i++ )
		stddev += (average - set[i])*(average - set[i]);

	stddev /= num_trials;

	return sqrt(stddev);
}
