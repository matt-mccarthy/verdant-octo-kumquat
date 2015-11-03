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
#include <vector>

#define	FILE_SIZE	14336

using namespace std;
using namespace chrono;

double run_experiment_dir(int* indices, int hash_mod, string base_dir);
double run_experiment_db(int* indices, string db_loc);

double get_average(double* times, int num_trials);
double get_stddev(double* times, int num_trials, double average);

int*	get_cont_indices(int first, int last);
int*	get_trace_indices(const char* trace_path, int offset);

enum op_mode { OP_CONT, OP_RAND, OP_TRACE };
enum rd_mode { RD_DIR, RD_DB };

int main(int argc, char** argv)
{
	if ( argc < 7 || argc > 8 )
	{
		cerr << "Usage is: " << argv[0] << "<read mode> <operating mode> "
			<< " <num trials> <directory where files are stored> "
			<< "[<trace file> || <first index> <last index>] [<hash modulus> <db offset>]" << endl;

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
	
	int		first, last;
	int		hash_mod;
	int		db_offset;
	string	base_dir;
	string	trace_path;
	string	db_loc;

	if ( read == RD_DIR )
	{
		hash_mod	= atoi(argv[4]);
		base_dir	= string(argv[5]);
		
		if ( mode != OP_TRACE )
		{
			first	= atoi(argv[6]);
			last	= atoi(argv[7]);
		}
		
		else
		{
			trace_path	= string(argv[6]);
			db_offset	= 0;
		}
	}
	
	else if ( read == RD_DB )
	{
		db_loc	= string(argv[4]);
		
		if ( mode != OP_TRACE )
		{
			first	= atoi(argv[5]);
			last	= atoi(argv[6]);
		}
		
		else
		{
			trace_path	= string(argv[5]);
			db_offset	= atoi(argv[6]);
		}
	}

	int*	indices;
	
	if ( mode != OP_TRACE )
		indices	= get_cont_indices(first, last);
		
	else
		indices	= get_trace_indices(trace_path.c_str(), db_offset);

	double	times[num_trials];

	for ( int i = 0 ; i < num_trials ; i++)
	{
		if ( mode == OP_RAND )
		{
			unsigned	seed = chrono::system_clock::now().time_since_epoch().count();
			shuffle(indices, indices + (last - first), mt19937_64(seed));
		}
		
		if ( read == RD_DIR )
			times[i]	= run_experiment_dir(indices, hash_mod, base_dir); 
		
		else if ( read == RD_DB )
			times[i]	= run_experiment_db(indices, base_dir); 
	}

	delete indices;

	double av(get_average(times, num_trials));
	double sd(get_stddev(times, num_trials, av));

	cout << av << "\t" << sd << endl;

	return 0;
}

int* get_cont_indices(int first, int last)
{
	int* indices = new int[last - first + 1];

	for ( int i = 0 ; i < last - first ; i++ )
		indices[i] = first + i;

	indices[last - first] = -1;

	return indices;
}

int* get_rand_indices(int first, int last)
{
	int*		indices = get_cont_indices(first, last);
	unsigned	seed = chrono::system_clock::now().time_since_epoch().count();

	shuffle(indices, indices + (last - first), mt19937_64(seed));

	return indices;
}

int* get_trace_indices(const char* trace_path, int offset)
{
	ifstream	trace(trace_path);
	vector<int>	index_list;
	int			cur_index;
	int*		indices;

	while (trace.good())
	{
		trace >> cur_index;
		index_list.push_back(cur_index - offset);
	}

	indices	= new int[index_list.size() + 1];

	for ( unsigned i = 0 ; i < index_list.size() ; i++ )
		indices[i] = index_list[i];

	indices[index_list.size()] = -1;

	return indices;
}

double run_experiment_dir(int* ordering, int hash_mod, string base_dir)
{
	high_resolution_clock::time_point ts, tf;
	ifstream		input_file;
	string			file_loc;
	int 			mod;
	int				one;
	int				ten;
	int				hun;
	char*			read_here = new char[FILE_SIZE];
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

		input_file.read(read_here, FILE_SIZE);

		input_file.close();
	}

	tf	= system_clock::now();

	delete[] read_here;

	return (duration_cast< duration<double> >(tf - ts)).count()*1000.0;
}

double run_experiment_db(int* ordering, int first, string db_loc)
{
	high_resolution_clock::time_point ts, tf;
	ifstream		input_file;
	char*			read_here = new char[FILE_SIZE];

	ts	= system_clock::now();
	
	input_file.open(db_loc.c_str());

	// Read stuff
	for ( int* i = ordering ; *i != -1 ; i++ )
	{
		input_file.seekg((*i - first) * FILE_SIZE, input_file.beg); 
		
		input_file.read(read_here, FILE_SIZE);
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
