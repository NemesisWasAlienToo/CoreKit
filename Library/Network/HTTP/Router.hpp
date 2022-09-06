#pragma once

#include <tuple>
#include <functional>

#include <Extra/ctre_extension.hpp>
#include <Iterable/List.hpp>

using namespace Core;

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
    using Arguments = typename Repeater<Count + Group, std::string_view>::Tuple;

    template <typename TCallback, size_t... S, typename... TArgs, typename... TupleTArgs>
    static constexpr auto _Apply(std::string_view Path, std::integer_sequence<size_t, S...>, std::tuple<TupleTArgs...>, TCallback &&Callback, TArgs &&...Args)
    {
        if (auto m = Match(Path))
        {
            std::invoke(Callback, std::forward<TArgs>(Args)..., m.template get<S + 1>()...);
            return true;
        }

        return false;
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
    static constexpr inline auto Apply(std::string_view Path, TCallback Callback, TArgs &&...Args)
    {
        return _Apply(Path, Sequence{}, Arguments{}, Callback, std::forward<TArgs>(Args)...);
    }
};

template <typename>
struct Router
{
};

template <typename... TArgs>
class Router<void(TArgs...)>
{
protected:
    using TDefault = std::function<void(TArgs...)>;
    using TMatcher = std::function<bool(std::string_view, TArgs...)>;

    Iterable::List<std::tuple<Network::HTTP::Methods, TMatcher>> Routes;

public:
    TDefault Default;

    Router() = default;

    template <typename TCallback>
    Router(TCallback &&Callback) : Default(std::forward<TCallback>(Callback)) {}

    // Had to redefine args to enable universal reference

    template <typename... RTArgs>
    void Match(std::string_view Path, Network::HTTP::Methods Method, RTArgs &&...Args) const
    {
        for (size_t i = 0; i < Routes.Length(); i++)
        {
            auto RouteMethod = std::get<0>(Routes[i]);
            auto &Route = std::get<1>(Routes[i]);

            if ((RouteMethod == Network::HTTP::Methods::Any || RouteMethod == Method) && Route(Path, std::forward<RTArgs>(Args)...))
            {
                return;
            }
        }

        Default(Args...);
    }

    template <ctll::fixed_string TSignature, bool Group = false, typename TCallback>
    void Add(Network::HTTP::Methods Method, TCallback &&Callback)
    {
        using T = Route<TSignature, Group>;

        Routes.Add(
            Method,
            [CB = std::forward<TCallback>(Callback)](std::string_view Path, TArgs &&...Args)
            {
                return T::Apply(Path, CB, std::forward<TArgs>(Args)...);
            });
    }
};