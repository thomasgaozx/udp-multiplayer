#include "delta.h"

using namespace std;

template <typename ... Args, size_t ... Is>
auto getSerImpl(std::tuple<Args*...>&& seqPtr, std::integer_sequence<size_t, Is...>) {
    return std::make_tuple(*(std::get<Is>(seqPtr))...);
}

template <typename T>
auto getSerial(T& obj) {
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
void writeDeltaBuff(const serial_t<T>& cur, const serial_t<T>& old, PacketWriter& writer) {
    uint16_t header = diffHeader<T>(cur, old, serial_index_v<T>);
    writer << header;
    writeDeltaImpl<T>(writer, cur, header, serial_index_v<T>);
}

template <typename T>
void writeDelta(PacketWriter& writer, T& cur, T& old) {
    writeDeltaBuff<T>(getSerial(cur), getSerial(old), writer);
}

/* r-value to discourage outside scope usage*/
template <typename T>
void readDeltaBuff(serial_ptr_t<T>&& obj, PacketReader& reader) {
    uint16_t header = 0;
    reader >> header;
    readDeltaImpl<T>(reader, move(obj), header, serial_index_v<T>);
}

template <typename T>
void readDelta(PacketReader& reader, T& obj) {
    readDeltaBuff<T>(getSerPtr(obj), reader);
}

#define DECL_SERIAL(T, ...) auto getSerPtr(T & obj) \
    { return std::make_tuple(__VA_ARGS__); } \
    template void writeDelta<T>(PacketWriter& writer, T& cur, T& old); \
    template void readDelta<T>(PacketReader& reader, T& obj);

DECL_SERIAL(CreatureStatus, &obj.x, &obj.y, &obj.z, &obj.heading, &obj.anim, &obj.animframe, &obj.faction);

// WARNING: danger of dangling, can only be used in the context of 
// member function
// auto getSerPtr(CreatureStatus& obj) {
//     return std::make_tuple(
//         &obj.x,
//         &obj.y,
//         &obj.z,
//         &obj.heading,
//         &obj.anim,
//         &obj.animframe,
//         &obj.faction
//     );
// }

// template void writeDelta<CreatureStatus>(PacketWriter& writer, CreatureStatus& cur, CreatureStatus& old);
// template void readDelta<CreatureStatus>(PacketReader& reader, CreatureStatus& obj);

