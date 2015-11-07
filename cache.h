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

class cache
{
	public:
		cache(std::unordered_map<int,int>& mapper, std::string& db_location,
				int* id_list, unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines, unsigned num_threads);
		~cache();

		unsigned get_num_fetches();

		char* operator[](int entry_id);

	private:
		class entry
		{
			public:
				entry() {}
				entry(int offset);
				operator=(entry&& in);
				~entry();

				int					db_offset;
				std::atomic<char*>	memory;
				std::time_t			accessed;
				std::mutex			lock;
		};

		struct query
		{
			public:
				query() {}
				query(int id, bool now)
				{
					this -> id	= id;
					immediate	= now;
				}
                int		id;
                bool	immediate;
                bool	operator()(query& a, query& b);
		};

		void read();
		void fetch(int offset, char* put_here);

		std::unordered_map<int, entry>	cache_map;		// Maps index->entry
		std::ifstream					db_file;		// The database
		unsigned						entry_size;		// Size of each entry
		unsigned						line_length;	// Entries per line
		unsigned						line_count;		// Num lines stored
		unsigned						thread_count;	// Num threads
		unsigned						cache_size;		// Num entries in cache
		unsigned						fetch_count;	// Number of fetches

		std::priority_queue<query, std::vector<query>, query>	read_queue;	// Things to read later

		std::mutex				queue_lock;	// Queue lock
		std::mutex				cache_lock;	// Cache lock
		std::thread				reader;		// Reading thread
		std::condition_variable	sleep;		// Sleep variable
		std::mutex				sleep_lock;	// Sleep lock
		std::atomic_bool		stop;		// Stop flag
};
