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
#else
            Binary = 4,
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

        static inline bool Exist(std::string const &Name, int Tests = F_OK)
        {
            return (access(Name.c_str(), Tests) != -1);
        }

        static File Create(std::string const &Path, uint32_t Permissions = DefaultPermission)
        {
            int Result = creat(Path.c_str(), Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static std::string_view GetExtension(std::string_view Path)
        {
            auto pos = Path.find_last_of('.');
            if (pos == std::string_view::npos)
                return "";

            return Path.substr(pos + 1);
        }

        // enum LockFlags
        // {
        //     Exclusive = LOCK_EX,
        //     Shared = LOCK_SH,
        //     Read = LOCK_READ,
        //     Write = LOCK_WRITE,
        //     NonBlocking = LOCK_NB,
        //     Release = LOCK_UN,
        // };

        // static bool Lock(File const &file, int Flags)
        // {
        //     int Result = ::flock(file._INode, Flags);

        //     if (Result < 0)
        //     {
        //         throw std::system_error(errno, std::generic_category());
        //     }
        // }

        // static bool Unlock(File const &file)
        // {
        //     int Result = flock(file._INode, Release);

        //     if (Result < 0)
        //     {
        //         throw std::system_error(errno, std::generic_category());
        //     }
        // }

        static void Unlink(std::string &Path)
        {
            int Result = unlink(Path.c_str());

            if (Result == -1)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static File MakeTemp(std::string &Path)
        {
            if (!Path.ends_with("XXXXXX"))
                throw std::invalid_argument("Temp file name must terminate with XXXXXX");

            int Result = mkstemp(&Path[0]);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static File Open(std::string const &Path, int Flags = 0, uint32_t Permissions = DefaultPermission)
        {
            int Result = open(Path.c_str(), Flags, Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        static void Rename(std::string const &Old, std::string const &New)
        {
            int Result = rename(Old.c_str(), New.c_str());

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static void Remove(std::string const &Path)
        {
            int Result = remove(Path.c_str());

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

        // static Iterable::Span<char> ReadAll(std::string const &Path)
        // {
        //     auto file = Open(Path, ReadOnly);

        //     Iterable::Span<char> buffer = file.Size();

        //     size_t len = 0;

        //     while (len < buffer.Length())
        //     {
        //         len += file.Read(&(buffer[len]), (buffer.Length() - len));
        //     }

        //     file.Close();

        //     return buffer;
        // }

        static std::string ReadAll(std::string const &Path)
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

        static void WriteAll(std::string const &Path, std::string_view Content, bool Create = true, uint32_t Permissions = DefaultPermission)
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

        static void AppendAll(std::string const &Path, std::string_view Content)
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

        static struct stat Stat(std::string const &Path)
        {
            struct stat st;

            int Result = stat(Path.c_str(), &st);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return st;
        }

        inline static bool IsDirectory(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISDIR(st.st_mode);
        }

        inline static bool IsChar(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISCHR(st.st_mode);
        }

        inline static bool IsBulk(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISBLK(st.st_mode);
        }

        inline static bool IsFIFO(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISFIFO(st.st_mode);
        }

        inline static bool IsLink(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISLNK(st.st_mode);
        }

        inline static bool IsSocket(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISSOCK(st.st_mode);
        }

        inline static bool IsRegular(std::string const &Path)
        {
            struct stat st;

            if(stat(Path.c_str(), &st) < 0)
                return false;

            return S_ISREG(st.st_mode);
        }

        inline static size_t SizeOf(std::string const &Path)
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

        // void WriteLine(std::string_view Message)
        // {
        //     *this << Message << '\n';
        // }

        // std::string ReadLine()
        // {
        //     // @todo Fix and optimize this maybe with string stream?
        //     char c;
        //     size_t RRead;
        //     std::string Res;

        //     while ((RRead = Read(&c, 1)) > 0)
        //     {
        //         if (c == '\n')
        //         {
        //             break;
        //         }

        //         Res += c;
        //     }

        //     return Res;
        // }

        // ### Operators

        File &operator=(File &&Other) noexcept
        {
            Descriptor::operator=(std::move(Other));

            return *this;
        }

        File &operator=(File const &Other) = delete;

        // @todo Remove these operators

        // File &operator<<(const char Message)
        // {
        //     int Result = write(_INode, &Message, 1);

        //     // Error handling here

        //     if (Result < 0)
        //     {
        //         throw std::system_error(errno, std::generic_category());
        //     }

        //     return *this;
        // }

        // File &operator<<(std::string_view Message)
        // {
        //     int Result = 0;
        //     int Length = Message.length();
        //     int Sent = 0;
        //     const char *str = Message.begin();

        //     while (Sent < Length)
        //     {
        //         // Need to check for unblocking too

        //         Result = write(_INode, str + Sent, Length - Sent);
        //         Sent += Result;
        //     }

        //     // Error handling here

        //     if (Result < 0)
        //     {
        //         throw std::system_error(errno, std::generic_category());
        //     }

        //     return *this;
        // }

        // @todo Remove these

        // File &operator>>(std::string &Message)
        // {
        //     int Result = 0;
        //     size_t Size = 128;

        //     Message.resize(0);

        //     do
        //     {
        //         char buffer[Size];

        //         Result = read(_INode, buffer, Size);

        //         if (Result <= 0)
        //             break;

        //         Message.append(buffer, Result);

        //     } while ((Size = Received()) > 0);

        //     if (Result < 0 && errno != EAGAIN)
        //     {
        //         throw std::system_error(errno, std::generic_category());
        //     }

        //     return *this;
        // }

        // const File &operator>>(std::string &Message) const
        // {
        //     int Result = 0;
        //     size_t Size = 128;

        //     Message.resize(0);

        //     do
        //     {
        //         char buffer[Size];

        //         Result = read(_INode, buffer, Size);

        //         if (Result <= 0)
        //             break;

        //         Message.append(buffer, Result);

        //     } while ((Size = Received()) > 0);

        //     if (Result < 0 && errno != EAGAIN)
        //     {
        //         throw std::system_error(errno, std::generic_category());
        //     }

        //     return *this;
        // }

        // friend std::ostream &operator<<(std::ostream &os, File const &file)
        // {
        //     std::string str;
        //     file >> str; // fix this
        //     return os << str;
        // }
    };

    inline File STDIN = STDIN_FILENO;
    inline File STDOUT = STDOUT_FILENO;
    inline File STDERR = STDERR_FILENO;
}