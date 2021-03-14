#include "network/p2p/Client.h"
#include "boost/bind.hpp"
#include "app/Client.h"
#include "network/p2p/PeerId.h"
#include "torrent/PieceData.h"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <condition_variable>
#include <mutex>
#include <string>

Client::Client(std::unique_ptr<std::mutex> request_mutex,std::unique_ptr<std::condition_variable> request_cv
              ,std::shared_ptr<tcp::socket> socket
              ,std::shared_ptr<Torrent> torrent)
              : m_request_cv(std::move(request_cv))
              ,m_request_mutex(std::move(request_mutex))
              ,m_socket(socket)
              ,m_torrent(torrent)
              ,cur_piece(std::make_unique<PieceStatus>(
                                PieceStatus{PieceProgress::Nothing,
                                    PieceData{0,0,std::vector<Block>()}
                                    }
                                )
                ) {
    m_client_id = generate_peerId();
    // Pieces are zero based index
    for(int i = 0; i < torrent->m_mi.info.pieces.size();i++) {
        m_missing_pieces.insert(i);
    }
};

bool Client::connect_peer(PeerId p) {
    m_socket->connect(tcp::endpoint(boost::asio::ip::address::from_string(p.m_ip),p.m_port));

    return true;
}

bool Client::has_all_pieces() {
    return m_missing_pieces.size() == 0;
}

bool Client::is_choked_by(PeerId p) {
    return m_peer_status[p].m_peer_choking;
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
    std::unique_lock<std::mutex> lock(*m_request_mutex.get());
    cout << "client writing data.." << endl;
    Block b = Block { pc.m_begin, pc.m_block };

    cur_piece->m_data.add_block(b);
    if(cur_piece->m_data.is_complete()) {
        
        if(has_all_pieces()) { //Only report completed if all pieces have been downloaded
            cur_piece->m_progress = PieceProgress::Completed;
        } else { //otherwise we can continue to request pieces
            cur_piece->m_progress = PieceProgress::Nothing;
        }
        
        PieceData piece = cur_piece->m_data;
        m_torrent->write_data(std::move(piece));

        // update internal state of required pieces
        m_missing_pieces.erase(piece.m_piece_index);
        m_existing_pieces.insert(piece.m_piece_index);

        cur_piece.reset();

        m_request_cv->notify_one();
    }
}

void Client::received_garbage(PeerId p) {
    
}

void Client::send_handshake(const HandShake &hs) {
    boost::asio::write(*m_socket.get(),boost::asio::buffer(hs.to_bytes_repr()));
}

void Client::send_messages(PeerId p) {
    cout << "send messages" << endl;
    boost::asio::async_write(*m_socket.get()
                            ,boost::asio::buffer(Interested().to_bytes_repr())
                            ,boost::bind(&Client::sent_interested,this, p));
    cout << "after sent" << endl;
}

void Client::sent_interested(PeerId p) {
    m_peer_status[p].m_am_interested = true;

    send_piece_request(p);
}

void Client::send_piece_request(PeerId p) {
    cout << "send piece request" << endl;
    std::unique_lock<std::mutex> lock(*m_request_mutex.get());
    cout << "taken lock" << endl;
    //only continue if we're not being choked by the peer or if we've not yet requested a (new) piece
    m_request_cv->wait(lock,[this,p]() { 
        auto progress = cur_piece->m_progress;
        auto choking = m_peer_status[p].m_peer_choking; 
        return !choking && (progress != PieceProgress::Requested); });
    cout << "notified by cv" << endl;

    if(cur_piece->m_progress == PieceProgress::Downloaded) {
        Request request(cur_piece->m_data.m_piece_index,cur_piece->m_data.next_block_begin(),m_torrent->m_mi.info.piece_length);
        cur_piece->m_progress = PieceProgress::Requested;
        boost::asio::async_write(*m_socket.get()
                                ,boost::asio::buffer(request.to_bytes_repr())
                                ,boost::bind(&Client::send_piece_request,this,p));

    } else if (cur_piece->m_progress == PieceProgress::Nothing) {
        // find new piece to download
        auto piece = m_peer_status[p].m_available_pieces.begin();
        m_peer_status[p].m_available_pieces.erase(piece);

        cur_piece->m_progress = PieceProgress::Requested;
        Request request(cur_piece->m_data.m_piece_index,0,m_torrent->m_mi.info.piece_length);
        boost::asio::async_write(*m_socket.get()
                        ,boost::asio::buffer(request.to_bytes_repr())
                        ,boost::bind(&Client::send_piece_request,this,p));

    } else {
        cout << "Completed downloading. Leaving thread.." << endl;
    }
}