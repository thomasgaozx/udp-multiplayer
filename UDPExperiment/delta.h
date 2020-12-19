#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <tuple>
#include "packet.h"

template <typename T>
void writeDelta(PacketWriter& writer, T& cur, T& old);

template <typename T>
void readDelta(PacketReader& reader, T& obj);
