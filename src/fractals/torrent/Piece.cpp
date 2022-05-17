#include <iostream>

#include "fractals/common/logger.h"
#include "fractals/torrent/Piece.h"
#include "fractals/torrent/File.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::torrent {

    Piece::Piece(int piece_index, int64_t length) : m_piece_index(piece_index), m_length(length) {}

    bool Piece::is_complete() {
        int block_length = 0;
        for(auto &b : m_blocks) {
            block_length += b.m_data.size();
        }
        return block_length == m_length;
    }

    void Piece::add_block(const Block &incoming) {
        bool overlaps = false;
        for(const auto &existing : m_blocks) {
            auto o = incoming.m_begin < (existing.m_begin + existing.m_data.size()) 
                    && incoming.m_begin > existing.m_begin;
            overlaps = overlaps || o;
        }

        if(overlaps) {
            BOOST_LOG(common::logger::get()) << "Incoming block for piece " << m_piece_index << " overlaps with existing blocks" << std::endl; 
        } else {
            m_blocks.push_back(incoming);
        }
        
    }

    int64_t Piece::remaining() {
        int sum = 0;
        for(auto &b : m_blocks) { sum += b.m_data.size(); }
        return m_length - sum;
    }

    int64_t Piece::next_block_begin() {
        if(m_blocks.size() == 0) {
            return 0;
        }
        else {
            auto bl = m_blocks.back();
            return bl.m_begin + bl.m_data.size();
        }
    }

    int64_t Piece::numBytes() {
       int64_t res = 0;

       for(auto &bl : m_blocks) {
            res+= bl.m_size;
       }

       return res;
    }

    std::vector<char> Piece::getBytes(int64_t begin, int64_t end) {
        std::vector<char> bytes;

        auto beginPos = getIndex(begin);
        auto endPos = getIndex(end);

        // Return early if invalid inputs were provided
        if(beginPos.first < 0 || endPos.first < 0 || end < begin) { return {}; }

        auto dBBegin = m_blocks[beginPos.first].m_data.begin();
        auto dBEnd   = m_blocks[beginPos.first].m_data.end();
        auto dEBegin = m_blocks[endPos.first].m_data.begin();

        if(beginPos.first == endPos.first) { // indexes specify a range within a block
            bytes.insert(bytes.end()
                        ,dBBegin + beginPos.second
                        ,dBBegin + endPos.second + 1);
        }
        else { // range across at least two blocks

            // (Partially) insert the first block
            bytes.insert(bytes.end(), dBBegin + beginPos.second,dBEnd);

            // Fully insert all 'in-between' blocks (may be empty)
            for (int i = beginPos.first + 1; i < endPos.first; i++) {
                bytes.insert(bytes.end(), m_blocks[i].m_data.begin(), m_blocks[i].m_data.end());
            }

            // (Partially) insert the last block
            bytes.insert(bytes.end(), dEBegin,dEBegin + endPos.second + 1);

        }

        return bytes;
    }

    std::pair<int64_t ,int64_t> Piece::getIndex(int64_t pos) {

        int64_t curr_pos = 0;
        for(int interIndex = 0; interIndex < m_blocks.size(); ++interIndex) {

            auto blockSize = m_blocks[interIndex].m_size;
            if(blockSize + curr_pos > pos) {
                return {interIndex, pos - curr_pos};
            }

            curr_pos += blockSize;
        }

        return {-1,-1}; // Could not find position
    }

    int64_t size_of_piece(const TorrentMeta &tm, int piece) {
        auto info = tm.getMetaInfo().info;
        int64_t piece_length = info.piece_length;
        int64_t num_pieces = info.number_of_pieces();

        // All pieces but last have uniform size as specified in the info dict
        if(piece != num_pieces - 1) { return piece_length; }

        // For the last piece we need to compute the total file size (sum of all file lengths)
        int64_t totalSize = 0;

        if(info.file_mode.isLeft) {
            //if single file mode then it's simple
            totalSize = info.file_mode.leftValue.length;
        } else {
            //In multi file mode we need to traverse over each file and sum
            for (auto &f: info.file_mode.rightValue.files) {
                totalSize += f.length;
            }
        }

        //size of last piece equals total file size minus sum of sizes of all but the last piece
        return totalSize - (num_pieces - 1) * info.piece_length;
    }

    int64_t size_of_piece(const TorrentMeta &tm, std::set<int> pieces) {
        int64_t sum = 0;
        for(auto p : pieces) {
            sum += size_of_piece(tm, p);
        }

        return sum;
    }

    int64_t cumulative_size_of_pieces(const TorrentMeta &tm, int piece) {
        int64_t sum = 0;
        for(int i = 0; i <= piece;i++) { // piece is zero based index
            sum += size_of_piece(tm, i);
        }

        return sum;
    }
}