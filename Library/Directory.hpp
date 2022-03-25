#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <system_error>

#include <Descriptor.hpp>
#include <Iterable/List.hpp>

namespace Core
{
    class Directory : public Descriptor
    {
    private:
        struct EntryHelper
        {
            unsigned long INode;
            off_t Offset;
            unsigned short Length;
            char Name[];
        };

    public:
        // # Types

        struct Entry
        {
            // # Types

            enum Types
            {
                Unknown = DT_UNKNOWN,
                FIFO = DT_FIFO,
                Char = DT_CHR,
                Directory = DT_DIR,
                Block = DT_BLK,
                Regular = DT_REG,
                Link = DT_LNK,
                Socket = DT_SOCK,
                WhiteOut = DT_WHT,
            };

            // # Variables

            unsigned long INode;
            std::string Name;
            uint8_t Type;
            off_t Offset;
            unsigned short Length;

            // # Constructor

            Entry() = default;

            Entry(unsigned long _INode, std::string _Name, uint8_t _Type, off_t _Offset, unsigned short _Length) : INode(_INode), Name(_Name), Type(_Type), Offset(_Offset), Length(_Length) {}

            Entry(Entry &&Other) : INode(Other.INode), Name(std::move(Other.Name)), Type(Other.Type), Offset(Other.Offset), Length(Other.Length) {}

            Entry(const Entry &Other) : INode(Other.INode), Name(Other.Name), Type(Other.Type), Offset(Other.Offset), Length(Other.Length) {}

            // Static functions

            inline std::string TypeName() const
            {
                switch (Type)
                {
                case Types::Unknown:
                    return "Unknown";
                    break;
                case Types::FIFO:
                    return "FIFO";
                    break;
                case Types::Char:
                    return "Char Device";
                    break;
                case Types::Directory:
                    return "Directory";
                    break;
                case Types::Block:
                    return "Block Device";
                    break;
                case Types::Regular:
                    return "Regular File";
                    break;
                case Types::Link:
                    return "Symbolic Link";
                    break;
                case Types::Socket:
                    return "Socket";
                    break;
                case Types::WhiteOut:
                    return "Whiteout";
                    break;
                default:
                    return "";
                    break;
                }
            }

            // Move string

            Entry &operator=(Entry &&Other)
            {
                INode = Other.INode;
                Name = std::move(Other.Name);
                Type = Other.Type;
                Offset = Other.Offset;
                Length = Other.Length;
                return *this;
            }

            Entry &operator=(const Entry &Other)
            {
                INode = Other.INode;
                Name = Other.Name;
                Type = Other.Type;
                Offset = Other.Offset;
                Length = Other.Length;
                return *this;
            }
        };

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

            DefaultPermission = OwnerWrite | OwnerRead | OwnerExecute | GroupRead | GroupWrite | GroupExecute | OtherRead,
        };

        // ### Constructors

        Directory() = default;

        Directory(int INode) : Descriptor(INode) {}

        Directory(const Directory &Other) : Descriptor(Other) {}

        // ### Static functions

        static inline bool Exist(const std::string &Name, int Tests = F_OK)
        {
            return (access(Name.c_str(), Tests) != -1);
        }

        static void Create(const std::string &Path, uint32_t Permissions = DefaultPermission)
        {
            int Result = mkdir(Path.c_str(), Permissions);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static void Remove(const std::string &Path)
        {
            int Result = rmdir(Path.c_str());

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static void ChangeDirectory(const std::string &Path)
        {
            int Result = chdir(Path.c_str());

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static void ChangeDirectory(int _INode)
        {
            int Result = fchdir(_INode);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        static Iterable::List<Entry> List(size_t Buffersize = 1024)
        {
            auto dir = Directory::Open(".");

            auto _ls = dir.Entries(Buffersize);

            dir.Close();

            return _ls;
        }

        static Directory Open(const std::string &Path)
        {
            int Result = open(Path.c_str(), O_RDONLY | O_DIRECTORY, 0);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        // # Functions

        size_t Seek(ssize_t Offset = 0) const
        {
            auto Result = lseek(_INode, Offset, 0);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return static_cast<size_t>(Result);
        }

        Iterable::List<Entry> Entries(size_t BufferSize = 1024) const
        {
            char Buffer[BufferSize];
            auto Result = 0;
            Iterable::List<Entry> entries;

            Seek();

            while ((Result = syscall(SYS_getdents, _INode, Buffer, BufferSize)) > 0)
            {
                for (long bpos = 0; bpos < Result;)
                {
                    EntryHelper *helper = (EntryHelper *)(Buffer + bpos);
                    uint8_t type = *(Buffer + bpos + helper->Length - 1);
                    bpos += helper->Length;

                    Entry entry(helper->INode, helper->Name, type, helper->Offset, helper->Length);

                    entries.Add(std::move(entry));
                }
            }

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return entries;
        }
    };
}