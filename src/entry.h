#include <atomic>
#include <ctime>
#include <mutex>

class entry
{
	public:
		entry() {}
		entry(int offset);
		entry&	operator=(entry&& in);
		void	del();
		bool	is_in_cache();
		~entry();

		int							db_offset;
		char*						memory;
		std::atomic<std::time_t>	accessed;
		std::mutex					lock;
		std::atomic_bool			in_cache;
};
