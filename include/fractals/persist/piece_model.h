#pragma once

namespace fractals::persist {

    /**
    ADT for piece model in database
    */
    struct PieceModel {
        int id;
        int torrent_id;
        int piece;
    };

}