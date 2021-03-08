#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>

#include "common/utils.h"
#include "torrent/Torrent.h"
#include "torrent/MetaInfo.h"


Torrent::Torrent(MetaInfo &mi,std::string fileName) : m_mi(mi) {
    auto info = m_mi.info;
    auto fm = info.file_mode;
    boost::filesystem::path p(fileName);
    if(fm.isLeft) {
        auto paths = fm.leftValue.path;
        if(paths.begin() != paths.end()) { m_name = paths.front(); }
        else { m_name = p.filename().stem().string(); }
        m_files.push_back(fm.leftValue);
    }
    else {
        auto mdir = fm.rightValue.name;
        m_dir = from_maybe(mdir,p.filename().stem().string());
        m_files = fm.rightValue.files;
    }
}

std::vector<FileData> Torrent::match_file_data(int piece,int offset, int len) {
    std::vector<FileData> fds;

    auto piece_length = m_mi.info.piece_length;
    auto bytes_begin = piece * piece_length + offset;

    auto files_bytes = 0;
    for(auto fi : m_files) {
        int next_file_begin = files_bytes + fi.length;
        //
        if(bytes_begin < next_file_begin) { // start position is in current file and there is data to write
            int cur_file_offset = bytes_begin - files_bytes; // start position in current file
            int file_remaining = next_file_begin - bytes_begin; // bytes left in remainder of current file
            
            // How much can we write/is there to write
            int to_write;
            if (file_remaining < len) {
                to_write = file_remaining;
            } else {
                to_write = len; // 
            }

            //Add FileData to result vector
            auto fd = FileData { fi,files_bytes, files_bytes + to_write - 1};
            fds.push_back(fd);
            len -= to_write; // update how many bytes are left
        }

        // return early if there are no bytes left
        if(len <= 0) { 
            return fds;
        }

        files_bytes = next_file_begin;
    }

    return fds;
}

void Torrent::create_files(const std::vector<FileData> &fds) {
    
    //create root directory if does not exist
    if(m_dir != "") {
        boost::filesystem::create_directory(m_dir);
    }

    for(auto fd : fds) {
        auto fp = concat_paths(fd.fi.path);
        boost::filesystem::path p(fp);
        boost::filesystem::create_directories(p); //creates the (sub)directories if they don't exist already
        if(!boost::filesystem::exists(p)) {
            std::fstream fstream;
            fstream.open(fp,std::fstream::out | std::fstream::binary);
            fstream.seekp(fd.fi.length-1);
            fstream.close();
        }
    }
}

void Torrent::write_data(int piece, int offset, std::vector<char> bytes) {
    auto fds = match_file_data(piece, offset, bytes.size());

    create_files(fds);

    int bytes_pos = 0;
    for(auto fd : fds) {
        std::fstream fstream;

        fstream.open(concat_paths(fd.fi.path), std::fstream::in | std::fstream::out | std::fstream::binary);

        fstream.seekp(fd.begin); // set the correct offset to start write

        std::vector<char> bytes_for_file(bytes.begin() + bytes_pos,bytes.begin() + fd.end - fd.begin);
        fstream.write(bytes_for_file.data(), bytes_for_file.size());

        fstream.close();
    }
}
 
