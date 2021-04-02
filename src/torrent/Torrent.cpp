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
        // single file mode.
        auto fname = from_maybe(fm.leftValue.name,fileName);
        std::vector<std::string> paths;
        paths.push_back(fname);
        // We only need to push one file
        m_files.push_back(FileInfo { fm.leftValue.length, fm.leftValue.md5sum, paths});
    }
    else {
        // multi file mode
        auto mdir = fm.rightValue.name;
        m_dir = from_maybe(mdir,p.filename().stem().string());
        // Push multiple files
        std::copy(fm.rightValue.files.begin(),fm.rightValue.files.end(),back_inserter(m_files));
    }

    // Compute info hash and store
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

/*
Function computes for a given piece, start offset and number of bytes across how many file(s)
it spans. I.e. a piece may encompass the second 50% of file A, 100% of file B and 20% of file C.
The offset for this piece is then 50% of file A (plus #bytes of any previous files).
And the length 
*/
std::vector<FileData> Torrent::divide_over_files(int piece) {
    std::vector<FileData> fds;

    auto bytes_begin = piece == 0 ? 0 : cumulative_size_of_pieces(piece - 1);
    auto piece_len = size_of_piece(piece);

    cout << "[Torrent] bytes begin: " << bytes_begin << endl;
    
    auto files_bytes = 0;
    for(auto fi : m_files) {
        cout << "[Torrent] looking at file " << concat_paths(fi.path) << endl;

        auto next_file_begin = files_bytes + fi.length;
        
        if(bytes_begin < next_file_begin) { // start position is in current file and there is data to write
            auto cur_file_offset = bytes_begin - files_bytes; // start position in current file
            auto file_remaining = next_file_begin - bytes_begin; // bytes left in remainder of current file
            
            // How much can we write/is there to write
            int to_write;
            if (file_remaining < piece_len) {
                to_write = file_remaining;
            } else {
                to_write = piece_len;
            }

            //Add FileData to result vector
            auto fd = FileData { fi,files_bytes, files_bytes + to_write };
            fds.push_back(fd);
            piece_len -= to_write; // update how many bytes are left
        }

        // return early if there are no bytes left
        if(piece_len <= 0) { 
            return fds;
        }

        files_bytes = next_file_begin;
    }

    return fds;
}

void Torrent::create_files(const std::vector<FileData> &fds) {
    cout << "[Torrent] creating files for file data" << endl;
    //create root directory if does not exist
    if(m_dir != "") {
        cout << "[Torrent] Creating dir: " << m_dir << endl;
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
            cout << "[Torrent] creating (sub) directories" << endl;
        }

        if(!std::filesystem::exists(p)) {
            std::fstream fstream;
            fstream.open(fp,std::fstream::out | std::fstream::binary);
            fstream.seekp(fd.fi.length-1); //create file of actual size
            fstream.write("",1);
            fstream.close();
            cout << "[Torrent] created file " << p.stem() << endl;
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
        cout << "[Torrent] file length" << file_length << endl;
        //last piece has size of remainder of file
        return file_length - (info.pieces.size() - 1) * info.piece_length;
    } else {
        //In multi file mode we need to traverse over each file and sum
        int sum_file_lengths = 0;
        for(auto &f : info.file_mode.rightValue.files) {
            sum_file_lengths += f.length;
        }

        cout << "[Torrent] sum of files" << sum_file_lengths << endl;

        return sum_file_lengths - (info.pieces.size() - 1) * info.piece_length;
    }
}

long long Torrent::cumulative_size_of_pieces(int piece) {
    int sum = 0;
    for(int i = 0; i < piece + 1;i++) { // piece is zero based index
        sum += size_of_piece(i);
    }

    return sum;
}

void Torrent::write_data(PieceData &&pd) {
    cout << "[Torrent] writing to piece " << pd.m_piece_index << endl;
    auto fds = divide_over_files(pd.m_piece_index);

    create_files(fds);

    std::vector<char> bytes;
    for(auto &b : pd.m_blocks) {
        std::move(b.m_data.begin(),b.m_data.end(),back_inserter(bytes));
    }

    cout << "[Torrent] To write " << bytes.size() << " number of bytes" << endl;
    cout << "[Torrent] Spanning over " << fds.size() << " file(s)" << endl;
    int bytes_pos = 0;
    for(auto fd : fds) {
        std::fstream fstream;

        fstream.open(concat_paths(fd.fi.path), std::fstream::in | std::fstream::out | std::fstream::binary);

        fstream.seekp(fd.begin); // set the correct offset to start write

        std::vector<char> bytes_for_file(bytes.begin() + bytes_pos,bytes.begin() + bytes_pos + fd.end - fd.begin);
        fstream.write(bytes_for_file.data(), bytes_for_file.size());
        bytes_pos = fd.end - fd.begin;

        cout << "[Torrent] Wrote " << bytes_for_file.size() << " bytes to " << concat_paths(fd.fi.path) << endl;

        fstream.close();
    }
}
 
