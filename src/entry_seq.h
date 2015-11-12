#include <atomic>
#include <list>

class entry_seq
{
	public:
		entry_seq() {}
		entry_seq(int offset);
		entry_seq&	operator=(entry_seq&& in);
		void	del();
		bool	is_in_cache();
		~entry_seq();

		int								db_offset;
		char*							memory;
		std::list<entry_seq*>::iterator	spot;
};
