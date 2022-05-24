class Runnable
{
protected:
    enum class States : char
    {
        Stopped = 0,
        Running,
    };

    volatile States State = States::Stopped;

// public:
    inline void Run()
    {
        State = States::Running;
    }

    inline void Stop()
    {
        State = States::Stopped;
    }

    inline bool IsRunning()
    {
        return State == States::Running;
    }
};