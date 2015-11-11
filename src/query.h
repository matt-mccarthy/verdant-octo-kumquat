struct query
{
	public:
		query() {}
		query(int id, bool now)
		{
			this -> id	= id;
			immediate	= now;
		}
		int		id;
		bool	immediate;
		bool	operator()(const query& a, const query& b)
		{
			return (a.immediate && !b.immediate);
		}
};
