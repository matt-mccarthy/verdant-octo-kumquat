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
using std::queue;
using std::string;
using std::time_t;
using std::this_thread::yield;
using std::thread;
using std::unique_lock;
using std::unordered_map;

typedef unordered_map<int, int>	db_map;
typedef unique_lock<mutex> slock;

time_t get_time()
{
	return system_clock::to_time_t(system_clock::now());
}

cache::cache(db_map& mapper, string& db_location,
				unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines)
			: db_file(db_location), entry_count(0), stop(false), fetch_count(0),
				entry_size(entry_length), line_length(entries_per_line),
				line_count(num_lines), cache_size(line_count*line_length)
{
	// Initialize the cache map
	for (auto i = mapper.begin() ; i != mapper.end() ; i++)
		cache_map[i->first] = entry(i->second);

	reader = thread(&cache::read, this);
}

cache::~cache()
{
	// Make the reader thread stop
	stop = true;
	sleep.notify_all();
	reader.join();
	db_file.close();
}

cache::entry::entry(int offset) : db_offset(offset), memory(nullptr),
									accessed(get_time()), loaded(false) {}

cache::entry::~entry()
{
	if (memory)
		delete[] memory;
}

cache::entry& cache::entry::operator=(entry&& in)
{
	db_offset	= in.db_offset;
	memory		= in.memory;
	accessed	= in.accessed;
	loaded		= in.loaded;

	return *this;
}

bool cache::entry::operator<=(entry& in)
{
	// If either cannot be locked, return false
	if (lock.try_lock())
	{
		if (in.lock.try_lock())
		{
			// If both are in cache and lockable
			if(in.is_in_cache() && is_in_cache())
			{
				bool output = accessed <= in.accessed;
				in.lock.unlock();
				lock.unlock();

				return output;
			}
			in.lock.unlock();
		}

		lock.unlock();
	}

	return false;
}

void cache::entry::del()
{
	if (is_in_cache())
	{
		delete[] memory;
		memory = nullptr;
		loaded = false;
	}
}

bool cache::entry::is_in_cache()
{
	return loaded;
}

bool cache::query::operator()(const query& a, const query& b)
{
	return (a.immediate && !b.immediate);
}

int cache::get_num_fetches()
{
	return fetch_count;
}

char* cache::operator[](int entry_id)
{
	// Get a pointer to the right entry
	entry* cur_entry = &cache_map[entry_id];

	cur_entry->lock.lock();

	// If the entry is not in cache
	if (!(cur_entry->is_in_cache()))
	{
		cur_entry->lock.unlock();
		add_to_queue(entry_id);
		cur_entry->lock.lock();
	}

	else
		cur_entry->accessed = get_time();

	char* location(cur_entry->memory);
	cur_entry->lock.unlock();

	cur_entry = nullptr;

	return location;
}

void cache::read()
{
	mutex	l;
	slock	lock(l);
	bool	queue_empty(false);
	int id(-1);

	while (!stop)
	{
		while (!queue_empty)
		{
			id = -1;

			// Get next thing off of the queue
			queue_lock.lock();
			
			if (!read_queue.empty())
			{
				id = read_queue.top().id;
				read_queue.pop();
			}
			else
				queue_empty = true;

			queue_lock.unlock();

			// If we grabbed anything off of the queue, add it
			if (id != -1)
				add_to_db(id);
		}

		if (!stop)
		{
			sleep.wait(lock);
			queue_empty = false;
		}
	}
}

void cache::add_to_db(int id)
{
	// Get a pointer to the right entry
	entry* cur_entry = &cache_map[id];
	// Lock the id
	cur_entry -> lock.lock();

	// If the entry is not in cache
	if (!(cur_entry -> is_in_cache()))
	{
		// If the cache is full
		if (entry_count == cache_size)
			garbage_collect();

		// Fetch an entry from disk
		char* new_block		= new char[entry_size];
		cur_entry -> memory	= new_block;
		fetch_from_disk(cur_entry -> db_offset, new_block);
		cur_entry ->loaded	= true;

		entry_count++;
	}

	// If it is, give it a new access time
	else
		cur_entry ->accessed = get_time();

	cur_entry ->lock.unlock();
	cur_entry = nullptr;
}

void cache::add_to_queue(int id)
{
	fetch_count++;
	queue_lock.lock();

	// Enqueue the current id with immediate priority
	read_queue.push(query(id, true));
	sleep.notify_all();
	queue_lock.unlock();

	yield();

	// Enqueue the next few ids with normal priority
	queue_lock.lock();
	auto itr = next(cache_map.find(id));
	for (unsigned i = 1 ; i < line_length && itr != cache_map.end() ; i++)
	{
		if (!(itr->second.is_in_cache()))
			read_queue.push(query(itr->first, false));

		else
			itr->second.accessed = get_time();

		itr++;
	}
	sleep.notify_all();
	queue_lock.unlock();
}

void cache::garbage_collect()
{
	// Find the first entry in cache
	queue<entry*> oldest;
	entry* dummy = new entry(0);
	dummy->lock.lock();
	
	oldest.push(dummy);

	// Populate the queue
	for (auto i(next(cache_map.begin())) ; i != cache_map.end() ; i++)
	{
		if (i -> second <= *(oldest.front()))
			if (i -> second.lock.try_lock())
				oldest.push(&(i->second));

		if (oldest.size() > line_length)
		{
			oldest.front() -> lock.unlock();
			oldest.pop();
		}
	}

	// Delete everything in the queue from cache
	entry* i;
	while (!oldest.empty())
	{
		i = oldest.front();
		i->del();
		i->lock.unlock();
		oldest.pop();
		
		if (i != dummy)
			entry_count--;
	}
	i = nullptr;

	delete dummy;
}

void cache::fetch_from_disk(int offset, char* put_here)
{
	db_file.seekg(offset, db_file.beg);
	db_file.read(put_here, entry_size);
}
