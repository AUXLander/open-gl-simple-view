#pragma once

#include <fstream>


template<class T>
struct point
{
	T x;
	T y;
	T z;

	size_t layer;

	point() = default;

	point(T x, T y, T z, size_t layer) :
		x(x), y(y), z(z), layer(layer)
	{;}

	point(const point<T>& other) :
		x(other.x), y(other.y), z(other.z),
		layer(other.layer)
	{;}

	friend std::fstream& operator<<(std::fstream& stream, const point<T>& p)
	{
		stream.write((const char*)&p.x, sizeof(x));
		stream.write((const char*)&p.y, sizeof(y));
		stream.write((const char*)&p.z, sizeof(z));

		stream.write((const char*)&p.layer, sizeof(layer));

		return stream;
	}

	friend std::fstream& operator>>(std::fstream& stream, point<T>& p)
	{
		stream.read((char*)&p.x, sizeof(x));
		stream.read((char*)&p.y, sizeof(y));
		stream.read((char*)&p.z, sizeof(z));

		stream.read((char*)&p.layer, sizeof(layer));

		return stream;
	}
};

using point_t = point<double>;