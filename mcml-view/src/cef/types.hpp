#pragma once
#include <memory>

template<class T>
struct buffer
{
	std::unique_ptr<T[]> data { nullptr };
	size_t			     size { 0 };

	buffer() = default;
	buffer(buffer&& rhs)
	{
		*this = std::move(rhs);
	}

	buffer(size_t size) :
		data{ }, size{ size }
	{
		auto* p = new T[size]{ 0 };

		data.reset(p);
	
	}

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

	void reset(T* data, size_t size)
	{
		this->data.reset(data);
		this->size = size;
	}

	buffer<T> clone() const
	{
		buffer<T> temporary(size);

		std::copy(data.get(), data.get() + size, temporary.data.get());

		return temporary;
	}
};
