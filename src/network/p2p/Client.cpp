#include "network/p2p/Client.h"
#include "app/Client.h"
#include "network/p2p/PeerId.h"
#include <string>

Client::Client(std::shared_ptr<tcp::socket> socket,const Torrent &torrent) : m_socket(socket),m_torrent(torrent) {
    m_client_id = generate_peerId();
};

bool Client::connect_peer(PeerId p) {
    m_socket->connect(tcp::endpoint(boost::asio::ip::address::from_string(p.m_ip),p.m_port));

    return true;
}

void Client::received_choke(PeerId p) {
    m_peer_status[p].m_peer_choking = true;
}

void Client::received_unchoke(PeerId p) {
    m_peer_status[p].m_peer_choking = false;
}

void Client::received_interested(PeerId p) {
    m_peer_status[p].m_peer_interested = true;
}

void Client::received_not_interested(PeerId p) {
    m_peer_status[p].m_peer_interested = false;
}

void Client::received_have(PeerId p, int piece) {
    m_peer_status[p].m_available_pieces.insert(piece);
}

void Client::received_bitfield(PeerId p, Bitfield &bf) {
    cout << "Client received bitfield" << endl;
    int piece_index = 0;
    for(auto b : bf.m_bitfield) {
        if(b) { m_peer_status[p].m_available_pieces.insert(piece_index); }
        piece_index++;
    }
}

void Client::received_request(PeerId p, Request r) {
    cout << "Client received receive request from "+ p.m_ip + ":" + std::to_string(p.m_port) << endl;
    cout << r.m_index << " " << r.m_begin << " " << r.m_length << endl;
    cout << "Client does not currently support receive request" << endl;
}

void Client::received_piece(PeerId p, Piece pc) {
    cout << "client writing data.." << endl;
    Block b = Block { pc.m_begin, pc.m_block };

    cur_piece->add_block(b);
    if(cur_piece->is_complete()) {
        m_torrent.write_data(std::move(*cur_piece.get()));
        cur_piece = nullptr;
    }
}

void Client::received_garbage(PeerId p) {
    
}

void Client::send_handshake(const HandShake &hs) {
    boost::asio::write(*m_socket.get(),boost::asio::buffer(hs.to_bytes_repr()));
}

void Client::send_piece_request(PeerId p) {
    if(cur_piece != nullptr) {
        std::cout << "Cannot request new piece. Client busy downloading " << cur_piece->m_piece_index << std::endl;
    } else {
        auto piece = m_peer_status[p].m_available_pieces.begin();
        Request request(*piece,cur_piece->next_block_begin(),m_torrent.m_mi.info.piece_length);
        m_peer_status[p].m_available_pieces.erase(piece);
        boost::asio::write(*m_socket.get(),boost::asio::buffer(request.to_bytes_repr()));
    }
}