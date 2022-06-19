#pragma once

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <system_error>
#include <string_view>

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

        static inline bool Exist(std::string_view const &Name, int Tests = F_OK)
        {
            return (access(Name.begin(), Tests) != -1);
        }

        static File Create(std::string_view const &Path, uint32_t Permissions = DefaultPermission)
        {
            int Result = creat(Path.begin(), Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static File Open(std::string_view const &Path, int Flags = 0, uint32_t Permissions = DefaultPermission)
        {
            int Result = open(Path.begin(), Flags, Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static void Remove(std::string_view const &Path)
        {
            int Result = remove(Path.begin());

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        std::string ReadAll()
        {
            std::string buffer;
            buffer.resize(Size());

            size_t len = 0;

            while (len < buffer.length())
            {
                len += Read(&(buffer[len]), (buffer.length() - len));
            }

            return buffer;
        }

        std::string ReadAllString()
        {
            std::string buffer;
            buffer.resize(Size());

            size_t len = 0;

            while (len < buffer.length())
            {
                len += Read(&(buffer[len]), (buffer.length() - len));
            }

            return buffer;
        }

        static Iterable::Span<char> ReadAll(std::string_view const &Path)
        {
            auto file = Open(Path, ReadOnly);

            Iterable::Span<char> buffer = file.Size();

            size_t len = 0;

            while (len < buffer.Length())
            {
                len += file.Read(&(buffer[len]), (buffer.Length() - len));
            }

            file.Close();

            return buffer;
        }

        static std::string ReadAllString(std::string_view const &Path)
        {
            auto file = Open(Path, ReadOnly);

            std::string buffer;
            buffer.resize(file.Size());

            size_t len = 0;

            while (len < buffer.length())
            {
                len += file.Read(&(buffer[len]), (buffer.length() - len));
            }

            file.Close();

            return buffer;
        }

        static void WriteAll(std::string_view const &Path, std::string_view const &Content, bool Create = true, uint32_t Permissions = DefaultPermission)
        {
            int flags = WriteOnly;

            if (Create)
                flags |= CreateFile;

            auto file = Open(Path, flags, Permissions);

            size_t size = Content.length();
            size_t len = 0;

            while (len < size)
            {
                len += file.Write(&(Content.begin()[len]), (size - len));
            }

            file.Close();
        }

        static void AppendAll(std::string_view const &Path, std::string_view const &Content)
        {
            auto file = Open(Path, WriteOnly | Append);

            size_t size = Content.length();
            size_t len = 0;

            while (len < size)
            {
                len += file.Write(&(Content.begin()[len]), (size - len));
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

        static struct stat Stat(std::string_view const &Path)
        {
            struct stat st;

            int Result = stat(Path.begin(), &st);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return st;
        }

        inline static bool IsDirectory(std::string_view const &Path)
        {
            return S_ISDIR(Stat(Path).st_mode);
        }

        inline static bool IsChar(std::string_view const &Path)
        {
            return S_ISCHR(Stat(Path).st_mode);
        }

        inline static bool IsBulk(std::string_view const &Path)
        {
            return S_ISBLK(Stat(Path).st_mode);
        }

        inline static bool IsFIFO(std::string_view const &Path)
        {
            return S_ISFIFO(Stat(Path).st_mode);
        }

        inline static bool IsLink(std::string_view const &Path)
        {
            return S_ISLNK(Stat(Path).st_mode);
        }

        inline static bool IsSocket(std::string_view const &Path)
        {
            return S_ISSOCK(Stat(Path).st_mode);
        }

        inline static bool IsRegular(std::string_view const &Path)
        {
            return S_ISREG(Stat(Path).st_mode);
        }

        inline static size_t SizeOf(std::string_view const &Path)
        {
            return Stat(Path).st_size;
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

        void WriteLine(std::string_view const &Message)
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

        // @todo Remove these operators

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

        File &operator<<(std::string_view const &Message)
        {
            int Result = 0;
            int Length = Message.length();
            int Sent = 0;
            const char *str = Message.begin();

            while (Sent < Length)
            {
                // Need to check for unblocking too

                Result = write(_INode, str + Sent, Length - Sent);
                Sent += Result;
            }

            // Error handling here

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return *this;
        }

        operator bool() const
        {
            return _INode != -1;
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

        friend std::ostream &operator<<(std::ostream &os, File const &file)
        {
            std::string str;
            file >> str; // fix this
            return os << str;
        }
    };

    inline File STDIN = STDIN_FILENO;
    inline File STDOUT = STDOUT_FILENO;
    inline File STDERR = STDERR_FILENO;
}