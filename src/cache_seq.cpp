#include "cache_seq.h"

using std::queue;
using std::string;
using std::unordered_map;
using std::vector;

typedef unordered_map<int, int>	db_map;

cache_seq::cache_seq(db_map& mapper, unsigned entry_length, unsigned entries_per_line,
				unsigned num_lines)
			: fetch_count(0), entry_count(0), entry_size(entry_length),
				line_length(entries_per_line), line_count(num_lines),
				cache_size(line_count*line_length)
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
	{
		cache_cont.erase(cur_entry->spot);
		cache_cont.push_back(cur_entry);
		cur_entry -> spot = prev(cache_cont.end());
	}

	char* location(cur_entry->memory);

	cur_entry = nullptr;

	return location;
}

void cache_seq::add_to_db(int id)
{
	// Get a pointer to the right entry
	entry_seq* cur_entry = &cache_map[id];

	// If the cache is full
	if (get_size() >= cache_size)
		garbage_collect();

	// Fetch an entry from disk
	char* new_block			= new char[entry_size];
	cur_entry -> memory		= new_block;
	fetch_from_disk(cur_entry -> db_offset, new_block);

	new_block = nullptr;
	cache_cont.push_back(cur_entry);
	cur_entry -> spot = prev(cache_cont.end());
	entry_count++;

	cur_entry = nullptr;
}

void cache_seq::add_to_queue(int id)
{
	fetch_count++;

	add_to_db(id);

	// Enqueue the next few ids with normal priority
//	auto itr = cache_map.find(id);
//	itr++;
//	for (unsigned i = 1 ; i < line_length && itr != cache_map.end() ; i++)
//	{
//		if ( !(itr->second.is_in_cache()) )
//			add_to_db(itr->first);

//		else
//			itr->second.accessed = get_time_s();

//		itr++;
//	}
}

void cache_seq::garbage_collect()
{
	for (int i = 0 ; i < line_length && !cache_cont.empty(); i++)
	{
		entry_seq* f(cache_cont.front());
		f ->del();
		cache_cont.pop_front();
	}

	entry_count -= line_length;
}

void cache_seq::fetch_from_disk(int offset, char* put_here)
{
	db_file.seekg(offset, db_file.beg);
	db_file.read(put_here, entry_size);
}
