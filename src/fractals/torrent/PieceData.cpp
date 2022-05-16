#include <iostream>

#include "fractals/common/logger.h"
#include "fractals/torrent/PieceData.h"

namespace fractals::torrent {

    PieceData::PieceData(int piece_index,int64_t length) : m_piece_index(piece_index),m_length(length) {}

    bool PieceData::is_complete() {
        int block_length = 0;
        for(auto &b : m_blocks) {
            block_length += b.m_data.size();
        }
        return block_length == m_length;
    }

    void PieceData::add_block(const Block &incoming) {
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

    int64_t PieceData::remaining() {
        int sum = 0;
        for(auto &b : m_blocks) { sum += b.m_data.size(); }
        return m_length - sum;
    }

    int64_t PieceData::next_block_begin() {
        if(m_blocks.size() == 0) {
            return 0;
        }
        else {
            auto bl = m_blocks.back();
            return bl.m_begin + bl.m_data.size();
        }
    }

    int64_t PieceData::numBytes() {
       int64_t res = 0;

       for(auto &bl : m_blocks) {
            res+= bl.m_size;
       }

       return res;
    }

    std::vector<char> PieceData::getBytes(int64_t begin, int64_t end) {
        std::vector<char> bytes;

        auto beginPos = getIndex(begin);
        auto endPos = getIndex(end);

        // Return early if invalid inputs were provided
        if(beginPos.first < 0 || endPos.first < 0 || end < begin) { return {}; }

        auto dBBegin = m_blocks[beginPos.first].m_data.begin();
        auto dBEnd   = m_blocks[beginPos.first].m_data.end();
        auto dEBegin = m_blocks[endPos.first].m_data.begin();

        if(beginPos.first == endPos.first) {
            bytes.insert(bytes.end()
                        ,dBBegin + beginPos.second
                        ,dBBegin + endPos.second + 1);
        }
        else {

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

    std::pair<int64_t ,int64_t> PieceData::getIndex(int64_t pos) {

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

}