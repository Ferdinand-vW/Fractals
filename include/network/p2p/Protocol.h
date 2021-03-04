#pragma once

void start_protocol(Client c,Peer p) {
    auto ok = initiate_protocol(c, p);
    if (!ok) { return; }

    send_interest(c,p);

    rcv_unChoke(c,p);
}

bool initiate_protocol(Client c,Peer p);


show_interest(Client c,Peer p);

choke(Client c,Peer p);

unChoke(Client c, Peer p);