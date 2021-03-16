#pragma once

#include <vector>

struct Block {
    int m_begin;
    std::vector<char> m_data;
};

class PieceData {

    public:
        int m_piece_index;
        int m_length;
        std::vector<Block> m_blocks;

        bool is_complete();
        void add_block(const Block &b);
        int remaining();
        int next_block_begin();
};
