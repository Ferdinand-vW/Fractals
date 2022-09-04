ConnectionsEventHandler
    Take in Send queue
    Take in Read queue

    Notify thread pool on data arrival

    Notified by on send queue

    Read messages
    Send messages

    subscribe
    unsubscribe

1 thread for reading from sockets
1 thread for sending to sockets
1 thread for handling BitTorrent logic
1 thread for writing 

           GUI
            |
Socket -> BitTorrent -> File/DB
Socket <- BitTorrent <- File/DB
            |
        TimeOutManager

TaskQueueEvents:
    - ReceivedMessage
    - FileDataReady
    - DBDataReady
    - TimeOutEvent
    - NewPeer
    - RemovePeer

while(active)
{
    if(TimeOutActive)
    {
        wait_for(timeout)
        if(has_time_out_passed)
        {
            SetPeerTimeout
        }
        else
        {
            
        }
    }
    else
    {
        wait()
    }

    while()
}

class PeerEventManager
{
    ReceiveEventManager
    SendEventManager
    SendQueue&
    ReadQueue&
    ActivePeers

    void init()
}

class ReceiveEventManager
{
    public:
        ReceiveEventManager(ReadQueue &rq)
        {
            ep = create_epoll
        }

        ~ReceiveEventManager

        void run()
        {
            while(active)
            {
                ep.wait()

                //read from fd and put in ReadQueue
            }
        }

        void addPeer(PeerSocket)
        void removePeer(PeerSocket)


}

class SendEventManager
{
    public:
        SendEventManager(SendQueue &sq)
        {
            
        }

        void run()
        {
            while(active)
            {
                sq.wait

                std::lock_guard

                while(!sq.isEmpty)
                {
                    sq.deque
                    write to sockets   
                    // write to sockets until queue is empty

                }
            }
        }

}



