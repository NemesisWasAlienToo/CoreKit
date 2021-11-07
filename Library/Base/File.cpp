#pragma once

#include <iostream>
#include <functional>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "Base/Descriptor.cpp"

namespace Core
{
    class File : public Descriptor
    {
    private:
        /* data */
    public:
        enum TestType
        {
            CanRead = R_OK,
            CanWrite = W_OK,
            CanExecute = X_OK,
        };

        enum PermissionType
        {
            UserID = S_ISUID,
            GroupID = S_ISGID,

            OwnerRead = S_IRUSR,
            OwnerWrite = S_IWUSR,
            OwnerExecute = S_IXUSR,
            OwnerAll = S_IRWXU,

            GroupRead = S_IRGRP,
            GroupWrite = S_IWGRP,
            GroupExecute = S_IXGRP,
            GroupAll = S_IRWXG,

            OtherRead = S_IROTH,
            OtherWrite = S_IWOTH,
            OtherExecute = S_IXOTH,
            OtherAll = S_IRWXO,
        };

        enum FlagType
        {
            ReadOnly = O_RDONLY,
            WriteOnly = O_WRONLY,
            ReadWrite = O_RDWR,
            CreateFile = O_CREAT,
            Append = O_APPEND,
            CloseOnExec = O_CLOEXEC,
        };

        enum SeekType
        {
            Start = SEEK_SET,
            Current = SEEK_CUR,
            End = SEEK_END,
            Data = SEEK_DATA,
            Hole = SEEK_HOLE,
        };

        // ### Constructors

        File() = default;

        File(int Handler) : Descriptor(Handler) {}

        File(const File &Other) : Descriptor(Other) {}

        // ### Functionalities

        auto Write(const void *Data, size_t Size)
        {
            return write(_INode, Data, Size);
        }

        auto Read(void *Data, size_t Size)
        {
            return read(_INode, Data, Size);
        }

        // ### Static functions

        static inline bool Exist(const std::string &Name, int Tests = F_OK)
        {
            return (access(Name.c_str(), Tests) != -1);
        }

        static File Create(const std::string &Path, uint32_t Permissions)
        {
            int Result = creat(Path.c_str(), Permissions);

            if (Result < 0)
            {
                throw std::invalid_argument(strerror(errno));
            }

            return Result;
        }

        static File Open(const std::string &Path, int Flags = 0, uint32_t Permissions = 0)
        {
            if (Flags & Append)
            {
                if (!(Flags & (ReadWrite | WriteOnly)))
                    throw std::invalid_argument("Append flag must come with write permission");
            }

            int Result = open(Path.c_str(), Flags, Permissions);

            if (Result < 0)
            {
                throw std::invalid_argument(strerror(errno));
            }

            return Result;
        }

        static void Remove(const std::string &Path)
        {
            int Result = remove(Path.c_str());

            if (Result < 0)
            {
                throw std::invalid_argument(strerror(errno));
            }
        }

        // void Run(const std::string& Path)
        // {
        //     int execve(Path.c_str(), char *const argv[], char *const envp[]);
        // }

        // ### Functions

        int Received() const
        {

            int Count = 0;
            int Result = ioctl(_INode, FIONREAD, &Count);

            if (Result < 0)
            {
                std::cout << "Received : " << strerror(errno) << std::endl;
                exit(-1);
            }

            return Count;
        }

        size_t Offset() const
        {
            int Result = lseek(_INode, 0, Current);

            if (Result < 0)
            {
                std::cout << "Size : " << strerror(errno) << std::endl;
                exit(-1);
            }

            return static_cast<size_t>(Result);
        }

        size_t Size() const
        {
            auto CurPos = Offset();

            auto Result = lseek(_INode, 0, SEEK_END);

            if (Result < 0)
            {
                std::cout << "Size : " << strerror(errno) << std::endl;
                exit(-1);
            }

            Seek(static_cast<ssize_t>(CurPos));

            return static_cast<size_t>(Result);
        }

        size_t Seek(ssize_t Offset = 0, SeekType Start = SeekType::Start) const
        {
            auto Result = lseek(_INode, Offset, Start);

            if (Result < 0)
            {
                std::cout << "Size : " << strerror(errno) << std::endl;
                exit(-1);
            }

            return static_cast<size_t>(Result);
        }

        void Close() const
        {
            int Result = close(_INode);

            if (Result < 0)
            {
                std::cout << "Close : " << strerror(errno) << std::endl;
                exit(-1);
            }
        }

        // ### Operators

        File &operator<<(const std::string &Message)
        {
            int Result = 0;
            int Left = Message.length();
            const char *str = Message.c_str();

            while (Left > 0)
            {
                // Need to check for unblocking too

                Result = write(_INode, str, Left);
                Left -= Result;
            }

            // Error handling here

            if (Result < 0)
            {
                std::cout << "operator<< : " << strerror(errno) << std::endl;
                exit(-1);
            }

            return *this;
        }

        File &operator>>(std::string &Message)
        {
            size_t Size = Received() + 1;

            if (Size <= 0)
                return *this;

            char buffer[Size];

            int Result = read(_INode, buffer, Size);

            // Instead, can increase string size by
            // avalable bytes in socket buffer
            // and call read on c_str at the new memory index

            if (Result < 0 && errno != EAGAIN)
            {
                std::cout << "operator>> " << errno << " : " << strerror(errno) << std::endl;
                exit(-1);
            }

            buffer[Result] = 0;
            Message.append(buffer);

            return *this;
        }

        const File &operator>>(std::string &Message) const
        {
            size_t Size = Received() + 1;

            if (Size <= 0)
                return *this;

            char buffer[Size];

            int Result = read(_INode, buffer, Size);

            // Instead, can increase string size by
            // avalable bytes in socket buffer
            // and call read on c_str at the new memory index

            if (Result < 0 && errno != EAGAIN)
            {
                std::cout << "operator>> " << errno << " : " << strerror(errno) << std::endl;
                exit(-1);
            }

            buffer[Result] = 0;
            Message.append(buffer);

            return *this;
        }

        friend std::ostream &operator<<(std::ostream &os, const File &file)
        {
            std::string str;
            file >> str; // fix this
            return os << str;
        }
    };
}