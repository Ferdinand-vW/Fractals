#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <set>

#include <bencode/bencode.h>
#include <neither/either.hpp>

#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include "fractals/common/logger.h"
#include "fractals/network/p2p/Message.h"
#include "fractals/torrent/Piece.h"
#include "fractals/torrent/Torrent.h"
#include "fractals/torrent/MetaInfo.h"
#include "fractals/torrent/Bencode.h"

namespace fractals::torrent {

    Torrent::Torrent(TorrentMeta &&tm,TorrentIO &tio,std::set<int> &&pieces)
                    : m_tm(std::move(tm)),m_io(tio),m_pieces(std::move(pieces)) {}

    Torrent::Torrent(const TorrentMeta &tm, TorrentIO &tio, const std::set<int> &pieces)
                    : m_tm(tm),m_io(tio),m_pieces(pieces) {}

    void Torrent::add_piece(int p) {
        m_pieces.insert(p);
    }

    void Torrent::add_piece(std::set<int> pieces) {
        m_pieces.insert(pieces.begin(),pieces.end());
    }

    std::set<int> Torrent::get_pieces() {
        return m_pieces;
    }

    const TorrentMeta &Torrent::getMeta() const {
        return m_tm;
    }

    TorrentIO &Torrent::getIO() const {
        return m_io;
    }

    std::string Torrent::getName() const {
        return getMeta().getName();
    }

    void Torrent::writePiece(Piece &&p) {
        getIO().writePieceToDisk(getMeta(),std::move(p));
        getIO().writePieceToDb(getMeta(),p.m_piece_index);
    }

    const MetaInfo& Torrent::getMetaInfo() const {
        return getMeta().getMetaInfo();
    }

    std::shared_ptr<Torrent> Torrent::makeTorrent(TorrentMeta &&tm, TorrentIO &tio, std::set<int> &&pieces) {
        return std::make_shared<Torrent>(std::move(tm),tio,std::move(pieces));
    }

    std::shared_ptr<Torrent> Torrent::makeTorrent(const TorrentMeta &tm, TorrentIO &tio, const std::set<int> &pieces) {
        return std::make_shared<Torrent>(tm,tio,pieces);
    }

}