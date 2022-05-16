//
// Created by Ferdinand on 5/16/2022.
//

#include <fstream>
#include <boost/log/sources/record_ostream.hpp>

#include "fractals/torrent/FileData.h"
#include "fractals/torrent/PieceData.h"
#include "fractals/torrent/TorrentMeta.h"
#include "fractals/torrent/TorrentIO.h"

namespace fractals::torrent {

    void TorrentIO::writePieceToDisk(const TorrentMeta &tm, PieceData &&pd) {
        BOOST_LOG(m_lg) << "[Torrent] writing piece " << pd.m_piece_index;
        auto fds = touchedFiles(tm, pd.m_piece_index); // which files does this piece touch?

        createFiles(fds); // make sure the files exist

        BOOST_LOG(m_lg) << "[Torrent] To write " << pd.numBytes() << " number of bytes";
        BOOST_LOG(m_lg) << "[Torrent] Spanning over " << fds.size() << " file(s)";

        int64_t bytes_pos;
        for(const auto &fd : fds) {
            std::unique_lock<std::mutex> l(m_mutex);
            //Open file for writing data
            std::fstream fstr(fd.full_path, std::fstream::out | std::fstream::in | std::fstream::binary);

            fstr.seekp(fd.begin); // set the correct offset to start write

            BOOST_LOG(m_lg) << "[Torrent] " << bytes_pos;
            BOOST_LOG(m_lg) << "[Torrent] " << fd.end;
            BOOST_LOG(m_lg) << "[Torrent] " << fd.begin;
            BOOST_LOG(m_lg) << "[Torrent] " << (bytes_pos + fd.end - fd.begin);
            auto bytes = pd.getBytes(bytes_pos, bytes_pos + fd.end - fd.begin);
            fstr.write(bytes.data(), bytes.size());
            bytes_pos = fd.end - fd.begin;

            BOOST_LOG(m_lg) << "[Torrent] Wrote " << bytes.size() << " bytes from offset " << fd.begin << " to " << fd.full_path;

            fstr.close();
        }

    }

    void TorrentIO::writePieceToDb(const TorrentMeta &tm, int piece) {

    }

    void TorrentIO::createFiles(const std::vector<FileData> &fds) {

    }

    neither::Either<std::string,TorrentMeta> TorrentIO::readTorrentFile(std::string fp) {

    }
}