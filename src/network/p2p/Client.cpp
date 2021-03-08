#include "network/p2p/Client.h"
#include "network/p2p/PeerId.h"
#include <string>

Client::Client(std::shared_ptr<tcp::socket> socket,std::fstream &torrent) : m_socket(socket),m_torrent(std::move(torrent)) {};

bool Client::connect_peer(PeerId p) {
    m_socket->connect(tcp::endpoint(boost::asio::ip::address::from_string(p.ip),p.port));

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
    int piece_index = 0;
    for(auto b : bf.m_bitfield) {
        if(b) { m_peer_status[p].m_available_pieces.insert(piece_index); }
        piece_index++;
    }
}

void Client::received_request(PeerId p, Request r) {
    cout << "Client received receive request from "+ p.ip + ":" + std::to_string(p.port) << endl;
    cout << "Client does not currently support receive request" << endl;
}

void Client::received_piece(PeerId p, Piece pc) {
    pc.
}