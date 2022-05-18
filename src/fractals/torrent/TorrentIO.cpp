//
// Created by Ferdinand on 5/16/2022.
//

#include <fstream>
#include <boost/log/sources/record_ostream.hpp>
#include <filesystem>

#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/File.h"
#include "fractals/torrent/Piece.h"
#include "fractals/torrent/TorrentMeta.h"
#include "fractals/torrent/TorrentIO.h"

namespace fractals::torrent {

    void TorrentIO::writePieceToDisk(const TorrentMeta &tm, Piece &&pd) {
        BOOST_LOG(m_lg) << "[Torrent] writing piece " << pd.m_piece_index;
        auto fds = touchedFiles(tm, pd.m_piece_index); // which files does this piece touch?

        createFiles(tm.getDirectory(), fds); // make sure the files exist

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

    void TorrentIO::createFiles(std::string dir, std::vector<File> &fds) {
        //create root directory if does not exist
        if(dir != "") {
            BOOST_LOG(m_lg) << "[Torrent] Creating dir: " << dir;
            std::filesystem::create_directory(dir);
        }

        for(auto &fd : fds) {
            // prepend m_dir to file path
            std::vector<std::string> full_path;
            full_path.push_back(dir);
            std::copy(fd.fi.path.begin(),fd.fi.path.end(),std::back_inserter(full_path));

            // concat file path as single string
            auto fp = common::concat(full_path);
            std::filesystem::path p(fp);


            if(p.parent_path() != "") { // if parent path is empty then we're dealing with single file with no specified directory
                //creates the (sub)directories if they don't exist already
                std::filesystem::create_directories(p.parent_path());
                BOOST_LOG(m_lg) << "[Torrent] creating (sub) directories";
            }

            fd.full_path = fp; //assign file path to file data

            if(!std::filesystem::exists(p)) {
                std::fstream fstream;
                fstream.open(fp,std::fstream::out | std::fstream::binary);
                fstream.close();

                BOOST_LOG(m_lg) << "[Torrent] created file " << p.stem();
            }
        }
    }

    neither::Either<std::string,TorrentMeta> TorrentIO::readTorrentFile(std::string fp) {
        std::ifstream fstream;
        fstream.open(fp, std::ifstream::in | std::ifstream::binary);

        auto mbd = bencode::decode<bencode::bdict>(fstream);
        //return on decode failure
        if(mbd.has_error()) { return left(mbd.error().message()); }

        //value is safe since we return on error above
        bencode::bdict bd_ = mbd.value();

        std::filesystem::path p(fp);
        neither::Either<std::string,MetaInfo> bd = torrent::from_bdata<MetaInfo>(bencode::bdata(bd_));

        if(bd.isLeft) {
            return left<std::string>(bd.leftValue);
        } else {
            TorrentMeta tm(bd.rightValue,p.filename().string());
            return right<TorrentMeta>(tm);
        }
    }
}