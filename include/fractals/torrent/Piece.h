#pragma once

#include <cstdint>
#include <vector>
#include <set>

namespace fractals::torrent {

    struct TorrentMeta;
    struct File;

    struct Block {
        int64_t m_begin;
        std::vector<char> m_data;
        int64_t m_size;
    };

    /**
    Class containing actual piece byte data.
    The piece data are often split up in blocks. Size of each block is determined by the MetaInfo.
    */
    class Piece {

        public:
            Piece(int piece_index, int64_t length);

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

    /**
    Add up size of each piece up to and including @piece.
    Can be used to calculate offset of piece by cumulative_size_of_pieces(piece - 1)
    */
    int64_t cumulative_size_of_pieces(const TorrentMeta &tm, int piece);

    /**
     * Calculate the size of a piece in number of bytes. Usually this is specified by the
     * MetaInfo, except for the last piece which contains the remainder of bytes.
     * @param piece index
     * @return
     */
    int64_t size_of_piece(const TorrentMeta &tm, int piece);

    int64_t size_of_piece(const TorrentMeta &tm, std::set<int> pieces);

    /**
    Function computes for a given piece, start offset and number of bytes across how many file(s)
    it spans. I.e. a piece may encompass the second 50% of file A, 100% of file B and 20% of file C.
    The offset for this piece is then 50% of file A (plus #bytes of any previous files).
    */
    std::vector<File> touchedFiles(const TorrentMeta &tm, int piece);

}