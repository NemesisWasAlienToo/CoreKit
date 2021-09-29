namespace Network{
    class Packet
    {
    private:
        void * _Buffer = nullptr;
        int _Size = 0;
        int _Sent = 0;
    public:
        Packet(void * Buffer, int Size) : _Buffer(Buffer), _Size(Size) {}
        ~Packet() { delete[] _Buffer;}
    };
}