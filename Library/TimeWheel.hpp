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
    class TimeWheel : public Timer
    {
    public:
        struct Wheel;
        struct Bucket;

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
                    entry.Callback();

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

            Wheel() = default;
            Wheel(size_t interval) : Interval(interval) {}
        };

        TimeWheel() = default;
        TimeWheel(Duration const &interval) : Timer(Timer::Monotonic, 0), Interval(interval.AsMilliseconds())
        {
            size_t TempInterval = Interval;

            for (size_t i = 0; i < Wheels.size(); i++)
            {
                Wheels[i].Interval = TempInterval;
                TempInterval *= Steps;
            }
        }

        // constexpr size_t MaxSteps()  const noexcept
        // {
        //     size_t Counter = Steps;

        // }

        // Duration MaxDelay()
        // {
        // }

        inline void Start()
        {
            auto _Interval = Duration::FromMilliseconds(Interval);
            Timer::Set(_Interval, _Interval);
        }

        inline void Execute()
        {
            Increment();

            Current().Execute();
        }

        Bucket::Iterator Add(Duration const &interval, Entry::TCallback callback)
        {
            Entry entry{std::move(callback), Offset(interval), 0, 0};

            size_t Level = 0;

            for (Level = Wheels.size() - 1; Level > 0 && entry.Posision[Level] == 0; --Level)
            {
            }

            entry.Wheel = Level;
            entry.Bucket = (entry.Posision[Level] + Indices[Level]) % Wheels[Level].Buckets.size();

            return At(entry.Wheel, entry.Bucket).Add(std::move(entry));
        }

        inline void Remove(Bucket::Iterator entry)
        {
            At(entry->Wheel, entry->Bucket).Remove(entry);
        }

        inline Bucket &At(size_t _Wheel, size_t _Bucket)
        {
            return Wheels[_Wheel].Buckets[_Bucket];
        }

        inline Bucket &Current(size_t Stage = 0)
        {
            return At(Stage, Indices[Stage]);
        }

    private:
        std::array<size_t, Stages> Offset(Duration const &duration)
        {
            std::array<size_t, Stages> Result;

            size_t _Steps = duration.AsMilliseconds(); // / Interval;

            for (size_t Level = Wheels.size() - 1; _Steps && Level < Wheels.size(); --Level)
            {
                auto &_Interval = Wheels[Level].Interval;

                Result[Level] = _Steps / _Interval;
                _Steps %= _Interval;
            }

            return Result;
        }

        void Increment(size_t Level = 0)
        {
            if (Level >= (Wheels.size() - 1))
                return;

            size_t &_Index = Indices[Level];
            Wheel &_Wheel = Wheels[Level];

            if (++_Index >= _Wheel.Buckets.size())
            {
                _Index = 0;

                Increment(++Level);

                Current(Level).Cascade(_Wheel, Level - 1);
            }
        }

        // @todo Change this to miliseconds

        size_t Interval;
        std::array<Wheel, Stages> Wheels{0};
        std::array<size_t, Stages> Indices{0};
    };
}