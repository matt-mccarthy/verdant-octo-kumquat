#ifndef CACHE_SEQ_H
#define CACHE_SEQ_H

#include <ctime>
#include <fstream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "entry_seq.h"

class cache_seq
{
	public:
		cache_seq(std::unordered_map<int,int>& mapper, unsigned entry_length,
				unsigned entries_per_line, unsigned num_lines);
		~cache_seq();

		int get_num_fetches();

		char*	operator[](int entry_id);
		bool	open(std::string& db_loc);
		int		get_size();

	private:
		void add_to_db(int id);
		void add_to_queue(int id);
		void fetch_from_disk(int offset, char* put_here);
		void garbage_collect();

		std::unordered_map<int, entry_seq>	cache_map;		// Maps index->entry
		std::ifstream						db_file;		// The database
		const unsigned						entry_size;		// Size of each entry
		const unsigned						line_length;	// Entries per line
		const unsigned						line_count;		// Num lines stored
		const unsigned						cache_size;		// Max entries in cache
		int									entry_count;	// Num entries in cache
		int									fetch_count;	// Number of fetches
};

#endif
