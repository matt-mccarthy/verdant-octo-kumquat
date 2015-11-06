#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "cache.h"

using std::fill_n;
using std::chrono::duration_cast;
using std::chrono::system_clock;
using std::condition_variable;
using std::mutex;
using std::queue;
using std::thread;
using std::unordered_map;

typedef unordered_map<int><int>	db_map;
typedef unordered_map<int><cache::entry> c_map;

cache::cache(db_map& mapper, string& db_location, unsigned entry_length,
				unsigned entries_per_line, unsigned num_lines,
				unsigned num_threads)
			: db_mapper(mapper), db_loc(db_location), db_file(db_loc),
				entry_size(entry_length), line_length(entries_per_line),
				line_count(num_lines), thread_count(num_threads),
				cache_size(line_count*line_length),
				stop(false), reader(read), fetch_count(0) {}

cache::~cache()
{
	stop = true;
	reader.join();
	db_file.close();

	for (int i = 0 ; i < cache_size ; i++)
		if (!cached_entries[i])
			delete cached_entries[i];

	delete cached_entries;
}

cache::entry::entry() : location(new char[entry_size]),
{
	accessed = duration_cast<long>(system_clock::now());
}

cache::entry::~entry() {}

char* cache::entry::access()
{
	accessed = duration_cast<long>(system_clock::now());
	return location;
}

void cache::read()
{
	bool flag = !stop;
	
	while (flag)
	{
		while (!read_queue.empty())
		{
			// Get next thing off of the queue
			queue_lock.lock();
			
			int id(read_queue.front());
			read_queue.pop();
			
			queue_lock.unlock();
			
			// Fetch an entry from disk
			entry new_entry;
			fetch(id, new_entry.location);
			
			// Write entry to cache map
			cache_lock.lock();
			
			if (cache_map.size() == cache_size())
			{
				// Remove least recently used
				auto lru(cache_map.begin());
				for (auto i = cache_map.begin() ; i != cache_map.end() ; i++)
					if ((*lru).accessed < (*i).accessed)
						lru = i;
						
				cache_map.remove(lru);
			}
			
			// Entry
			cache_map[id] = new_entry;
			
			cache_lock.unlock();
		}
	}
}

