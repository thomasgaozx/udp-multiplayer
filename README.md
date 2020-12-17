# Multiplayer Networking

- [Multiplayer Networking](#multiplayer-networking)
  - [Useful Resources](#useful-resources)
  - [Requirements](#requirements)
  - [Terminology](#terminology)
  - [Snapshot and Delta Serialization](#snapshot-and-delta-serialization)
  - [Architecture](#architecture)
    - [Data Structure Samples](#data-structure-samples)
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

## Terminology

Two-way serializable:

- `Notification`: deliver a message (debugging, for example), but does not cause in-game effects

Server-to-client serializables:

- `GameState`: continuous payload that carries gameplay states (e.g. creature position, animationState)
- `Command`: one-time payload, and cause some in-game effects
  - WorldCommand (e.g. door opening, ambient visual effect application)
  - InterruptCommand (e.g. playMovie, startDialog)
  - ContextualCommand (e.g. animated damage text)

Client-to-server serializable:

- `Event`: movement, interactions
  - Movement Event
  - Interact Event
  - Shoot/QueueActionEvent

## Snapshot and Delta Serialization

- **Status**: `GameState` associated to an object in game
- **Snapshot**: collection of all relevant object status in game
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

| Field type            | Field header                  | Field Body                          |
| --------------------- | ----------------------------- | ----------------------------------- |
| Primitive Type        | None                          | Big Endian                          |
| Primitive-Typed Tuple | None                          | Concatenate bytes of premitive type |
| Primitive-Typed Array | 1 Byte indicating array size  | Concatenate bytes of premitive type |
| String                | 1 Byte indicating string size | dump in the bytes                   |

**Delta string of structure**: First two bytes will indicate which fields are changed. The subsequent bytes will represent the changed fields (see field serialization), respectively.

**Packets Structure**: UDP packet size is restricted to 1500 bytes. So a single UDP packet may not convey the entirety of a Master Snapshot, so UDP packet has the following structure.

- The first 4 bytes (uint32_t) will indicate snapshot sequence
- The next 4 bytes (uint32_t) will be the tick
- The body is a bundle of multiple structure delta strings. Each delta string has a 4-byte prefix (uint32_t): Creature ID (client can then look up type of structure used) followed by its delta string.

Note that there are no way to know whether the entirety of a snapshot is properly conveyed, but that is of no consequence.

**Huffman Compression**: use Vitter's algorithm. Do not update NYT (which include any floating point number, bit mask, etc.), encode all frequently used `uint8_t` e.g. (0-100). This is not a priority but would be nice to have on a per-packet basis.

Limitations:

1. Field arrays will be capped at 255 entries
2. Each Status structure can have a maximum of 16 fields.

Testing:

Each level of serialization requires a test suite on its own, and should be ran via `main()`.

## Architecture

### Data Structure Samples

Server to Client Objects:

```c++
struct GameState {
    uint32_t objId;
    uint32_t timestamp; // for interpolation

    glm::vec3 position;
    float heading;

    // ... args
};

struct Command {
    bool ack = false;

    uint8_t type;

    // ... args
};

struct CommandWorld : public Command {};
struct CommandInterrupt : public Command {};
struct CommandContextual : public Command {};
```

Client to Server Objects:

```c++
struct Event {
    bool ack = false;

    uint8_t type;
    uint32_t timestamp; // for lag compensation

    // ... args
};

struct EventMovement : public Event {};
struct EventInteract : public Event {};
```

Interpolation Data Structure: history of `GameState`s.
This is very rudimentary, it may be better to make a subclass of SpatialObject.

```c++
class GameStateHistory {
public:
    /** interpolate or extrapolate */
    glm::vec3 getPosition(uint32_t id);

    /** interpolate or extrapolate */
    glm::vec3 getHeading(uint32_t id);

private:
    /** maps objId to its history of GameState */
    std::map<uint32_t, std::list<GameState>>;
};
```

**Socket Buffer Workers**: use threadpool or (queue + multiple worker threads)

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
