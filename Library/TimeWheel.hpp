#pragma once

#include <string>
#include <functional>
#include <array>
#include <list>

#include <Descriptor.hpp>
#include <Duration.hpp>
#include <DateTime.hpp>
#include <Timer.hpp>
#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>

namespace Core
{
    template <size_t Steps, size_t Stages>
    class TimeWheel
    {
    public:
        struct Wheel;

        struct Entry
        {
            // @todo Maybe execute task in desctructor?

            using TCallback = std::function<void()>;

            TCallback Callback;
            std::array<size_t, Stages> Posision;
            size_t Bucket;
            size_t Wheel;
        };

        struct Bucket
        {
            using Container = std::list<Entry>;
            using Iterator = typename Container::iterator;

            Container Entries;

            Iterator Add(Entry const &entry)
            {
                return Entries.insert(Entries.end(), entry);
            }

            Iterator Add(Entry &&entry)
            {
                return Entries.insert(Entries.end(), std::move(entry));
            }

            void Remove(Iterator entry)
            {
                Entries.erase(entry);
            }

            void Execute()
            {
                for (auto &entry : Entries)
                {
                    // @todo Check whether entry is valid
                    if (entry.Callback)
                        entry.Callback();
                }

                Entries.clear();
            }

            void Cascade(Wheel &Destination, size_t Stage)
            {
                // @todo Maybe user pointer to entry to make move faster?

                while (!Entries.empty())
                {
                    auto Iterator = Entries.begin();

                    // @todo Test this

                    Iterator->Wheel = Stage;

                    size_t &Index = Iterator->Posision[Stage];

                    auto &DestinationBucket = Destination.Buckets[Index].Entries;

                    DestinationBucket.splice(DestinationBucket.end(), Entries, Iterator);
                }

                // bucket.clear();
            }
        };

        struct Wheel
        {
            std::array<Bucket, Steps> Buckets;
            size_t Interval;
        };

        TimeWheel() = default;
        TimeWheel(TimeWheel const &Other) = delete;
        TimeWheel(TimeWheel &&Other) noexcept : Wheels(std::move(Other.Wheels)), Indices(std::move(Other.Indices)), IntervalMS(Other.IntervalMS) {}

        TimeWheel(Duration const &Interval) : IntervalMS(Interval.AsMilliseconds())
        {
            size_t TempInterval = 1;

            for (size_t i = 0; i < Wheels.size(); i++)
            {
                Indices[i] = 0;

                Wheels[i].Interval = TempInterval;
                TempInterval *= Steps;
            }
        }

        TimeWheel &operator=(TimeWheel const &Other) = delete;

        TimeWheel &operator=(TimeWheel &&Other) noexcept
        {
            Wheels = std::move(Other.Wheels);
            Indices = std::move(Other.Indices);
            IntervalMS = std::move(Other.IntervalMS);

            return *this;
        }

        inline void Interval(Duration const &Interval)
        {
            IntervalMS = Interval.AsMilliseconds();
        }

        inline Duration Interval() const
        {
            return Duration::FromMilliseconds(IntervalMS);
        }

        static constexpr size_t MaxSteps()
        {
            size_t Counter = Stages;
            size_t TotalSteps = 1;

            while (Counter)
            {
                TotalSteps *= Steps;
                Counter--;
            }

            return TotalSteps;
        }

        Duration MaxDuration()
        {
            return Duration::FromMilliseconds(MaxSteps() * IntervalMS);
        }

        inline void Tick()
        {
            Increment();

            Current().Execute();
        }

        typename Bucket::Iterator Add(size_t _Steps, typename Entry::TCallback callback)
        {
            Entry entry{std::move(callback), Offset(_Steps), 0, 0};

            size_t Level = 0;

            for (Level = Wheels.size() - 1; Level > 0 && entry.Posision[Level] == 0; --Level)
            {
            }

            entry.Wheel = Level;
            entry.Bucket = (entry.Posision[Level] + Indices[Level]) % Wheels[Level].Buckets.size();

            return At(entry.Wheel, entry.Bucket).Add(std::move(entry));
        }

        typename Bucket::Iterator Add(Duration const &Interval, typename Entry::TCallback callback)
        {
            return Add(Interval.AsMilliseconds() / IntervalMS, std::move(callback));
        }

        inline void Remove(typename Bucket::Iterator Iterator)
        {
            if (Iterator == end())
                return;

            At(Iterator->Wheel, Iterator->Bucket).Remove(Iterator);
        }

        inline Bucket &At(size_t _Wheel, size_t _Bucket)
        {
            return Wheels[_Wheel].Buckets[_Bucket];
        }

        inline Bucket &Current(size_t Stage = 0)
        {
            return At(Stage, Indices[Stage]);
        }

        inline auto end()
        {
            return At(0, 0).Entries.end();
        }

    private:
        std::array<size_t, Stages> Offset(size_t _Steps)
        {
            std::array<size_t, Stages> Result;

            size_t Level = Wheels.size() - 1;

            for (; _Steps && Level < Wheels.size(); --Level)
            {
                auto &_Interval = Wheels[Level].Interval;

                Result[Level] = _Steps / _Interval;
                _Steps %= _Interval;
            }

            for (; Level < Wheels.size(); --Level)
            {
                Result[Level] = 0;
            }

            return Result;
        }

