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

#define	FILE_SIZE	14336

using namespace std;
using namespace chrono;

double run_experiment(int first, int last, int hash_mod, string base_dir, int* ordering);
double get_average(double* times, int num_trials);
double get_stddev(double* times, int num_trials, double average);

int main(int argc, char** argv)
{
	if ( argc != 6 )
		return 1;
	
	int		first		= atoi(argv[1]);
	int		last		= atoi(argv[2]);
	int		hash_mod	= atoi(argv[3]);
	int		num_trials	= atoi(argv[4]);
	string	base_dir(argv[5]);
	
	int		indices[last-first];
	
	for ( int i = 0 ; i < last - first ; i++ )
		indices[i] = i + first;
		
	unsigned seed = chrono::system_clock::now().time_since_epoch().count();
		
	shuffle(indices, indices + (last - first), mt19937_64(seed));
	
	double	times[num_trials];
	
	for ( int i = 0 ; i < num_trials ; i++)
		times[i]	= run_experiment(first, last, hash_mod, base_dir, indices);
		
	double av(get_average(times, num_trials));
	double sd(get_stddev(times, num_trials, av));
	
	cout << av << "\t" << sd << endl;
}

double run_experiment(int first, int last, int hash_mod, string base_dir, int* ordering)
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
	for ( int i = 0 ; i < last - first ; i++ )
	{	
		mod	= ordering[i] % hash_mod;
		one	= mod % 10;
		ten	= (mod / 10) % 10;
		hun	= (mod / 100) % 10;

		strstream << base_dir << '/' << hun << '/' << ten << '/' << one << '/' << ordering[i] << ".txt" << endl;
		strstream >> file_loc;
		
		input_file.open(file_loc.c_str());
		
		input_file.read(read_here, FILE_SIZE);
		
		input_file.close();
	}
	
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
