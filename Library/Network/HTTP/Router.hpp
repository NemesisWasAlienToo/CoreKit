#pragma once

#include <tuple>
#include <Function.hpp>
#include <regex>
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

template <ctll::fixed_string Pattern>
struct Route
{
protected:
    constexpr static auto GroupImp()
    {
        return Pattern.size() > 1 && Pattern[Pattern.size() - 2] == '/' && Pattern[Pattern.size() - 1] == '*';
    }

    constexpr static bool Group = GroupImp();
    constexpr static ctll::fixed_string Parameter{"[]"};
    constexpr static ctll::fixed_string Capture{"([^/?]+)"};
    constexpr static size_t Count = FindCount<Pattern, Parameter>();

    using Sequence = typename std::make_index_sequence<Count + Group>;
    using Arguments = typename Repeater<Count + Group, std::string_view>::Tuple;

    template <typename TCallback, size_t... S, typename... TArgs>
    static constexpr auto ApplyImpl(std::string_view Path, std::integer_sequence<size_t, S...>, TCallback &&Callback, TArgs &&...Args)
    {
        // @todo Provide readable compile time error
        
        if (auto m = Match(Path))
        {
            std::invoke(Callback, std::forward<TArgs>(Args)..., m.template get<S + 1>()...);
            return true;
        }

        return false;
    }

    constexpr static auto RegexImpl()
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
    constexpr static auto Regex = RegexImpl();

    static constexpr auto Match(std::string_view Path)
    {
        return ctre::match<Regex>(Path);
    }

    template <typename TCallback, typename... TArgs>
    static constexpr inline auto Apply(std::string_view Path, TCallback Callback, TArgs &&...Args)
    {
        return ApplyImpl(Path, Sequence{}, Callback, std::forward<TArgs>(Args)...);
    }
};

template <ctll::fixed_string Pattern>
struct RegexRoute
{
protected:
    constexpr static auto GroupImp()
    {
        return Pattern.size() > 1 && Pattern[Pattern.size() - 2] == '/' && Pattern[Pattern.size() - 1] == '*';
    }

    constexpr static bool Group = GroupImp();
    constexpr static ctll::fixed_string Parameter{"[]"};
    constexpr static ctll::fixed_string Capture{"([^/?]+)"};
    constexpr static size_t Count = FindCount<Pattern, Parameter>();

    using Sequence = typename std::make_index_sequence<Count + Group>;
    using Arguments = typename Repeater<Count + Group, std::string_view>::Tuple;

    template <typename TCallback, size_t... S, typename... TArgs>
    static constexpr decltype(auto) ApplyImpl(std::smatch &Match, std::integer_sequence<size_t, S...>, TCallback &&Callback, TArgs &&...Args)
    {
        // return std::invoke(Callback, std::forward<TArgs>(Args)..., m.template get<S + 1>()...);
        return std::invoke(Callback, std::forward<TArgs>(Args)..., Match[S + 1]...);
    }

    constexpr static auto RegexImpl()
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
    constexpr static auto Regex = RegexImpl();

    static std::string RegexString()
    {
        std::string Result;
        Result.resize(Regex.size());

        for (size_t i = 0; i < Regex.size(); i++)
        {
            Result[i] = Regex[i];
        }

        return Result;
    }

    template <typename TCallback, typename... TArgs>
    static constexpr inline decltype(auto) Apply(std::smatch &Match, TCallback Callback, TArgs &&...Args)
    {
        return ApplyImpl(Match, Sequence{}, Callback, std::forward<TArgs>(Args)...);
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
    using TDefault = Core::Function<void(TArgs...)>;
    using TMatcher = Core::Function<bool(std::string_view, TArgs...)>;

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

    template <ctll::fixed_string TSignature, typename TCallback>
    void Add(Network::HTTP::Methods Method, TCallback &&Callback)
    {
        using T = Route<TSignature>;

        Routes.Add(
            Method,
            [CB = std::forward<TCallback>(Callback)](std::string_view Path, TArgs &&...Args)
            {
                return T::Apply(Path, CB, std::forward<TArgs>(Args)...);
            });
    }
};

template <typename TRet, typename... TArgs>
class Router<TRet(TArgs...)>
{
protected:
    using THandler = Core::Function<TRet(std::smatch &, TArgs...)>;
    using TDefault = Core::Function<TRet(TArgs...)>;

    struct Branch
    {
        Network::HTTP::Methods Method;
        std::regex Regex;
        THandler Handler;
    };

    Iterable::List<Branch> Routes;

public:
    TDefault Default;

    Router() = default;

    template <typename TCallback>
    Router(TCallback &&Callback) : Default(std::forward<TCallback>(Callback)) {}

    // Had to redefine args to enable universal reference

    template <typename... RTArgs>
    TRet Match(std::string const &Path, Network::HTTP::Methods Method, RTArgs &&...Args)
    {
        for (size_t i = 0; i < Routes.Length(); i++)
        {
            Branch &Item = Routes[i];
            std::smatch Match;

            if ((Item.Method == Network::HTTP::Methods::Any || Item.Method == Method) && std::regex_match(Path, Match, Item.Regex))
            {
                return Item.Handler(Match, std::forward<RTArgs>(Args)...);
            }
        }

        return Default(Args...);
    }

    template <ctll::fixed_string TSignature, typename TCallback>
    void Add(Network::HTTP::Methods Method, TCallback &&Callback)
    {
        using T = RegexRoute<TSignature>;

        Routes.Insert(
            {Method,
             std::regex(T::RegexString()),
             [CB = std::forward<TCallback>(Callback)](std::smatch Match, TArgs &&...Args)
             {
                 return T::Apply(Match, CB, std::forward<TArgs>(Args)...);
             }});
    }
};