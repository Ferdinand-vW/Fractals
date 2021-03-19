#include "network/p2p/Client.h"
#include "boost/bind.hpp"
#include "app/Client.h"
#include "common/utils.h"
#include "network/p2p/Message.h"
#include "network/p2p/PeerId.h"
#include "torrent/PieceData.h"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

Client::Client(std::shared_ptr<tcp::socket> socket
              ,std::shared_ptr<deadline_timer> timer
              ,std::shared_ptr<Torrent> torrent)
              : m_request_cv(std::make_unique<std::condition_variable>())
              ,m_request_mutex(std::make_unique<std::mutex>())
              ,m_socket(socket)
              ,m_timer(timer)
              ,m_torrent(torrent)
              ,cur_piece(std::make_unique<PieceStatus>(
                                PieceStatus{PieceProgress::Nothing,
                                    PieceData{0,0,std::vector<Block>()}
                                    }
                                )
                ) {
    m_client_id = generate_peerId();
    m_timer->expires_at(boost::posix_time::pos_infin);
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
    m_timer->cancel_one(); // triggers write call back handler
}

void Client::received_interested(PeerId p) {
    m_peer_status[p].m_peer_interested = true;
}

void Client::received_not_interested(PeerId p) {
    m_peer_status[p].m_peer_interested = false;
}

void Client::received_have(PeerId p, int piece) {
    m_peer_status[p].m_available_pieces.insert(piece);
    // m_request_cv->notify_one();
}

void Client::received_bitfield(PeerId p, Bitfield &bf) {
    int piece_index = 0;
    for(auto b : bf.m_bitfield) {
        if(b) { m_peer_status[p].m_available_pieces.insert(piece_index); }
        piece_index++;
    }

    if(m_peer_status[p].m_available_pieces.size() > 0) {
        m_request_cv->notify_one();
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
    }
    else {
        cur_piece->m_progress = PieceProgress::Downloaded;
        m_timer->cancel_one();
    }
}

void Client::received_garbage(PeerId p) {
    
}

void Client::send_handshake(const HandShake &hs) {
    boost::asio::write(*m_socket.get(),boost::asio::buffer(hs.to_bytes_repr()));
}

void doNothing(){}


void Client::wait_for_unchoke(PeerId p) {
    m_timer->async_wait(boost::bind(&Client::send_messages,shared_from_this(),p));
}

void Client::send_messages(PeerId p) {
    std::vector<bool> bf;
    int tail = m_torrent->m_mi.info.pieces.size() % 4; // if num pieces is not multiple of 8 then add remaining bits
    for(int i = 0; i < m_torrent->m_mi.info.pieces.size() / 4;i++) {
        bf.push_back(0);
    }
    auto msg = Bitfield(bf.size(),bf);

    auto unchoke_msg = UnChoke();
    boost::asio::async_write(*m_socket.get()
                            ,boost::asio::buffer(UnChoke().to_bytes_repr())
                            ,std::bind(doNothing));
    cout << ">>> Unchoke " << p.m_ip << endl;

    cout << bytes_to_hex(msg.to_bytes_repr()) << endl;
    boost::asio::async_write(*m_socket.get()
                            ,boost::asio::buffer(Interested().to_bytes_repr())
                            ,boost::bind(&Client::sent_interested,this,p,boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

    cout << ">>> Interested " << p.m_ip << endl;
}

void Client::sent_interested(PeerId p,boost::system::error_code error, size_t size) {
    m_peer_status[p].m_am_interested = true;

    send_piece_request(p,error,size);
}

void Client::wait_send_piece_request(PeerId p,boost::system::error_code error, size_t size) {
    m_timer->async_wait(std::bind(&Client::send_piece_request,shared_from_this(),p,error,size));
}

void Client::send_piece_request(PeerId p,boost::system::error_code error, size_t size) {
    cout << "Error: " << error.message() << endl;
    cout << "Size: " << size << endl;

    if(cur_piece->m_progress == PieceProgress::Downloaded) {
        int request_size = 1 << 14; // 16KB
        int remaining = cur_piece->m_data.remaining();
        int piece_length = m_torrent->m_mi.info.piece_length;

        Request request(cur_piece->m_data.m_piece_index
                       ,cur_piece->m_data.next_block_begin()
                       ,std::min(remaining,std::min(piece_length,request_size)));
        cur_piece->m_progress = PieceProgress::Requested;
        boost::asio::async_write(*m_socket.get()
                                ,boost::asio::buffer(request.to_bytes_repr())
                                ,boost::bind(&Client::wait_send_piece_request,shared_from_this(),p
                                            ,boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        cout << ">>> Piece " + std::to_string(request.m_index) + " " + std::to_string(request.m_begin) + " " + std::to_string(request.m_length) << endl;

    } else if (cur_piece->m_progress == PieceProgress::Nothing) {
        // find new piece to download
        select_piece(p);

        //calculate size to request
        int request_size = 1 << 14 ; // 16KB, standard request size 
        int remaining = cur_piece->m_data.remaining(); // remaining data of piece
        int piece_length = m_torrent->m_mi.info.piece_length; //size of pieces

        cur_piece->m_progress = PieceProgress::Requested;
        Request request(cur_piece->m_data.m_piece_index
                       ,0
                       ,std::min(remaining,std::min(piece_length,request_size)));
        
        boost::asio::async_write(*m_socket.get()
                        ,boost::asio::buffer(request.to_bytes_repr())
                        ,boost::bind(&Client::wait_send_piece_request,shared_from_this(),p
                                    ,boost::asio::placeholders::error,
                                     boost::asio::placeholders::bytes_transferred));
        cout << ">>> Piece " + std::to_string(request.m_index) + " " + std::to_string(request.m_begin) + " " + std::to_string(request.m_length) << endl;
    } else {
        cout << "Completed downloading. Leaving thread.." << endl;
    }
}

void Client::select_piece(PeerId p) {
    int piece = *m_peer_status[p].m_available_pieces.begin();
    m_peer_status[p].m_available_pieces.erase(piece);
    int piece_size = m_torrent->size_of_piece(piece);

    cur_piece->m_data.m_piece_index = piece;
    cur_piece->m_data.m_length = piece_size;
    cur_piece->m_progress = PieceProgress::Nothing;
}