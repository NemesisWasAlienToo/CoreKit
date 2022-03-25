#pragma once

#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <system_error>

#include <Descriptor.hpp>

namespace Core
{
    class Semaphore : public Descriptor
    {
    public:
        Semaphore() = default;
    };
}
