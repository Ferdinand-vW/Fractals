#include <filesystem>
#include <fstream>
#include <iterator>

#include "bencode/bencode.h"
#include "common/encode.h"
#include "common/utils.h"
#include "network/p2p/Message.h"
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

std::vector<FileData> Torrent::divide_by_files(int piece,long long offset, long long len) {
    std::vector<FileData> fds;

    auto piece_length = m_mi.info.piece_length;
    auto bytes_begin = piece * piece_length + offset;

    cout << "divide by files" << endl;
    cout << "piece length: " << piece_length << endl;
    cout << "bytes begin: " << bytes_begin << endl;
    cout << "len: " << len << endl;
    auto files_bytes = 0;
    for(auto fi : m_files) {
        cout << "file " << intercalate("/",fi.path) << endl;
        cout << "file size: " << fi.length << endl;
        auto next_file_begin = files_bytes + fi.length;
        //
        cout << "cond " << bytes_begin << " " << next_file_begin << endl;
        if(bytes_begin < next_file_begin) { // start position is in current file and there is data to write
            auto cur_file_offset = bytes_begin - files_bytes; // start position in current file
            auto file_remaining = next_file_begin - bytes_begin; // bytes left in remainder of current file
            
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
    cout << "creating files" << endl;
    //create root directory if does not exist
    if(m_dir != "") {
        cout << "created dir: " << m_dir << endl;
        std::filesystem::create_directory(m_dir);
    }

    for(auto fd : fds) {
        // prepend m_dir to file path
        std::vector<std::string> full_path;
        full_path.push_back(m_dir);
        std::copy(fd.fi.path.begin(),fd.fi.path.end(),std::back_inserter(full_path));

        // concat file path as single string
        auto fp = concat_paths(full_path);
        std::filesystem::path p(fp);

        
        if(p.parent_path() != "") { // if parent path is empty then we're dealing with single file with no specified directory
            //creates the (sub)directories if they don't exist already
            std::filesystem::create_directories(p.parent_path()); 
        }

        if(!std::filesystem::exists(p)) {
            std::fstream fstream;
            fstream.open(fp,std::fstream::out | std::fstream::binary);
            fstream.seekp(fd.fi.length-1); //create file of actual size
            fstream.write("",1);
            fstream.close();
        } else {
            cout << "file or directory with same path already exists: " + fp << endl;
        }
    }
}

long long Torrent::size_of_piece(int piece) {
    auto info = m_mi.info;
    int piece_length = info.piece_length;

    // All pieces but last have uniform size as specified in the info dict
    if(piece != info.pieces.size() -1) { return piece_length; }
    
    // For the last piece we need to compute the total file size (sum of all file lengths)
    if(info.file_mode.isLeft) {
        //if single file mode then it's simple
        int file_length = info.file_mode.leftValue.length;
        cout << "file length" << file_length << endl;
        //last piece has size of remainder of file
        return file_length - (info.pieces.size() - 1) * info.piece_length;
    } else {
        //In multi file mode we need to traverse over each file and sum
        int sum_file_lengths = 0;
        for(auto &f : info.file_mode.rightValue.files) {
            sum_file_lengths += f.length;
        }

        cout << "sum of files" << sum_file_lengths << endl;

        return sum_file_lengths - (info.pieces.size() - 1) * info.piece_length;
    }
}

void Torrent::write_data(PieceData &&pd) {
    cout << "write data" << endl;
    cout << "writing to piece " << pd.m_piece_index << endl;
    auto fds = divide_by_files(pd.m_piece_index, 0, pd.m_length);

    create_files(fds);

    std::vector<char> bytes;
    for(auto &b : pd.m_blocks) {
        std::move(b.m_data.begin(),b.m_data.end(),back_inserter(bytes));
    }

    cout << "writing " << bytes.size() << " bytes" << endl;
    cout << "file data pieces " << fds.size() << endl;
    int bytes_pos = 0;
    for(auto fd : fds) {
        std::fstream fstream;

        cout << concat_paths(fd.fi.path) << endl << endl;
        fstream.open(concat_paths(fd.fi.path), std::fstream::in | std::fstream::out | std::fstream::binary);

        fstream.seekp(fd.begin); // set the correct offset to start write

        std::vector<char> bytes_for_file(bytes.begin() + bytes_pos,bytes.begin() + bytes_pos + fd.end - fd.begin);
        fstream.write(bytes_for_file.data(), bytes_for_file.size());
        bytes_pos = fd.end - fd.begin;

        fstream.close();
    }
}
 
