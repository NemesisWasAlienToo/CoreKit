#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>

#include <Descriptor.hpp>

namespace Core
{
    class File : public Descriptor
    {
    public:
        enum TestType
        {
            CanRead = R_OK,
            CanWrite = W_OK,
            CanExecute = X_OK,
        };

        enum PermissionType
        {
            Sticky = S_ISVTX,

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

            EveryoneAll = OwnerAll | GroupAll | OtherAll,

            DefaultPermission = OwnerWrite | OwnerRead | GroupRead | GroupWrite | OtherRead,
        };

        enum FlagType
        {
            ReadOnly = O_RDONLY,
            WriteOnly = O_WRONLY,
            ReadWrite = O_RDWR,
            CreateFile = O_CREAT,
            Append = O_APPEND,
            CloseOnExec = O_CLOEXEC,
            NonBlocking = O_NONBLOCK,
            Directory = O_DIRECTORY,
#ifdef O_BINARY
            Binary = O_BINARY,
#endif
#ifdef O_TEXT
            Text = O_TEXT,
#endif
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

        File(int INode) : Descriptor(INode) {}

        File(File &&Other) noexcept : Descriptor(std::move(Other)) {}

        // ### Static functions

        static inline bool Exist(const std::string &Name, int Tests = F_OK)
        {
            return (access(Name.c_str(), Tests) != -1);
        }

        static File Create(const std::string &Path, uint32_t Permissions = DefaultPermission)
        {
            int Result = creat(Path.c_str(), Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static File Open(const std::string &Path, int Flags = 0, uint32_t Permissions = DefaultPermission)
        {
            int Result = open(Path.c_str(), Flags, Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static void Remove(const std::string &Path)
        {
            int Result = remove(Path.c_str());

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static std::string ReadAll(const std::string &Path)
        {
            auto file = Open(Path, ReadOnly);

            size_t size = file.Size();
            size_t len = 0;

            char buffer[size + 1];

            buffer[size] = 0;

            while (len < size)
            {
                len += file.Read(&(buffer[len]), (size - len));
            }

            file.Close();

            return buffer;
        }

        static void WriteAll(const std::string &Path, const std::string &Content, bool Create = true, uint32_t Permissions = DefaultPermission)
        {
            int flags = WriteOnly;

            if (Create)
                flags |= CreateFile;

            auto file = Open(Path, flags, Permissions);

            const char *buffer = Content.c_str();
            size_t size = Content.length();
            size_t len = 0;

            while (len < size)
            {
                len += file.Write(&(buffer[len]), (size - len));
            }

            file.Close();
        }

        static void AppendAll(const std::string &Path, const std::string &Content)
        {
            auto file = Open(Path, WriteOnly | Append);

            const char *buffer = Content.c_str();
            size_t size = Content.length();
            size_t len = 0;

            while (len < size)
            {
                len += file.Write(&(buffer[len]), (size - len));
            }

            file.Close();
        }

        // ### Functions

        size_t Offset() const
        {
            int Result = lseek(_INode, 0, Current);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return static_cast<size_t>(Result);
        }

        size_t Size() const
        {
            auto CurPos = Offset();

            auto Result = lseek(_INode, 0, SEEK_END);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            Seek(static_cast<ssize_t>(CurPos));

            return static_cast<size_t>(Result);
        }

        size_t Seek(ssize_t Offset = 0, SeekType Start = SeekType::Start) const
        {
            auto Result = lseek(_INode, Offset, Start);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return static_cast<size_t>(Result);
        }

        void WriteLine(const std::string &Message)
        {
            *this << Message << '\n';
        }

        std::string ReadLine()
        {
            // @todo Fix and optimize this maybe with string stream?
            char c;
            size_t RRead;
            std::string Res;

            while ((RRead = Read(&c, 1)) > 0)
            {
                if (c == '\n')
                {
                    break;
                }

                Res += c;
            }

            return Res;
        }

        // ### Operators

        File &operator=(File &&Other) noexcept
        {
            Descriptor::operator=(std::move(Other));

            return *this;
        }

        File &operator=(File const &Other) = delete;

        File &operator<<(const char Message)
        {
            int Result = write(_INode, &Message, 1);

            // Error handling here

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return *this;
        }

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
                throw std::system_error(errno, std::generic_category());
            }

            return *this;
        }

        // @todo Remove these

        File &operator>>(std::string &Message)
        {
            int Result = 0;
            size_t Size = 128;

            Message.resize(0);

            do
            {
                char buffer[Size];

                Result = read(_INode, buffer, Size);

                if (Result <= 0)
                    break;

                Message.append(buffer, Result);

            } while ((Size = Received()) > 0);

            if (Result < 0 && errno != EAGAIN)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return *this;
        }

        const File &operator>>(std::string &Message) const
        {
            int Result = 0;
            size_t Size = 128;

            Message.resize(0);

            do
            {
                char buffer[Size];

                Result = read(_INode, buffer, Size);

                if (Result <= 0)
                    break;

                Message.append(buffer, Result);

            } while ((Size = Received()) > 0);

            if (Result < 0 && errno != EAGAIN)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return *this;
        }

        friend std::ostream &operator<<(std::ostream &os, const File &file)
        {
            std::string str;
            file >> str; // fix this
            return os << str;
        }
    };

    File STDIN = STDIN_FILENO;
    File STDOUT = STDOUT_FILENO;
    File STDERR = STDERR_FILENO;
}