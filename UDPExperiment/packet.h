// use similar format as https://github.com/seedhartha/reone/blob/master/src/common/streamwriter.h
// so it would be easy to merge

#pragma once

#include <memory>
#include <string>
#include <vector>

bool isBigEndian();
void unitTestSetEndian(bool isBig);

template <typename T>
std::enable_if_t< std::is_integral_v<T> || std::is_floating_point_v<T>> swapByte(T& val) {
	char* c = reinterpret_cast<char*>(&val);
	for (size_t i = 0, j = 0; i * 2 < sizeof(T); i += 1) std::swap(c[i], c[sizeof(T) - i - 1]);
}

class PacketWriter {
public:
	PacketWriter() : _buffer() { }

	void write(char obj);

	template <typename T>
	std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>> write(T obj) {
		if (isBigEndian()) swapByte(obj);
		auto c = reinterpret_cast<char*>(&obj);
		for (size_t i = 0; i < sizeof(T); ++i) _buffer.push_back(c[i]);
	}

	void write(std::string obj);

	template<typename ... Args>
	void write(const std::tuple<Args...>& tup) {
		tupleWriteImpl(tup, std::make_index_sequence<sizeof...(Args)>());
	}

	template<typename T>
	void write(std::vector<T> arr) {
		write(static_cast<char>(arr.size()));
		for (T obj : arr) write(obj);
	}

	template<typename T>
	PacketWriter& operator<<(T obj) {
		write(obj);
		return *this;
	}

	std::vector<char> flush() {
		std::vector<char> tmp{};
		std::swap(_buffer, tmp);
		return std::move(tmp);
	}

private:
	std::vector<char> _buffer;

	template<typename Tuple, size_t... Is>
	void tupleWriteImpl(const Tuple& t, std::index_sequence<Is...>) {
		(write(std::get<Is>(t)), ...);
	}
};

class PacketReader {
public:
	PacketReader(std::vector<char>&& buffer) : _buffer(std::move(buffer)) { }

	char get();
	void read(char & obj);

	template <typename T>
	std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>> read(T & obj) {
		auto c = reinterpret_cast<char*>(&obj);
		for (size_t i = 0; i < sizeof(T); ++i) c[i] = _buffer[_pos++];
		if (isBigEndian()) swapByte(obj);
	}

	/* overwrites obj */
	void read(std::string & obj);

	template<typename ... Args>
	void read(std::tuple<Args...>& tup) {
		tupleReadImpl(tup, std::make_index_sequence<sizeof...(Args)>());
	}

	/* overwrite obj */
	template<typename T>
	void read(std::vector<T> & arr) {
		uint8_t size = static_cast<uint8_t>(_buffer[_pos++]);
		arr.clear();
		for (uint8_t i = 0; i < size; ++i) arr.push_back(get());
	}

	template<typename T>
	PacketReader& operator>>(T& obj) {
		read(obj);
		return *this;
	}

private:
	size_t _pos = 0 ;
	std::vector<char> _buffer;

	template<typename Tuple, size_t... Is>
	void tupleReadImpl(Tuple& t, std::index_sequence<Is...>) {
		(read(std::get<Is>(t)), ...);
	}
};
