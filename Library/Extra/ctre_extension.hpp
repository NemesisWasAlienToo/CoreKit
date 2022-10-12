#pragma once
#include <iostream>
#include <Extra/compile-time-regular-expressions/single-header/ctre.hpp>

template <ctll::fixed_string... Strings>
constexpr auto Concatenate() noexcept
{
    char32_t Data[(Strings.size() + ...) + 1] = {0};

    size_t Index = 0;

    ([&](auto &Item) mutable
     {
         for (size_t i = 0; i < Item.size(); i++)
         {
             Data[Index++] = Item[i];
         } }(Strings),
     ...);

    return ctll::fixed_string<(Strings.size() + ...)>(Data);
}

template <size_t Index, ctll::fixed_string String>
constexpr auto Split() noexcept
{
    char32_t First[Index + 1];
    char32_t Second[String.size() - Index + 1];

    size_t Counter = 0;

    for (size_t i = 0; i < (sizeof(First) / sizeof(char32_t)) - 1; i++)
    {
        First[i] = String[Counter++];
    }

    for (size_t i = 0; i < (sizeof(Second) / sizeof(char32_t)) - 1; i++)
    {
        Second[i] = String[Counter++];
    }

    return std::make_tuple(ctll::fixed_string(First), ctll::fixed_string(Second));
}

template <ctll::fixed_string Original, ctll::fixed_string Text>
constexpr auto Find(size_t Index)
{
    for (size_t i = Index; i < Original.size() - Text.size() + 1; i++)
    {
        if (Original[i] != Text[0])
            continue;

        bool found = true;

        for (size_t j = 1; j < Text.size(); j++)
        {
            if (Original[i + j] != Text[j])
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            return i;
        }
    }

    return static_cast<size_t>(-1);
}

// Make Length use a struct

template <ctll::fixed_string Original, ctll::fixed_string Phrase>
constexpr size_t FindCount()
{
    if (!Phrase.size() || !Original.size())
        return 0;

    size_t Count = 0;
    size_t Index = 0;

    while ((Index = Find<Original, Phrase>(Index)) != static_cast<size_t>(-1))
    {
        Count++;
        Index += Phrase.size();
    }

    return Count;
}

template <ctll::fixed_string Original, ctll::fixed_string Phrase, ctll::fixed_string Text>
constexpr auto Replace(size_t Index)
{
    constexpr size_t DataSize = []()
    {
        if (Phrase.size() == Text.size())
            return Original.size();
        
        size_t Count = FindCount<Original, Phrase>();

        if (Text.size() > Phrase.size())
        {
            return Original.size() + Count * (Text.size() - Phrase.size());
        }
        else
        {
            return Original.size() - Count * (Phrase.size() - Text.size());
        }
    }();

    char32_t Data[DataSize + 1];

    if (!Original.size() || !Phrase.size())
        return ctll::fixed_string(Data);

    size_t OCounter = 0, RCounter = 0;

    while ((Index = Find<Original, Phrase>(RCounter)) != static_cast<size_t>(-1))
    {
        for (; RCounter < Index; RCounter++)
        {
            Data[OCounter++] = Original[RCounter];
        }

        for (size_t i = 0; i < Text.size(); i++)
        {
            Data[OCounter++] = Text[i];
        }

        RCounter += Phrase.size();
    }

    for (; RCounter < Original.size(); RCounter++)
    {
        Data[OCounter++] = Original[RCounter];
    }

    return ctll::fixed_string(Data);
}

// template <size_t L, size_t N, size_t M>
// constexpr auto Replace(ctll::fixed_string<L> Original, ctll::fixed_string<N> Phrase, ctll::fixed_string<M> Text, size_t Index)
// {
//     char32_t Data[ReplacedSize<Original.size(), FindCount<Original, Phrase>(), Phrase.size(), Text.size()>() + 1];
//     size_t OCounter = 0, RCounter = 0;

//     while ((Index = Find(Original, Phrase, Index + 1)) != static_cast<size_t>(-1))
//     {
//         for (; RCounter < Index; RCounter++)
//         {
//             Data[OCounter++] = Original[RCounter];
//         }

//         for (size_t i = 0; i < M; i++)
//         {
//             Data[OCounter++] = Text[i];
//         }

//         RCounter += N;
//     }

//     for (; RCounter < L; RCounter++)
//     {
//         Data[OCounter++] = Original[RCounter];
//     }

//     return ctll::fixed_string(Data);
// }

template <size_t Size>
void Print(ctll::fixed_string<Size> const &String)
{
    for (auto const &Element : String)
    {
        std::cout << static_cast<char>(Element);
    }

    std::cout << '\n';
}