#pragma once

#include <vector>

namespace fractals::torrent {

    struct Block {
        long long m_begin;
        std::vector<char> m_data;
    };

    /**
    Class containing actual piece byte data.
    The piece data are often split up in blocks. Size of each block is determined by the MetaInfo.
    */
    class PieceData {

        public:
            PieceData(int piece_index,long long length);

            int m_piece_index;
            long long m_length;
            std::vector<Block> m_blocks;

            bool is_complete();
            void add_block(const Block &b);
            long long remaining();
            long long next_block_begin();
    };

}