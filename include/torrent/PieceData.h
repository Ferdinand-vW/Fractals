#pragma once

#include <vector>

struct Block {
    long long m_begin;
    std::vector<char> m_data;
};

class PieceData {

    public:
        int m_piece_index;
        long long m_length;
        std::vector<Block> m_blocks;

        bool is_complete();
        void add_block(const Block &b);
        long long remaining();
        long long next_block_begin();
};
