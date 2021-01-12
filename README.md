# Multiplayer Networking

- [Multiplayer Networking](#multiplayer-networking)
  - [Useful Resources](#useful-resources)
  - [Requirements](#requirements)
  - [Connection and Initialization](#connection-and-initialization)
  - [Timing Parameters](#timing-parameters)
  - [Snapshot and Delta Serialization](#snapshot-and-delta-serialization)
  - [Broadcast Packet Structure](#broadcast-packet-structure)
  - [Threading Model](#threading-model)
    - [Client Architecture](#client-architecture)
    - [Server Architecture](#server-architecture)

## Useful Resources

- [Q3 UDP Architecture](https://web.archive.org/web/20190425175004/http://trac.bookofhook.com/bookofhook/trac.cgi/wiki/Quake3Networking), and [Another Analysis](https://fabiensanglard.net/quake3/network.php), and [More](http://caia.swin.edu.au/reports/110209A/CAIA-TR-110209A.pdf)
- [Q3 Source Code](https://github.com/id-Software/Quake-III-Arena/tree/master/code/cgame)
- [Source Engine Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking): note the interpolation, input prediction and lag compensation.
- [Prediction and Compensation Tutorial](https://www.gabrielgambetta.com/client-server-game-architecture.html)
- [UDP over SSH Tunnel](https://www.qcnetwork.com/vince/doc/divers/udp_over_ssh_tunnel.html)
- [Delta compression, interesting read](https://stackoverflow.com/questions/29310983/what-application-game-state-delta-compression-techniques-algorithm-do-exist)
- [`boost::asio` Tutorials](https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/tutorial.html), and [Anatomy](https://www.boost.org/doc/libs/1_74_0/doc/html/boost_asio/overview/core/basics.html)
- [UDP Structure Overview](https://docs.microsoft.com/en-us/archive/msdn-magazine/2006/february/udp-delivers-take-total-control-of-your-networking-with-net-and-udp)

## Requirements

- UDP-only for simplicity (refer to Q3 architecture)
- Client interpolation
- Client input prediction and correction
- Security (UDP over SSH Tunnel?)
- Delete Hackers (Server does everything)
- Test suite: lag simulation, metric overlay, etc.

There should be 3 modes of traffic:

1. **Bulk**: using TCP to load resources remotely, should be done once in the beginning of connect/reconnect to establish a **base**
2. **Fallback**: continuous UDP payloads, full update from the **base**. This is used when client has poor connection and constantly miss a massive bunch of updates.
3. **Performant**: continuous UDP payloads, much smaller packet than fallback mode. It contains the partial update from the last known client state (that's still recorded).

The networking system should consist of

- **Rudimentary RPC** (TCP), to communicate data such as logs, inventory items, and getting global/local variables from server.
- **Broadcasting System** (UDP), to update objects in real time.

To support easier connection between players, a dedicated, publically visable server (e.g. digital ocean) is required to

- Act as a TCP/IP proxy and establish tunnels between players
- Act as a STUN server for UDP hole punching.
- Meet industry security criteria (research required)

## Connection and Initialization

This cycle occurs for all module.

**Handshake**: a TCP connection from client to server for confirmation.

**Loading**: when host enters a module, all connected clients will load necessary resources from server
(e.g. save file, in the future perhaps module file and models to accommodate for mods).

**Initialize**: right after loading, client ping server first, server decides the game mode (spectator or PC) for each client, and broadcasts transmission parameters (see Timing Parameters) and wait for client's ACK before broadcasting packets to established clients.

**Disconnect**: if a client failed to respond for 60 seconds after 32 snapshots, the client will be removed.

**Reconnect**: client ping server with whatever UDP packet.
Server will respond with an `invalid+reason` signal, from which the client will know whether the host is still in the same module, or in a new module.

- If a client becomes online again in the same module after disconnect, it will ping server again and follow the initialize procedure.
- If it is a new module, then client has to restart from handshake again.

Potential problems with absolute diff:

- In case of full inventory change and object replacement, an absolute diff will have double the size compared to a normal snapshot transfer.

## Timing Parameters

- `tickrate`: number of snapshots broadcasted per second, default to 20 (industry standard)
- `lerp`: interpolation period (default to 100 ms by industry standard). Regardless how long the value is set to, we'll only interpolate the 3 consecutive packets. (One could use this parameter to simulate delay).

## Snapshot and Delta Serialization

- **Status**: `GameState` associated to an object in game
- **Snapshot**: collection of all relevant **object status** in game, **object diff** per-frame, **inventory diff** per-frame, **ballistic/projectile** type+initial position+timestamp (client is responsible for managing the life cycle)
- **History**:
  - For server: global collection of snapshots with timestamp and sequence number, dynamically sized with the very last ACK received from any client, max size 32.
  - For client: same as server, but size 3 for interpolation.
  - In the case a client hang, it is treated as disconnected: client will automatically reset, server will only respond to packets after re-handshake.

Serialization technique: consider the following structure,

```c++
struct CreatureStatus {
    std::tuple<uint32_t, uint32_t, uint32_t> pos;
    uint32_t heading;
    uint8_t anim;
    uint8_t animframe;
    uint8_t faction;        // hostile reticle
    uint16_t equipment1;    // set equipment model
    uint16_t equipment2;
    uint16_t equipment3;
    uint16_t equipment4;
    uint16_t equipment5;
    uint16_t equipment6;
};
```

**Field serialization**:

| Field type     | Field Header          | Field Body      |
| -------------- | --------------------- | --------------- |
| Primitive Type | N/A                   | Little Endian   |
| String         | N/A                   | Null-Terminated |
| List/Container | One byte specify size | Null-Terminated |

**Delta string of structure**: First 4 bytes (uint32_t) will indicate which fields are changed. (Each Status structure can have a maximum of 32 fields.) The subsequent bytes will represent the changed fields (see field serialization), respectively.

**Snapshot**: a full record of all area game state including

- All object status in the area
- Full inventory item list
- Global/local game variables

## Broadcast Packet Structure

A packet consists of a header and a body.

**Header**:

- The first 4 bytes (uint32_t) will indicate Server snapshot sequence
- The second 4 bytes (uint32_t) will indicate last client ACK sequence
- The next 4 bytes (uint32_t) will be the tick

**Body**: arbitrary number of lists. Each list contains a sub-header and a body.

- **List sub-header**:
  - 1 byte (uint8_t) indicate list type
  - 1 byte (uint8_t) indicate list length

```c
enum class PackListType : uint8_t {
  CreatureDiff,
  CreatureFull,
  SpatialDiff,
  SpatialFull,
  KeepAlive,
  Ballistics,
  PlayAnimation,
  VisualEffect
}
```

Where each type corresponds to a structure:

- CreatureDiff: ID + 16 bit sub-header for delta diff + actual diff content
- CreatureFull: ID + full content without header (redundant)
- SpatialDiff: performant
- SpatialFull: fallback
- KeepAlive: ID, so client knows to keep render the object
- Ballistics: ID'd by the unique (timestamp, initial_position) tuple, only relevant in their lifecycle
- PlayAnimation: ID + timestamp + flags + speed + resref, Timestamped command. Client is responsible for adjusting speed while compiling the list of AnimInfo
  - If it's between 0-10 frames(seq) behind, do not modify speed
  - If it's between 11-20 frames behind, double the speed
  - If it's between 21-30 frames behind, triple the speed
- VisualEffect: Timestamped command

> Note that client will construct snapshots based on the packets.
> If a creature has an unknown ID, client will use RPC to get the creature's resref and construct it in Area.

**Huffman Compression**: use Vitter's algorithm. Do not update NYT (which include any floating point number, bit mask, etc.), encode all frequently used `uint8_t` e.g. (0-100). This is not a priority but would be nice to have on a per-packet basis.

## Threading Model

Will be here when its time to worry about those ...

### Client Architecture

Client Game Loop:

- Client-side prediction (movement, shoot, etc.)
- Push events asynchronously, e.g. `pushEvent(e, ack=False)`
- Interpolate NPC/creature locations and/or animation states
- Render

Client receive buffer workers:

- Update `GameState`; decide  correction
- Handle `Command`, apply effects

### Server Architecture

Server Game Loop:

- Same logic as single-player game loop
- Lag compensation (maybe not applicable for OG base game, but keep this in mind for the future)
- Render

Server transmit buffer workers:

- Broadcast `GameState`, by default 10 times per second.
  - Use delta compression to minimize traffic
  - Only worry about moveable objects in render-range, otherwise there are simply too much traffic
  - If necessary, broadcast 10 `GameState`s at once by rotation
- Broadcast any `Command` as soon as they become available

Server receive buffer workers:

- Handle events from client, update positions/queue actions, etc.
