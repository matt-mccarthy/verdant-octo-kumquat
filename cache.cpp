#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "cache.h"

using std::atomic;
using std::atomic_bool;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::system_clock;
using std::condition_variable;
using std::mutex;
using std::priority_queue;
using std::string;
using std::thread;
using std::unique_lock;
using std::unordered_map;

typedef unordered_map<int, int>	db_map;
typedef unique_lock<mutex> slock;

cache::cache(db_map& mapper, string& db_location, int* id_list,
				unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines, unsigned num_threads)
			: db_file(db_location),
				entry_size(entry_length), line_length(entries_per_line),
				line_count(num_lines), thread_count(num_threads),
				cache_size(line_count*line_length),
				stop(false), fetch_count(0),
				reader{read, this}
{
    for (int* i = id_list ; *i != -1 ; i++)
    	cache_map[*i] = entry(*i);
}

cache::~cache()
{
	stop = true;
	reader.join();
	db_file.close();
}

cache::entry::entry(int offset) : db_offset(offset), memory(nullptr)
{
	accessed = system_clock::to_time_t(system_clock::now());
}

cache::entry::~entry()
{
	if (memory)
		delete[] memory;
}

cache::entry::operator=(entry&& in)
{
	db_offset	= in.db_offset;
	memory		= in.memory.load(std::memory_order_seq_cst);
	accessed	= in.accessed;
}

bool cache::query::operator()(query& a, query& b)
{
	return (a.immediate && !b.immediate);
}

unsigned cache::get_num_fetches()
{
	return fetch_count;
}

char* cache::operator[](int entry_id)
{
	entry* cur_entry = &cache_map[entry_id];

	cur_entry->lock.lock();
	cur_entry->accessed = system_clock::to_time_t(system_clock::now());

	if (cur_entry->memory == nullptr)
	{
		cur_entry->lock.unlock();

		queue_lock.lock();
        read_queue.push(query(entry_id, true));

        sleep.notify_all();

        if (line_length > 1)
		{
			auto itr = next(cache_map.find(entry_id));
			for (int i = 1 ; i < line_length ; i++)
			{
				read_queue.push(query(itr->first, false));
				itr++;
			}
			sleep.notify_all();
		}
		queue_lock.unlock();

		cur_entry->lock.lock();
	}

	cur_entry->lock.unlock();

	return cur_entry->memory;
}

void cache::read()
{
	slock lock(sleep_lock);

	while (!stop)
	{
		while (!read_queue.empty())
		{
			// Get next thing off of the queue
			queue_lock.lock();

			int id(read_queue.top().id);
			read_queue.pop();

			queue_lock.unlock();

			if (cache_map[id].memory == nullptr)
			{
				cache_map[id].lock.lock();

				// If the cache is full
				if (cache_map.size() == cache_size)
				{
					// Remove least recently used
					// TODO: Garbage collection
				}

				// Fetch an entry from disk
				char* new_block = new char[entry_size];
				cache_map[id].memory = new_block;
				fetch(cache_map[id].db_offset, new_block);

				cache_map[id].lock.unlock();
			}
		}

		sleep.wait(lock);
	}
}

void cache::fetch(int offset, char* put_here)
{
	db_file.seekg(offset, db_file.beg);
	db_file.read(put_here, entry_size);
}
