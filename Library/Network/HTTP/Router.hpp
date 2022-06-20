#pragma once

#include <tuple>
#include <optional>
#include <functional>

#include <Extra/ctre.hpp>
#include <Extra/ctre_extension.hpp>
#include <Iterable/List.hpp>
#include <Network/HTTP/Request.hpp>
#include <DateTime.hpp>

template <size_t Count, typename T, typename... S>
struct Repeater : Repeater<Count - 1, T, T, S...>
{
};

template <typename T, typename... S>
struct Repeater<0, T, S...>
{
    using Tuple = std::tuple<S...>;
};

template <ctll::fixed_string Pattern, bool Group = false>
struct Route
{
protected:
    constexpr static ctll::fixed_string Parameter{"[]"};
    constexpr static ctll::fixed_string Capture{"([^/?]+)"};
    constexpr static size_t Count = FindCount<Pattern, Parameter>();

    using Sequence = typename std::make_index_sequence<Count + Group>;
    using Arguments = typename Repeater<Count + Group, std::string>::Tuple;

    template <typename TCallback, size_t... S, typename... TArgs, typename... TupleTArgs>
    static constexpr auto _Apply(std::string_view Path, std::integer_sequence<size_t, S...>, std::tuple<TupleTArgs...>, TCallback &&Callback, TArgs &&...Args)
    {
        using TRet = typename std::invoke_result<TCallback, TArgs..., TupleTArgs...>::type;

        if (auto m = Match(Path))
        {
            return std::make_optional(std::invoke(Callback, std::forward<TArgs>(Args)..., m.template get<S + 1>()...));
        }

        return std::optional<TRet>(std::nullopt);
    }
    constexpr static auto _Regex()
    {
        if constexpr (Group)
        {
            return Concatenate<Replace<Pattern, Parameter, Capture>(0), "(?:\\/([^?]*))?(?:\\/|\\?.*)?">();
        }
        else
        {
            return Concatenate<Replace<Pattern, Parameter, Capture>(0), "(?:\\/|\\?.*)?">();
        }
    }

public:
    constexpr static auto Regex = _Regex();

    static constexpr auto Match(std::string_view Path)
    {
        return ctre::match<Regex>(Path);
    }

    template <typename TCallback, typename... TArgs>
    static constexpr inline auto Apply(std::string_view const &Path, TCallback Callback, TArgs &&...Args)
    {
        return _Apply(Path, Sequence{}, Arguments{}, Callback, std::forward<TArgs>(Args)...);
    }
};

using namespace Core;

template <typename>
struct Router
{
};

template <typename TRet, typename... TArgs>
class Router<TRet(TArgs...)>
{
protected:
    using TDefault = std::function<TRet(TArgs...)>;
    using TMatcher = std::function<std::optional<TRet>(std::string_view, TArgs...)>;

    // Iterable::List<TMatcher> Routes;
    Iterable::List<std::tuple<Network::HTTP::Methods, TMatcher>> Routes;

public:
    TDefault Default;

    Router() = default;
    Router(TDefault &&_Default) : Default(std::move(_Default)) {}
    Router(TDefault const &_Default) : Default(_Default) {}

    // Had to redefine args to enable universal refrence

    template <typename... RTArgs>
    TRet Match(Network::HTTP::Methods Method, std::string_view Path, RTArgs &&...Args) const
    {
        for (size_t i = 0; i < Routes.Length(); i++)
        {
            auto &RouteMethod = std::get<0>(Routes[i]);
            auto &Route = std::get<1>(Routes[i]);

            if(RouteMethod == Network::HTTP::Methods::Any || RouteMethod == Method)
            {
                if (auto Result = Route(Path, std::forward<RTArgs>(Args)...))
                {
                    return Result.value();
                }
            }
        }

        return Default(Args...);
    }

    template <ctll::fixed_string TSigniture, bool Group = false, typename TCallback>
    void Add(Network::HTTP::Methods Method, TCallback &&Callback)
    {
        using T = Route<TSigniture, Group>;

        // Routes.Add(
        Routes.Construct(
            Method,
            [CB = std::forward<TCallback>(Callback)](std::string_view Path, TArgs &&...Args)
            {
                return T::Apply(Path, CB, std::forward<TArgs>(Args)...);
            });
    }
};

// Router v2

// template <typename>
// struct Router2
// {
// };

// template <typename TRet, typename... TArgs>
// class Router2<TRet(TArgs...)>
// {
// protected:
//     using TDefault = std::function<TRet(TArgs...)>;
//     using TMatcher = std::function<TRet(std::string_view, Network::HTTP::Methods, TArgs...)>;

//     TMatcher Matcher;

// public:
//     TDefault Default;

//     Router2() = default;
//     Router2(TDefault &&_Default) : Default(std::move(_Default)) {}
//     Router2(TDefault const &_Default) : Default(_Default) {}

//     // Had to redefine args to enable universal refrence

//     template <typename... RTArgs>
//     TRet Match(Network::HTTP::Methods Method, std::string_view Path, RTArgs &&...Args) const
//     {
//         return Matcher(Path, Method, Args...);
//     }

//     template <ctll::fixed_string TSigniture, bool Group = false, typename TCallback>
//     void Add(Network::HTTP::Methods M, TCallback &&Callback)
//     {
//         using T = Route<TSigniture, Group>;

//         Matcher = [this, M, Next = std::move(Matcher), CB = std::forward<TCallback>(Callback)](std::string_view Path, Network::HTTP::Methods Method, TArgs &&...Args)
//         {
//             if (M == Network::HTTP::Methods::Any || (Method == M && (auto Match = ctre::match<T::Regex>(Path))))
//             {
//                 return T::Apply(Match, CB, std::forward<TArgs>(Args)...);
//             }

//             if (Next)
//                 return Next(Path, Method, std::forward<TArgs>(Args)...);
            
//             return Default(std::forward<TArgs>(Args)...);
//         };
//     }
// };