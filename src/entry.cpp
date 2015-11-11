#include <chrono>

#include "entry.h"

using std::chrono::system_clock;
using std::time_t;


entry::entry(int offset) 
	: db_offset(offset), memory(nullptr), in_cache(false),
		accessed(system_clock::to_time_t(system_clock::now())) {}

entry::~entry()
{
	if (memory)
		delete[] memory;
}

entry& entry::operator=(entry&& in)
{
	db_offset	= in.db_offset;
	memory		= in.memory;
	accessed	= (time_t)in.accessed;
	in_cache	= (bool)in.in_cache;

	return *this;
}

void entry::del()
{
	if (is_in_cache())
	{
		in_cache = false;
		delete[] memory;
		memory = nullptr;
	}
}

bool entry::is_in_cache()
{
	return in_cache;
}