        void Increment(size_t Level = 0)
        {
            size_t &_Index = Indices[Level];
            Wheel &_Wheel = Wheels[Level];

            if (++_Index >= _Wheel.Buckets.size())
            {
                _Index = 0;

                if (Level >= (Wheels.size() - 1))
                    return;

                Increment(++Level);

                Current(Level).Cascade(_Wheel, Level - 1);
            }
        }

        std::array<Wheel, Stages> Wheels;
        std::array<size_t, Stages> Indices;
        size_t IntervalMS;
    };
}

/*#pragma once

#include <string>
#include <functional>
#include <array>
#include <list>

#include <Descriptor.hpp>
#include <Duration.hpp>
#include <DateTime.hpp>
#include <Timer.hpp>
#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>

namespace Core
{
    template <size_t Steps, size_t Stages>
    class TimeWheel
    {
    public:
        struct Wheel;

        struct Entry
        {
            // @todo Maybe execute task in desctructor?

            using TCallback = std::function<void()>;

            TCallback Callback;
            size_t Posision[Stages];
            size_t Bucket;
            size_t Wheel;
        };

        struct Bucket
        {
            using Container = std::list<Entry>;
            using Iterator = Container::iterator;

            Container Entries;

            Iterator Add(Entry const &entry)
            {
                return Entries.insert(Entries.end(), entry);
            }

            Iterator Add(Entry &&entry)
            {
                return Entries.insert(Entries.end(), std::move(entry));
            }

            void Remove(Iterator entry)
            {
                Entries.erase(entry);
            }

            void Execute()
            {
                for (auto &entry : Entries)
                {
                    entry.Callback();
                }

                Entries.clear();
            }

            void Cascade(Wheel &Destination, size_t Stage)
            {
                // @todo Maybe user pointer to entry to make move faster?

                while (!Entries.empty())
                {
                    auto Iterator = Entries.begin();

                    // @todo Test this

                    Iterator->Wheel = Stage;

                    size_t &Index = Iterator->Posision[Stage];

                    auto &DestinationBucket = Destination.Buckets[Index].Entries;

                    DestinationBucket.splice(DestinationBucket.end(), Entries, Iterator);
                }

                // bucket.clear();
            }
        };

        struct Wheel
        {
            Bucket Buckets[Steps];
            size_t Interval;
        };

        TimeWheel() = default;
        TimeWheel(TimeWheel const &Other) = delete;
        TimeWheel(TimeWheel &&Other) noexcept = default;

        TimeWheel(Duration const &Interval) : IntervalMS(Interval.AsMilliseconds())
        {
            size_t TempInterval = 1;

            for (size_t i = 0; i < Stages; i++)
            {
                Indices[i] = 0;

                Wheels[i].Interval = TempInterval;
                TempInterval *= Steps;
            }
        }

        TimeWheel &operator=(TimeWheel const &Other) = delete;
        TimeWheel &operator=(TimeWheel &&Other) noexcept = default;

        inline void Interval(Duration const &Interval)
        {
            IntervalMS = Interval.AsMilliseconds();
        }

        inline Duration Interval() const
        {
            return Duration::FromMilliseconds(IntervalMS);
        }

        // constexpr size_t MaxSteps()  const noexcept
        // {
        //     size_t Counter = Steps;

        // }

        inline void Tick()
        {
            Increment();

            Current().Execute();
        }

        Bucket::Iterator Add(size_t _Steps, Entry::TCallback callback)
        {
            // Entry entry{std::move(callback), Offset(_Steps), 0, 0};
            Entry entry{std::move(callback), {0}, 0, 0};

            Offset(entry.Posision, _Steps);

            size_t Level = 0;

            for (Level = Stages - 1; Level > 0 && entry.Posision[Level] == 0; --Level)
            {
            }

            entry.Wheel = Level;
            entry.Bucket = (entry.Posision[Level] + Indices[Level]) % Steps;

            return At(entry.Wheel, entry.Bucket).Add(std::move(entry));
        }

        Bucket::Iterator Add(Duration const &Interval, Entry::TCallback callback)
        {
            return Add(Interval.AsMilliseconds() / IntervalMS, std::move(callback));
        }

        inline void Remove(Bucket::Iterator Iterator)
        {
            if (Iterator == end())
                return;

            // if (entry._M_node != nullptr)

            At(Iterator->Wheel, Iterator->Bucket).Remove(Iterator);
        }

        inline Bucket &At(size_t _Wheel, size_t _Bucket)
        {
            return Wheels[_Wheel].Buckets[_Bucket];
        }

        inline Bucket &Current(size_t Stage = 0)
        {
            return At(Stage, Indices[Stage]);
        }

        inline auto end()
        {
            return At(0, 0).Entries.end();
        }

    private:
        void Offset(size_t Result[], size_t _Steps)
        {
            size_t Level = Stages - 1;

            for (; _Steps && Level < Stages; --Level)
            {
                auto &_Interval = Wheels[Level].Interval;

                Result[Level] = _Steps / _Interval;
                _Steps %= _Interval;
            }

            for (; Level < Stages; --Level)
            {
                Result[Level] = 0;
            }
        }

        void Increment(size_t Level = 0)
        {
            size_t &_Index = Indices[Level];
            Wheel &_Wheel = Wheels[Level];

            if (++_Index >= Steps)
            {
                _Index = 0;

                if (Level >= (Stages - 1))
                    return;

                Increment(++Level);

                Current(Level).Cascade(_Wheel, Level - 1);
            }
        }

        Wheel Wheels[Stages];
        size_t Indices[Stages];

        size_t IntervalMS;
    };
}*/