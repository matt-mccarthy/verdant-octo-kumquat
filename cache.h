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
				 unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines);
		~cache();

		int get_num_fetches();

		char* operator[](int entry_id);
		void clear();

	private:
		class entry
		{
			public:
				entry() {}
				entry(int offset);
				entry&	operator=(entry&& in);
				bool	operator<=(entry& in);
				void	del();
				bool	is_in_cache();
				~entry();

				int					db_offset;
				char*				memory;
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
				bool	operator()(const query& a, const query& b);
		};

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
		std::atomic_bool		stop;		// Stop flag
};
