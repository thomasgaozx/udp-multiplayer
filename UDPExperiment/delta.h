#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <tuple>
#include "packet.h"

struct CreatureStatus {
    CreatureStatus() : x(0), y(0), z(0), heading(0), // dummy
        anim(0), animframe(0), faction(0) { }

    float x;
    float y;
    float z;
    double heading;
    uint8_t anim;
    uint8_t animframe;
    uint8_t faction;		// hostile reticle
    uint16_t equipment1 = 0;	// set equipment model
    uint16_t equipment2 = 0;
    uint16_t equipment3 = 0;
    uint16_t equipment4 = 0;
    uint16_t equipment5 = 0;
    uint16_t equipment6 = 0;
};

// WARNING: danger of dangling, can only be used in the context of 
// member function
inline auto getSerPtr(CreatureStatus& obj) {
    return std::make_tuple(
        &obj.x,
        &obj.y,
        &obj.z,
        &obj.heading,
        &obj.anim,
        &obj.animframe,
        &obj.faction
    );
}

template <typename ... Args, size_t ... Is>
inline auto getSerImpl(std::tuple<Args*...> &&seqPtr, std::integer_sequence<size_t, Is...>) {
    return std::make_tuple(*(std::get<Is>(seqPtr))...);
}

template <typename T>
inline auto getSerial(T& obj) {
    return getSerImpl(getSerPtr(obj), std::make_index_sequence<std::tuple_size_v<decltype(getSerPtr(obj))>>());
}

template <typename T>
struct serial {
    using ptr_type = decltype(getSerPtr(T{}));
    using type = decltype(getSerial(T{}));
};

template <typename T>
using serial_t = typename serial<T>::type;

template <typename T>
using serial_ptr_t = typename serial<T>::ptr_type;

template <typename T>
constexpr size_t serial_size_v = std::tuple_size_v<serial_t<T>>;

template <typename T>
using serial_index_t = std::make_index_sequence<serial_size_v<T>>;

template <typename T>
constexpr auto serial_index_v = serial_index_t<T>();

/* helper functions */

using expander = int[]; // https://stackoverflow.com/a/25683817

template <typename T, size_t I>
void diffHeaderIter(const serial_t<T>& left, const serial_t<T>& right, uint16_t& header) {
    if (std::get<I>(left) != std::get<I>(right)) {
        header |= 1 << I;
    }
}

template <typename T, size_t ... Is>
uint16_t diffHeader(const serial_t<T>& left, const serial_t<T>& right, std::integer_sequence<size_t, Is...>) {
    uint16_t header = 0;
    expander{ 0, ((void)diffHeaderIter<T, Is>(left, right, header), 0) ... }; // pseudo fold-expansion

    return header;
}

template <typename T, size_t I>
void writeDeltaIter(PacketWriter& writer, const serial_t<T>& o, uint16_t& header) {
    if (header & (1 << I)) {
        writer << std::get<I>(o);
    }
}

template <typename T, size_t ... Is>
void writeDeltaImpl(PacketWriter& writer, const serial_t<T>& o, uint16_t& header, std::integer_sequence<size_t, Is...>) {
    expander{ 0, ((void)writeDeltaIter<T, Is>(writer, o, header), 0) ... }; // pseudo fold-expansion
}


template <typename T, size_t I>
void readDeltaIter(PacketReader& reader, serial_ptr_t<T>& optr, uint16_t& header) {
    if (header & (1 << I)) {
        reader >> *(std::get<I>(optr));
    }
}

template <typename T, size_t ... Is>
void readDeltaImpl(PacketReader& reader, serial_ptr_t<T>&& optr, uint16_t& header, std::integer_sequence<size_t, Is...>) {
    expander{ 0, ((void)readDeltaIter<T, Is>(reader, optr, header), 0) ... };
}

/* END helper functions */

template <typename T>
void writeDelta(const serial_t<T>& cur, const serial_t<T>& old, PacketWriter& writer) {
    uint16_t header = diffHeader<T>(cur, old, serial_index_v<T>);
    writer << header;
    writeDeltaImpl<T>(writer, cur, header, serial_index_v<T>);
}

/* r-value to discourage outside scope usage*/
template <typename T>
void readDelta(serial_ptr_t<T>&& obj, PacketReader& reader) {
    uint16_t header = 0;
    reader >> header;
    readDeltaImpl<T>(reader, move(obj), header, serial_index_v<T>);
}

inline void test() {
    auto stat = CreatureStatus();
    auto fakeseq = getSerial(stat);
    auto fakeseqPtr = getSerPtr(stat);

    auto pakw = PacketWriter();
    auto pakr = PacketReader(std::vector<char>{});

    writeDelta<CreatureStatus>(fakeseq, fakeseq, pakw);
    readDelta<CreatureStatus>(move(fakeseqPtr), pakr);
}
