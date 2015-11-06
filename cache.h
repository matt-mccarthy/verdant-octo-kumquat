#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

class cache
{
	public:
		cache(std::unordered_map<int,int>& mapper, std::string& db_location,
				unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines, unsigned num_threads);
		~cache();

		char* operator[](int entry_id);

	private:
		class entry
		{
			public:
				cached_entry();
				~cached_entry();
				char* access();
			
				char*	location;
				long	accessed;
		}
		
		void read();
		void fetch(int offset, char* put_here);

		std::unordered_map<int, int>	db_mapper;		// Maps index->offset
		std::unordered_map<int, entry>	cache_map;		// Maps index->cache
		std::string						db_loc;			// Location of database
		std::ifstream					db_file;		// The database
		unsigned						entry_size;		// Size of each entry
		unsigned						line_length;	// Entries per line
		unsigned						line_count;		// Num lines stored
		unsigned						thread_count;	// Num threads
		unsigned						cache_size;		// Num entries in cache
		unsigned						fetch_count;	// Number of fetches

		std::queue				read_queue;	// Things to read
		std::mutex				queue_lock;	// Queue lock
		std::mutex				cache_lock;	// Cache lock
		std::mutex				file_lock;	// File lock
		std::thread				reader;		// Reading thread
		std::condition_variable	sleep;		// Sleep variable
		bool					stop;		// Stop flag
};
