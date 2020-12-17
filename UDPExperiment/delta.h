#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <tuple>
#include "packet.h"

struct CreatureStatus {
	int32_t x;
	int32_t y;
	int32_t z;
	uint32_t heading;
	uint8_t anim;
	uint8_t animframe;
	uint8_t faction;		// hostile reticle
	uint16_t equipment1;	// set equipment model
	uint16_t equipment2;
	uint16_t equipment3;
	uint16_t equipment4;
	uint16_t equipment5;
	uint16_t equipment6;
};

inline auto getSequence(const CreatureStatus& obj) {
	return std::make_tuple(
		obj.x,
		obj.y,
		obj.z,
		obj.heading,
		obj.anim,
		obj.animframe,
		obj.faction
	);
}

template <typename T>
struct seq {
	using type = decltype(getSequence(T{}));
};

template <typename T>
using seq_t = typename seq<T>::type;

template <typename T>
constexpr size_t seq_size_v = std::tuple_size_v<seq_t<T>>;

template <typename T>
using seq_index_t = std::make_index_sequence<seq_size_v<T>>;

template <typename T>
constexpr auto seq_index_v = seq_index_t<T>();

/* helper functions */

using expander = int[]; // https://stackoverflow.com/a/25683817

template <typename T, size_t I>
void diffHeaderIter(const seq_t<T>& left, const seq_t<T>& right, uint16_t& header) {
	if (std::get<I>(left) != std::get<I>(right)) {
		header |= 1 << I;
	}
}

template <typename T, size_t ... Is>
uint16_t diffHeader(const seq_t<T>& left, const seq_t<T>& right, std::integer_sequence<size_t, Is...>) {
	uint16_t header = 0;
	expander{ 0, ((void)diffHeaderIter<T, Is>(left, right, header), 0) ... }; // pseudo fold-expansion

	return header;
}

template <typename T, size_t I>
void writeDeltaIter(PacketWriter& writer, const seq_t<T>& o, uint16_t& header) {
	if (header & (1 << I)) {
		writer << std::get<I>(o);
	}
}

template <typename T, size_t ... Is>
void writeDeltaImpl(PacketWriter& writer, const seq_t<T>& o, uint16_t& header, std::integer_sequence<size_t, Is...>) {
	expander{ 0, ((void)writeDeltaIter<T, Is>(writer, o, header), 0) ... }; // pseudo fold-expansion
}

/* END helper functions */

template <typename T>
void writeDelta(const seq_t<T>& cur, const seq_t<T>& old, PacketWriter& writer) {
	uint16_t header = diffHeader<T>(cur, old, seq_index_v<T>);
	writer << header;

	writeDeltaImpl<T>(writer, cur, header, seq_index_v<T>);
}

inline void test() {
	auto fakeseq = getSequence(CreatureStatus());
	auto pakw = PacketWriter();

	writeDelta<CreatureStatus>(fakeseq, fakeseq, pakw);
}