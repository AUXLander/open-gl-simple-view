#include <memory>

template<class T>
struct buffer
{
	std::unique_ptr<T[]> data;
	size_t			     size;

	buffer() = default;
	buffer(buffer&& rhs)
	{
		*this = std::move(rhs);
	}

	buffer(size_t size) :
		data{ new T[size]{0} }, size{ size }
	{;}

	~buffer()
	{
		std::cout << "destruct " << size << '\n';
		data.reset(nullptr);
	}

	buffer<T>& operator=(buffer<T>&& rhs)
	{
		data = std::move(rhs.data);
		size = std::move(rhs.size);

		rhs.size = 0;

		return *this;
	}
};