//
// Created by Ferdinand on 5/17/2022.
//

#include "fractals/torrent/File.h"
#include "fractals/torrent/Piece.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::torrent {

    std::vector<File> touchedFiles(const TorrentMeta &tm, int piece) {
        std::vector<File> fds;

        int64_t bytes_begin = piece == 0 ? 0 : cumulative_size_of_pieces(tm,piece - 1);
        auto piece_len = size_of_piece(tm, piece);

        auto curr_offset = 0;
        for(auto fi : tm.getFiles()) {
            auto next_file_begin = curr_offset + fi.length;

            if(bytes_begin < next_file_begin) { // start position is in current file and there is data to write
                auto cur_file_offset = bytes_begin - curr_offset; // start position in current file
                auto file_remaining = next_file_begin - bytes_begin; // bytes left in remainder of current file

                // How much can we write/is there to write
                int64_t to_write;
                if (file_remaining < piece_len) {
                    to_write = file_remaining;
                } else {
                    to_write = piece_len;
                }

                //Add File to result vector
                auto fd = File {fi, cur_file_offset, cur_file_offset + to_write };
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

            curr_offset = next_file_begin;
        }

        return fds;
    }
}