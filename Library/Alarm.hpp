/**
 * @file Alarm.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief
 * @version 0.1
 * @date 2022-01-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <functional>
#include <map>

#include <Test.hpp>
#include <Event.hpp>
#include <Timer.hpp>

#include <Network/EndPoint.hpp>
#include <Network/Socket.hpp>

#include <Iterable/List.hpp>
#include <Iterable/Poll.hpp>

#include <Format/Serializer.hpp>
#include <Network/DHT/DHT.hpp>

namespace Core
{
    class Alarm
    {
    public:
        using EndCallback = std::function<void()>;

    private:
        using AlarmList = Iterable::List<std::tuple<DateTime, EndCallback>>;

        std::mutex Lock;
        Timer Clock;

        std::mutex ALock;
        AlarmList Alarms;

        volatile size_t Tracking;

    public:
        // Constructors

        Alarm() : Clock(Timer::Monotonic, 0) {}

        // Peroperties

        inline Descriptor INode()
        {
            return Clock.INode();
        }

        // Functionalities

        size_t Schedual(Duration Timeout, EndCallback End)
        {
            if (Timeout <= Duration(0, 0))
            {
                return 0;
            }

            {
                std::lock_guard lock(ALock);

                DateTime Expires = DateTime::FromNow(Timeout);

                Alarms.Add(std::tuple(Expires, std::move(End)));

                if (Alarms.Length() == 1 || Expires < std::get<0>(Alarms[Tracking]))
                {
                    Tracking = Alarms.Length() - 1;
                }

                Clock.Set(std::get<0>(Alarms[Tracking]).Left());

                return Alarms.Length() - 1;
            }
        }

        void Remove(size_t Index)
        {
            std::lock_guard lock(ALock);

            Alarms.Swap(Index);

            Wind();
        }

        void Loop()
        {
            Wind();

            {
                std::lock_guard lock(Lock);

                Clock.Listen();
            }

            Clean();
        }

    protected:
        size_t Soonest()
        {
            size_t tempTrack = 0;

            for (size_t i = 1; i < Alarms.Length(); i++)
            {
                if (std::get<0>(Alarms[i]) < std::get<0>(Alarms[tempTrack]))
                {
                    tempTrack = i;
                }
            }

            return tempTrack;
        }

        void Wind()
        {
            std::lock_guard lock(ALock);

            if (Alarms.Length() == 0)
            {
                Clock.Stop();
                return;
            }

            Tracking = Soonest();

            if (std::get<0>(Alarms[Tracking]).IsExpired())
            {
                if (std::get<1>(Alarms[Tracking]))
                {
                    std::get<1>(Alarms[Tracking])();
                }

                Wind();

                return;
            }

            Clock.Set(std::get<0>(Alarms[Tracking]).Left());
        }

        void Clean()
        {
            EndCallback End;

            {
                std::lock_guard lock(ALock);

                End = std::move(std::get<1>(Alarms[Tracking]));

                Alarms.Swap(Tracking);
            }

            End();
        }
    };
}