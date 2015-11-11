#include <chrono>

#include "cache.h"

using std::atomic;
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
using std::vector;

typedef unordered_map<int, int>	db_map;
typedef unique_lock<mutex> slock;

std::time_t get_time()
{
	return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

cache::cache(db_map& mapper, unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines)
			: entry_count(0), stop(false), fetch_count(0),
				entry_size(entry_length), line_length(entries_per_line),
				line_count(num_lines), cache_size(line_count*line_length),
				wait_lock(waiter), reader([](){})
{
	// Initialize the cache map
	for (auto i : mapper)
		cache_map[i.first] = entry(i.second);
}

cache::~cache()
{
	// Make the reader thread stop
	stop = true;
	sleep.notify_all();
	reader.join();
	db_file.close();
}

int cache::get_num_fetches()
{
	return fetch_count;
}

int cache::get_size()
{
	return entry_count;
}

void cache::clear()
{
	queue_lock.lock();
	read_queue = priority_queue<query, vector<query>, query>();
	queue_lock.unlock();
}

bool cache::open(string& db_loc)
{
	reader.join();
	db_file.open(db_loc.c_str());
	//reader = thread(&cache::read, this);
	return db_file.good();
}

char* cache::operator[](int entry_id)
{
	// Get a pointer to the right entry
	entry* cur_entry = &cache_map[entry_id];

	cur_entry->lock.lock();
	// If the entry is not in cache
	if (!(cur_entry->is_in_cache()))
	{
		cur_entry ->lock.unlock();
		add_to_queue(entry_id);

		if (!(cur_entry->is_in_cache()))
			wait.wait(wait_lock);
		cur_entry ->lock.lock();
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
	if (!(cur_entry -> memory))
	{
		// If the cache is full
		if (entry_count >= cache_size)
		{
			//cur_entry -> lock.unlock();
			garbage_collect();
			//cur_entry -> lock.lock();
		}

		// Fetch an entry from disk
		char* new_block			= new char[entry_size];
		cur_entry -> memory		= new_block;
		fetch_from_disk(cur_entry -> db_offset, new_block);
		cur_entry -> in_cache	= true;
		wait.notify_all();

		entry_count++;
		new_block = nullptr;
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
	auto itr = cache_map.find(id);
	itr++;
	queue_lock.lock();
	for (unsigned i = 1 ; i < line_length && itr != cache_map.end() ; i++)
	{
		if ( !(itr->second.is_in_cache()) )
		{
			//queue_lock.lock();

			read_queue.push(query(itr->first, false));
			//sleep.notify_all();

			//queue_lock.unlock();
		}

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
	entry_count++;

	oldest.push(dummy);
	// Populate the queue
	for (auto i(cache_map.begin()) ; i != cache_map.end() ; i++)
	{
		entry* f = oldest.front();
		
		if (f->lock.try_lock())
		{
			if (i ->second.lock.try_lock())
			{
				if (i -> second.memory != nullptr && f -> memory != nullptr)
					if (i -> second.accessed <= (*oldest.front()).accessed )
						oldest.push(&(i->second));

				i->second.lock.unlock();
			}
			f->lock.unlock();
		}

		f = nullptr;

		if (oldest.size() > line_length)
			oldest.pop();
	}

	// Delete everything in the queue from cache
	entry* i;
	while (!oldest.empty())
	{
		i = oldest.front();
		i->lock.lock();
		i->del();
		i->lock.unlock();
		oldest.pop();
	
		entry_count--;
	}
	i = nullptr;
	
	if (dummy -> memory)
		entry_count--;

	delete dummy;
}

void cache::fetch_from_disk(int offset, char* put_here)
{
	db_file.seekg(offset, db_file.beg);
	db_file.read(put_here, entry_size);
}
