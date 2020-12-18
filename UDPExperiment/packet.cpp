
#include "packet.h"
#include <cstdint>
#include <stdexcept>
#include <limits>

uint16_t test = 0x1100;
bool _isBigEndian = *(char*)&test;

bool isBigEndian() {
	return _isBigEndian;
}

void unitTestSetEndian(bool isBig) {
	_isBigEndian = isBig;
}

void PacketWriter::write(char obj) {
	_buffer.push_back(obj);
}

void PacketWriter::write(std::string obj) {
	if (obj.size() > std::numeric_limits<uint8_t>::max())
		throw std::overflow_error("string too big");

	write(static_cast<char>(obj.size()));
	for (char c : obj) _buffer.push_back(c);
}

char PacketReader::get()
{
	return _buffer[_pos++];
}

void PacketReader::read(char& obj) {
	obj = _buffer[_pos++];
}

void PacketReader::read(std::string& obj) {
	uint8_t size = static_cast<uint8_t>(_buffer[_pos++]);

	obj.clear();
	for (uint8_t i = 0; i < size; ++i) {
		char tmp{};
		read(tmp);
		obj.push_back(std::move(tmp));
	}
}
