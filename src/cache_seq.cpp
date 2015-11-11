#include <chrono>

#include "cache_seq.h"

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::system_clock;
using std::queue;
using std::string;
using std::time_t;
using std::unordered_map;
using std::vector;

typedef unordered_map<int, int>	db_map;

std::time_t get_time_s()
{
	return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

cache_seq::cache_seq(db_map& mapper, unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines)
			: entry_count(0), fetch_count(0),
				entry_size(entry_length), line_length(entries_per_line),
				line_count(num_lines), cache_size(line_count*line_length)
{
	// Initialize the cache map
	for (auto i : mapper)
		cache_map[i.first] = entry_seq(i.second);
}

cache_seq::~cache_seq()
{
	db_file.close();
}

int cache_seq::get_num_fetches()
{
	return fetch_count;
}

int cache_seq::get_size()
{
	return entry_count;
}

bool cache_seq::open(string& db_loc)
{
	db_file.open(db_loc.c_str());
	return db_file.good();
}

char* cache_seq::operator[](int entry_id)
{
	// Get a pointer to the right entry
	entry_seq* cur_entry = &cache_map[entry_id];

	// If the entry is not in cache
	if (!(cur_entry->is_in_cache()))
		add_to_queue(entry_id);

	else
		cur_entry->accessed = get_time_s();

	char* location(cur_entry->memory);

	cur_entry = nullptr;

	return location;
}

void cache_seq::add_to_db(int id)
{
	// Get a pointer to the right entry
	entry_seq* cur_entry = &cache_map[id];

	// If the entry is not in cache
	if (!(cur_entry -> memory))
	{
		// If the cache is full
		if (entry_count >= cache_size)
			garbage_collect();

		// Fetch an entry from disk
		char* new_block			= new char[entry_size];
		cur_entry -> memory		= new_block;
		fetch_from_disk(cur_entry -> db_offset, new_block);

		entry_count++;
		new_block = nullptr;
	}

	// If it is, give it a new access time
	else
		cur_entry ->accessed = get_time_s();

	cur_entry = nullptr;
}

void cache_seq::add_to_queue(int id)
{
	fetch_count++;

	add_to_db(id);

	// Enqueue the next few ids with normal priority
	auto itr = cache_map.find(id);
	itr++;
	for (unsigned i = 1 ; i < line_length && itr != cache_map.end() ; i++)
	{
		if ( !(itr->second.is_in_cache()) )
			add_to_db(itr->first);

		else
			itr->second.accessed = get_time_s();

		itr++;
	}
}

void cache_seq::garbage_collect()
{
	// Find the first entry in cache
	queue<entry_seq*> oldest;

	entry_seq* dummy = new entry_seq(0);
	entry_count++;

	oldest.push(dummy);
	// Populate the queue
	for (auto i(cache_map.begin()) ; i != cache_map.end() ; i++)
	{
		entry_seq* f = oldest.front();
		
		if (i -> second.memory != nullptr && f -> memory != nullptr)
			if (i -> second.accessed <= (*oldest.front()).accessed )
				oldest.push(&(i->second));

		f = nullptr;

		if (oldest.size() > line_length)
			oldest.pop();
	}

	// Delete everything in the queue from cache
	entry_seq* i;
	while (!oldest.empty())
	{
		i = oldest.front();
		i->del();
		oldest.pop();
	
		entry_count--;
	}
	i = nullptr;
	
	if (dummy -> memory)
		entry_count--;

	delete dummy;
}

void cache_seq::fetch_from_disk(int offset, char* put_here)
{
	db_file.seekg(offset, db_file.beg);
	db_file.read(put_here, entry_size);
}
