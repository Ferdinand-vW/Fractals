Thread #1

Epoll - ConnectionReadHandler
            Maintain queue of timeouts. On wait use lowest timeout.
            if timeout expires, wait wakes up and should push Timeout event to WorkQueue
            if no timeout then use -1
            If receive message from peer 

      - ConnectionWriteHandler

ConnectionReadHandler pushes to BufferedQueueManager
ConnectionWriteHandler reads from BufferedQueueManager

BufferedQueueManager pushes read events to WorkQueue
BufferedQueueManager pushes write ready to WorkQueue


Thread #2

Reads BitTorrent events from WorkQueue
Sleeps if empty, must be notified on push to queue

Apply BitTorrent protocol logic
Produces new BitTorrent events and pushes to SendQueue
if write read, sends head of sendQueue

If timeout is required for send then add timeout to ConnectionReadHandler
Also trigger wait block by add special fd and remove it immediately.



