# Core Kit

| WARNING: This library is still in heavy development and might contain bugs or undergo heavy changes at any time. |
| --- |

Core Kit is general purpose c++ 20 library with focus on networking specially containing commonly used tools and DHT node runner implementation.

## Project architecture

- Library : Contains CoreKit library files
- Sample : Sample source codes

## Dependencies

- Standard c++ 20 Library
- Openssl 1.1.0 or higher

## Instalation

Core Kit is a header based library, so to use it basically download it, point your compiler include path to it and use it.

## Compilation

Compile with `-lssl -lcrypto` flags for open ssl and `-std=c++2a` for c++ 20 standard libraries.

As an example to compile Main.cpp using g++ :
```sh
g++ Source/Main.cpp -o CoreKit.elf -std=c++2a -Wall -ILibrary -pthread -lssl -lcrypto
```

## Features

Checked items are implemented completly at the moment and unchecked items are to be implemented or completed.

- [x] Test : Simple test and log functions
- [x] Dynamic Lib : _Dynamic Library facilities_
- [x] Duration
- [x] DateTime
- [x] File
- [x] Directory : Directory functionality including content list
- [x] Event : Linux Eventfd based event mechanism
- [x] Timer : Linux Timerfd based timer mechanism
- [x] Coroutine : Linux implementation of a stackful asymmetric coroutine
- [x] Machine : Linux implementation of a duff's device state machine coroutine
- [x] Foramt:
    - [x] Base64 : Base64 Encoding
    - [x] Hex : Hexadecimal String Encoding
    - [x] Serializer : Data serializer and deserializer for network data packing
    - [x] Stream : Data stream

- [ ] Storage:
    - [ ] Sqlite3 : Sqlite3 wrapper class

- [ ] Iterable:
    - [x] Span : Generic array wrapper
    - [x] List : Generic list
    - [x] Queue : Generic FIFO Queue
    - [ ] Map : Generic red black binary tree map
    - [ ] Linked List
    - [ ] Binary tree
    - [x] Poll : Poll io file descriptor watching mechanism
    - [x] ePoll : ePoll io file descriptor watching mechanism

- [ ] Network:
    - [x] DNS : Basic DNS lookup functionalities
    - [x] Address : Internet IP v4 and v6 address 
    - [x] EndPoint : End-Point consisting of address, port and some version specific fields 
    - [x] Socket
    - [ ] Http:
        - [x] : Request
        - [x] : Response
        - [x] : Server
        - [ ] : Controller

    - [x] DHT : Distributed Hash Table runners and tools
        - [x] Cache : Peer cache policy
        - [x] Handler : _Request_ to _Function_ Mapper for handling incomming new or pending requests
        - [x] Key : N-Byte key (id)
        - [x] Node
        - [x] Server : UDP Server
        - [x] Runner : A DHT node runner

- [x] Cryptography:
    - [x] Random : Cryptographicly secure random number generation and tools
    - [x] Digest : Digest functions like SHA , MD
    - [x] RSA
    - [x] AES

## To do

- Format
    - Optmize bit-copy-able objects Add and Take

- Iterable:
    - Add Linked-List
    - Add Map
    - Add (red-black & AVL) binary search trees

- Cryptography:
    - Add SHA3
    - Move States to heap to protect user against leaks
    - RSA Error handling

- Http:
    - Add Controller

- DHT:
    - Task queue between Runner and Server-Handler
    - Server connections limit