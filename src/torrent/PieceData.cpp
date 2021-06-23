#include "torrent/PieceData.h"
#include <functional>
#include <numeric>
#include <iostream>

PieceData::PieceData(int piece_index,long long length) : m_piece_index(piece_index),m_length(length) {}

bool PieceData::is_complete() {
    int block_length = 0;
    for(auto &b : m_blocks) {
        block_length += b.m_data.size();
    }
    return block_length == m_length;
}

void PieceData::add_block(const Block &incoming) {
    bool overlaps = false;
    for(auto &existing : m_blocks) {
        auto o = incoming.m_begin < (existing.m_begin + existing.m_data.size()) 
                && incoming.m_begin > existing.m_begin;
        overlaps = overlaps || o;
    }

    if(overlaps) {
        std::cout << "Incoming block for piece " << m_piece_index << " overlaps with existing blocks" << std::endl; 
    } else {
        m_blocks.push_back(incoming);
    }
    
}

long long PieceData::remaining() {
    int sum = 0;
    for(auto &b : m_blocks) { sum += b.m_data.size(); }
    return m_length - sum;
}

long long PieceData::next_block_begin() {
    if(m_blocks.size() == 0) {
        return 0;
    }
    else {
        auto bl = m_blocks.back();
        return bl.m_begin + bl.m_data.size();
    }
}
