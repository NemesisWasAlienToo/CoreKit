# Core Kit

| WARNING: This library is still in heavy development and might contain bugs or undergo heavy changes at any time. |
| --- |

Core Kit is general purpose c++ 20 library with focus on networking specially containing commonly used tools and DHT node runner implementation.

## Project architecture

- Library : Contains CoreKit library files
- Sample : Sample source codes
- Source : Main.cpp including main function containing most recent feature example

## Dependencies

- Standard c++ 20 Library
- Openssl 1.1.0 or higher

## Instalation

Core Kit is a header based library, so to use it basically download it, point your compiler include path to it and use it.

## Compilation

Compile with `-lssl -lcrypto` flags for open ssl and `-std=c++2a` for c++ 20 standard libraries.

## Features

Checked items are implemented completly at the moment and unchecked items are to be implemented or completed.

- [x] Test : Simple test and log functions
- [x] Dynamic Lib : _Dynamic Library facilities_
- [x] Duration
- [x] DateTime
- [x] File
- [x] Directory : Directory functionality including content list
- [x] Event : Linux Eventfd file based event mechanism
- [x] Timer : Linux Timerfd file based event mechanism
- [x] Foramt:
    - [x] Base64 : Base64 Encoding
    - [x] Hex : Hexadecimal String Encoding
    - [x] Serializer : Data serializer and deserializer for network data serialization

- [ ] Iterable:
    - [ ] Span : Generic array wrapper
    - [x] List : Generic list
    - [x] Queue : Generic FIFO Queue
    - [ ] Map : Generic red black binary tree map
    - [ ] Linked List
    - [ ] Binary tree
    - [x] Poll : Poll io file descriptor watching mechanism
    - [ ] ePoll : ePoll io file descriptor watching mechanism

- [ ] Network:
    - [x] DNS : Basic DNS lookup functionalities
    - [x] Address : Internet IP v4 and v6 address 
    - [x] EndPoint : End-Point consisting of address, port and some version specific fields 
    - [x] Socket
    - [ ] Http:
        - [ ] : Request
        - [ ] : Response
        - [ ] : Server

    - [ ] DHT : Distributed Hash Table runners and tools
        - [ ] Bucket : Peer bucket lists
        - [x] Handler : _Request_ to _Function_ Mapper for handling incomming new or pending requests
        - [x] Key : N-Byte key (id)
        - [x] Node
        - [x] Server : UDP Server
        - [ ] Runner : A DHT node runner

- [ ] Cryptography:
    - [x] Random : Cryptographicly secure random number generation and tools
    - [x] Digest : Digest functions like SHA , MD
    - [x] RSA
    - [ ] AES

## To do

- Iterable:
    - Add Map
    - Add (red-black & AVL) binary search trees

- Cryptography:
    - Add SHA3
    - Add AES

- Http:
    - Optimize Http request, response
    - Add http Server server

- DHT:
    - Cache time out and swap out policy
    - Remove dedicated thread for server
    - Listen and Dispatch architecture for Server and Handler
    - Task queue between Runner and Server-Handler
    - Redefine fundamental operation data structures
    - Node should perform self look up to notify others
    - Node should update its key values after join
    - End Callbakc Status
    - Server Send and Receive time out
    - Runner shared state for multiple operations
    - Multiple values for a key