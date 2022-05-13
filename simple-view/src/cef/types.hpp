#include <memory>

template<class T>
struct buffer
{
	std::unique_ptr<T[]> data { nullptr};
	size_t			     size { 0 };

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
		data.reset(nullptr);
	}

	buffer<T>& operator=(buffer<T>&& rhs) noexcept
	{
		data = std::move(rhs.data);
		size = std::move(rhs.size);

		rhs.size = 0;

		return *this;
	}
};
