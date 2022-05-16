#pragma once

#include <vector>

namespace fractals::torrent {

    struct Block {
        int64_t m_begin;
        std::vector<char> m_data;
        int64_t m_size;
    };

    /**
    Class containing actual piece byte data.
    The piece data are often split up in blocks. Size of each block is determined by the MetaInfo.
    */
    class PieceData {

        public:
            PieceData(int piece_index,int64_t length);

            int m_piece_index;
            int64_t m_length;
            std::vector<Block> m_blocks;

            bool is_complete();
            void add_block(const Block &b);
            std::vector<char> getBytes(int64_t begin, int64_t end);
            int64_t numBytes();
            int64_t remaining();
            int64_t next_block_begin();

        private:
            /**
             * Considers @pos to be an index in a single contiguous block of bytes
             * @param pos
             * @return A pair of indices. First element is index in @m_blocks and second is the index within the block.
             */
            std::pair<int64_t,int64_t> getIndex(int64_t pos);
    };

}