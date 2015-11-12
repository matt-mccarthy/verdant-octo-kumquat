#include <chrono>

#include "entry_seq.h"

using std::chrono::system_clock;
using std::time_t;


entry_seq::entry_seq(int offset) 
	: db_offset(offset), memory(nullptr), spot(nullptr)
{}

entry_seq::~entry_seq()
{
	if (memory)
		delete[] memory;
}

entry_seq& entry_seq::operator=(entry_seq&& in)
{
	db_offset	= in.db_offset;
	memory		= in.memory;
	spot		= in.spot;
	return *this;
}

void entry_seq::del()
{
	if (is_in_cache())
	{
		delete[] memory;
		memory	= nullptr;
	}
}

bool entry_seq::is_in_cache()
{
	return (bool)memory;
}

