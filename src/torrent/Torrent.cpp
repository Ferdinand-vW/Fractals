#include <filesystem>
#include <fstream>
#include <iterator>

#include "bencode/bencode.h"
#include "common/encode.h"
#include "common/utils.h"
#include "neither/either.hpp"
#include "network/p2p/Message.h"
#include "torrent/Torrent.h"
#include "torrent/MetaInfo.h"
#include "torrent/BencodeConvert.h"


Torrent::Torrent(MetaInfo &mi,std::string fileName,std::set<int> pieces) 
                : m_name(fileName),m_mi(mi),m_lg(logger::get()),m_pieces(pieces) {
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

neither::Either<std::string,Torrent> Torrent::read_torrent(std::string fp) {
    std::ifstream fstream;
    fstream.open(fp, std::ifstream::binary);
    
    auto mbd = bencode::decode<bencode::bdict>(fstream);
    //return on decode failure
    if(mbd.has_error()) { return left(mbd.error().message()); }

    //value is safe since we return on error above
    bencode::bdict bd_ = mbd.value();

    std::filesystem::path p(fp);
    neither::Either<std::string,MetaInfo> bd = BencodeConvert::from_bdata<MetaInfo>(bencode::bdata(bd_));

    if(bd.isLeft) {
        return left<std::string>(bd.leftValue);
    } else {
        return right(Torrent(bd.rightValue,p.stem().string(),{}));
    }
}

/*
Function computes for a given piece, start offset and number of bytes across how many file(s)
it spans. I.e. a piece may encompass the second 50% of file A, 100% of file B and 20% of file C.
The offset for this piece is then 50% of file A (plus #bytes of any previous files).
And the length 
*/
std::vector<FileData> Torrent::divide_over_files(int piece) {
    std::vector<FileData> fds;

    long long bytes_begin = piece == 0 ? 0 : cumulative_size_of_pieces(piece - 1);
    auto piece_len = size_of_piece(piece);

    auto cum_files_bytes = 0;
    for(auto fi : m_files) {
        auto next_file_begin = cum_files_bytes + fi.length;
        
        if(bytes_begin < next_file_begin) { // start position is in current file and there is data to write
            auto cur_file_offset = bytes_begin - cum_files_bytes; // start position in current file
            auto file_remaining = next_file_begin - bytes_begin; // bytes left in remainder of current file
            
            // How much can we write/is there to write
            int to_write;
            if (file_remaining < piece_len) {
                to_write = file_remaining;
            } else {
                to_write = piece_len;
            }

            //Add FileData to result vector
            auto fd = FileData { fi,cur_file_offset, cur_file_offset + to_write };
            fds.push_back(fd);
            piece_len -= to_write; // update how many bytes are left

            //given that there was data to write in the current file
            //the offset to write at for the next file must be at the begin of that file
            bytes_begin = next_file_begin;
        }

        // return early if there are no bytes left
        if(piece_len <= 0) { 
            return fds;
        }

        cum_files_bytes = next_file_begin;
    }

    return fds;
}

void Torrent::create_files(std::vector<FileData> &fds) {
    //create root directory if does not exist
    if(m_dir != "") {
        BOOST_LOG(m_lg) << "[Torrent] Creating dir: " << m_dir;
        std::filesystem::create_directory(m_dir);
    }

    for(auto &fd : fds) {
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

long long Torrent::size_of_pieces(std::set<int> pieces) {
    //all pieces but last have equal size.
    long long sum_size_pieces = m_mi.info.number_of_pieces() * m_mi.info.piece_length;

    //if last piece is also present then we must subtract one standard piece size 
    // from the above and add the last piece size
    int last_piece = m_mi.info.number_of_pieces() - 1;
    bool has_last_piece = pieces.find(last_piece) != pieces.end();
    if(has_last_piece) {
        sum_size_pieces -= m_mi.info.piece_length; //subtract standard piece size
        sum_size_pieces += size_of_piece(last_piece); //then add last piece size
    }

    return sum_size_pieces;

}

long long Torrent::size_of_piece(int piece) {
    auto info = m_mi.info;
    int piece_length = info.piece_length;
    int num_pieces = info.number_of_pieces();

    // All pieces but last have uniform size as specified in the info dict
    if(piece != num_pieces - 1) { return piece_length; }
    
    // For the last piece we need to compute the total file size (sum of all file lengths)
    if(info.file_mode.isLeft) {
        //if single file mode then it's simple
        int file_length = info.file_mode.leftValue.length;
        //last piece has size of remainder of file
        return file_length - (num_pieces - 1) * info.piece_length;
    } else {
        //In multi file mode we need to traverse over each file and sum
        int sum_file_lengths = 0;
        for(auto &f : info.file_mode.rightValue.files) {
            sum_file_lengths += f.length;
        }

        return sum_file_lengths - (num_pieces - 1) * info.piece_length;
    }
}

void Torrent::add_piece(int p) {
    m_pieces.insert(p);
}

void Torrent::add_piece(std::set<int> pieces) {
    m_pieces.insert(pieces.begin(),pieces.end());
}

std::set<int> Torrent::get_pieces() {
    return m_pieces;
}

long long Torrent::cumulative_size_of_pieces(int piece) {
    int sum = 0;
    for(int i = 0; i < piece + 1;i++) { // piece is zero based index
        sum += size_of_piece(i);
    }

    return sum;
}

void Torrent::write_data(PieceData &&pd) {
    BOOST_LOG(m_lg) << "[Torrent] writing piece " << pd.m_piece_index;
    auto fds = divide_over_files(pd.m_piece_index);

    create_files(fds);

    std::vector<char> bytes;
    for(auto &b : pd.m_blocks) {
        std::move(b.m_data.begin(),b.m_data.end(),back_inserter(bytes));
    }

    BOOST_LOG(m_lg) << "[Torrent] To write " << bytes.size() << " number of bytes";
    BOOST_LOG(m_lg) << "[Torrent] Spanning over " << fds.size() << " file(s)";
    int bytes_pos = 0;
    for(auto fd : fds) {
        std::fstream fstream;

        fstream.open(fd.full_path, std::fstream::out | std::fstream::in | std::fstream::binary);

        fstream.seekp(fd.begin); // set the correct offset to start write

        BOOST_LOG(m_lg) << "[Torrent] " << bytes_pos;
        BOOST_LOG(m_lg) << "[Torrent] " << fd.end;
        BOOST_LOG(m_lg) << "[Torrent] " << fd.begin;
        BOOST_LOG(m_lg) << "[Torrent] " << (bytes_pos + fd.end - fd.begin);
        std::vector<char> bytes_for_file(bytes.begin() + bytes_pos,bytes.begin() + bytes_pos + fd.end - fd.begin);
        fstream.write(bytes_for_file.data(), bytes_for_file.size());
        bytes_pos = fd.end - fd.begin;

        BOOST_LOG(m_lg) << "[Torrent] Wrote " << bytes_for_file.size() << " bytes from offset " << fd.begin << " to " << fd.full_path;

        fstream.close();
    }
}
 
