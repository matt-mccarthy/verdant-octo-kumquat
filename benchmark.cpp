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
#include <vector>

using namespace std;
using namespace chrono;

typedef unordered_map<int,int> db_map;
typedef unordered_map<int,string> dir_map;

double	run_experiment_dir	(int* indices, int hash_mod, int file_size, string base_dir);
double	run_experiment_db	(int* indices, db_map& mapper, int file_size, string db_loc);

double	get_average			(double* times, int num_trials);
double	get_stddev			(double* times, int num_trials, double average);

int*	get_indices			(const char* index_path);
db_map	mk_db_map			(int* id_list, int file_size);
dir_map	mk_dir_map			(int* id_list, int hash_mod);


enum op_mode { OP_CONT, OP_RAND, OP_TRACE };
enum rd_mode { RD_DIR, RD_DB };

int main(int argc, char** argv)
{
	if ( argc < 7 || argc > 9 )
	{
		cerr << "Usage is: " << argv[0] << "<read mode> <operating mode> "
			<< "<num trials> <id list> <file size> "
			<< "<base dir || db file> [hash modulus] [trace file]" << endl;

		return 1;
	}
	
	rd_mode read;
	
	switch (atoi(argv[1]))
	{
		case 0:		read = RD_DIR;		break;
		case 1:		read = RD_DB;		break;
		default:
			cerr << "Input correct read mode (0, or 1)" << endl;
			return 1;
	}

	op_mode	mode;

	switch (atoi(argv[2]))
	{
		case 0:		mode = OP_CONT;		break;
		case 1:		mode = OP_RAND;		break;
		case 2:		mode = OP_TRACE;	break;
		default:
			cerr << "Input correct operating mode (0, 1, or 2)" << endl;
			return 1;
	}
	
	int		num_trials	= atoi(argv[3]);
	string	id_file		= string(argv[4]);
	int		file_size	= atoi(argv[5]);

	int		hash_mod;
	string	base_dir;
	string	db_loc;
	string	trace_path;
	
	if ( read == RD_DIR )
	{
		base_dir = string(argv[6]);
		hash_mod = atoi(argv[7]);
		
		if ( mode == OP_TRACE )
			trace_path = string(argv[8]);
	}
	
	else if ( read == RD_DB )
	{
		db_loc	= string(argv[6]);
		
		if ( mode == OP_TRACE )
			trace_path = string(argv[7]);
	}
	
	int*	ids(get_indices(id_file.c_str());
	
	int*	indices;
	
	if ( mode != OP_TRACE )
		indices	= get_indices(id_file.c_str());
		
	else
		indices	= get_indices(trace_path.c_str());
	
	db_map	mapper;
	
	if ( read == RD_DB )
	{
		mapper	= mk_db_map(ids, file_size);
	}

	double	times[num_trials];

	for ( int i = 0 ; i < num_trials ; i++)
	{
		if ( mode == OP_RAND )
		{
			unsigned	seed = chrono::system_clock::now().time_since_epoch().count();
			shuffle(indices, indices + (last - first), mt19937_64(seed));
		}
		
		if ( read == RD_DIR )
			times[i]	= run_experiment_dir(indices, hash_mod, file_size, base_dir); 
		
		else if ( read == RD_DB )
			times[i]	= run_experiment_db(indices, file_size, db_loc);
	}

	delete indices;

	double av(get_average(times, num_trials));
	double sd(get_stddev(times, num_trials, av));

	cout << av << "\t" << sd << endl;

	return 0;
}

int* get_indices(const char* index_path)
{
	ifstream	index(index_path);
	vector<int>	index_list;
	int			cur_index;
	int*		indices;

	while (trace.good())
	{
		index >> cur_index;
		index_list.push_back(cur_index - offset);
	}

	indices	= new int[index_list.size() + 1];

	for ( unsigned i = 0 ; i < index_list.size() ; i++ )
		indices[i] = index_list[i];

	indices[index_list.size()] = -1;

	return indices;
}

db_map mk_db_map(int* id_list, int file_size)
{
	db_map mapper;
	
	int* cur_id = id_list;
	int count = 0;
	
	while (*cur_id != -1)
	{
		mapper[*cur_id] = count * file_size;
		count++;
	}
	
	return mapper;
}

double run_experiment_dir(int* ordering, int hash_mod, int file_size, string base_dir)
{
	high_resolution_clock::time_point ts, tf;
	ifstream		input_file;
	string			file_loc;
	int 			mod;
	int				one;
	int				ten;
	int				hun;
	char*			read_here = new char[file_size];
	stringstream	strstream;

	ts	= system_clock::now();

	// Read stuff
	for ( int* i = ordering ; *i != -1 ; i++ )
	{
		mod	= *i % hash_mod;
		one	= mod % 10;
		ten	= (mod / 10) % 10;
		hun	= (mod / 100) % 10;

		strstream << base_dir << '/' << hun << '/' << ten << '/' << one << '/'
			<< *i << ".txt" << endl;
		strstream >> file_loc;

		input_file.open(file_loc.c_str());

		input_file.read(read_here, file_size);

		input_file.close();
	}

	tf	= system_clock::now();

	delete[] read_here;

	return (duration_cast< duration<double> >(tf - ts)).count()*1000.0;
}

double run_experiment_db(int* ordering, db_map& mapper, int file_size, string db_loc)
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

double get_average(double* times, int num_trials)
{
	double sum(0.0);

	for (int i = 0 ; i < num_trials ; i++)
		sum += times[i];

	return sum / num_trials;
}

double get_stddev(double* times, int num_trials, double average)
{
	double stddev(0.0);

	for (int i = 0 ; i < num_trials ; i++ )
		stddev += (times[i] - average)*(times[i] - average);

	stddev /= num_trials;

	return sqrt(stddev);
}
