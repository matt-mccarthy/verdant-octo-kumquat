#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

class cache
{
	public:
		cache(std::unordered_map<int,int>& mapper, std::string& db_loc,
				unsigned entry_size, unsigned num_entries,
				unsigned thread_count);
		~cache();
		
		char* operator[](int entry_id);
		
	private:
		void fetch(int offset, char* put_here);
		
		char**					cached_entries;
		std::queue				read_queue;
		std::mutex				queue_lock;
		std::thread				reader;
		std::condition_variable	sleep;
};
