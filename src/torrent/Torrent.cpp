#include <filesystem>
#include <fstream>
#include <iterator>

#include "bencode/bencode.h"
#include "common/encode.h"
#include "common/utils.h"
#include "torrent/Torrent.h"
#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"


Torrent::Torrent(MetaInfo &mi,std::string fileName) : m_mi(mi) {
    auto info = m_mi.info;
    auto fm = info.file_mode;
    std::filesystem::path p(fileName);
    if(fm.isLeft) {
        auto fname = from_maybe(fm.leftValue.name,fileName);
        std::vector<std::string> paths;
        paths.push_back(fname);
        m_files.push_back(FileInfo { fm.leftValue.length, fm.leftValue.md5sum, paths});
    }
    else {
        auto mdir = fm.rightValue.name;
        m_dir = from_maybe(mdir,p.filename().stem().string());
        std::copy(fm.rightValue.files.begin(),fm.rightValue.files.end(),back_inserter(m_files));
    }

    m_info_hash = sha1_encode(bencode::encode(BencodeConvert::to_bdict(m_mi.info)));
}

Torrent Torrent::read_torrent(std::string fp) {
    std::ifstream fstream;
    fstream.open(fp, std::ifstream::binary);
    auto mbd = bencode::decode<bencode::bdict>(fstream);
    bencode::bdict bd_ = mbd.value();

    std::filesystem::path p(fp);
    neither::Either<std::string,MetaInfo> bd = BencodeConvert::from_bdata<MetaInfo>(bencode::bdata(bd_));

    return Torrent(bd.rightValue,p.stem().string());

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
        std::filesystem::create_directory(m_dir);
    }

    for(auto fd : fds) {
        auto fp = concat_paths(fd.fi.path);
        std::filesystem::path p(fp);
        std::filesystem::create_directories(p); //creates the (sub)directories if they don't exist already
        if(!std::filesystem::exists(p)) {
            std::fstream fstream;
            fstream.open(fp,std::fstream::out | std::fstream::binary);
            fstream.seekp(fd.fi.length-1);
            fstream.close();
        }
    }
}

void Torrent::write_data(int piece, int offset, const std::vector<char> &bytes) {
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
 