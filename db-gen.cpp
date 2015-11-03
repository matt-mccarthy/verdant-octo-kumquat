#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

using namespace std;

list<unsigned>	get_ids	(const string& trace_loc);
void			out_ids	(const list<unsigned>& id_list, const string& id_out);
void			out_dir	(const list<unsigned>& id_list, const string& dir_out, int hash_mod, int file_size);
void			out_db	(const string& db_out, int file_size, int num_files);

int main(int argc, char** argv)
{
	if (argc != 7)
	{
		cerr << "Usage is: " << argv[0] << "<trace> <hash mod> <file size> "
			<< "<id list out> <database out> <output file directory>" << endl;
	}

    string	trace_loc(argv[1]);
    int     hash_mod(atoi(argv[2]));
    int		file_size(atoi(argv[3]));
    string	id_out(argv[4]);
    string	db_out(argv[5]);
    string	dir_out(argv[6]);

	list<unsigned> id_list(get_ids(trace_loc));
	out_ids(id_list, id_out);

	out_dir(id_list, dir_out, hash_mod, file_size);
	out_db(db_out, file_size, id_list.size());

	return 0;
}

list<unsigned> get_ids(const string& trace_loc)
{
	ifstream		trace(trace_loc.c_str());
	list<unsigned>	id_list;
	unsigned		cur_id;

	while (!trace.eof())
	{
        trace >> cur_id;
        id_list.push_back(cur_id);
	}

	id_list.unique();
	id_list.sort();

	return id_list;
}

void out_ids(const list<unsigned>& id_list, const string& id_out)
{
	ofstream	out_here(id_out.c_str());

	for (unsigned i : id_list)
		out_here << i << endl;
}

void out_dir(const list<unsigned>& id_list, const string& dir_out, int hash_mod, int file_size)
{
    ofstream		out_here;
    stringstream	str_stream;
    string			file_loc;
	char			out_me[file_size];

	fill_n(out_me, file_size, 0);

	for (unsigned i : id_list)
	{
		unsigned i_mod	= i % hash_mod;
		unsigned ones	= i_mod % 10;
		unsigned tens	= (i_mod / 10) % 10;
		unsigned huns	= (i_mod / 100) % 10;

		str_stream << dir_out << '/' << huns << '/' << tens << '/' << ones << '/'
			<< i << ".txt" << endl;
		str_stream >> file_loc;

		out_here.open(file_loc.c_str());
		out_here.write(out_me, file_size);
		out_here.close();
	}
}

void out_db (const string& db_out, int file_size, int num_files)
{
	ofstream	out_here(db_out);
	char		out_me[file_size];

	fill_n(out_me, file_size, 0);

	for (int i = 0 ; i < num_files ; i++)
		out_here.write(out_me, file_size);
}
