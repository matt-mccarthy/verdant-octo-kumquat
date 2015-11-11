#ifndef CACHE_H
#define CACHE_H

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "entry.h"
#include "query.h"

class cache
{
	public:
		cache(std::unordered_map<int,int>& mapper, unsigned entry_length,
				unsigned entries_per_line, unsigned num_lines);
		~cache();

		int get_num_fetches();

		char*	operator[](int entry_id);
		void	clear();
		bool	open(std::string& db_loc);
		int		get_size();

	private:
		void read();
		void add_to_db(int id);
		void add_to_queue(int id);
		void fetch_from_disk(int offset, char* put_here);
		void garbage_collect();

		std::unordered_map<int, entry>	cache_map;		// Maps index->entry
		std::ifstream					db_file;		// The database
		const unsigned					entry_size;		// Size of each entry
		const unsigned					line_length;	// Entries per line
		const unsigned					line_count;		// Num lines stored
		const unsigned					cache_size;		// Max entries in cache
		std::atomic_int					entry_count;	// Num entries in cache
		std::atomic_int					fetch_count;	// Number of fetches

		// Things to read
		std::priority_queue<query, std::vector<query>, query>	read_queue;

		std::mutex				queue_lock;	// Queue lock
		std::thread				reader;		// Reading thread
		std::condition_variable	sleep;		// Sleep condition
		std::condition_variable	wait;		// Condition to make main block
		std::mutex				waiter;
		std::unique_lock<std::mutex>	wait_lock;
		std::atomic_bool		stop;		// Stop flag
};

#endif
