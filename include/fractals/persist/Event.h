#pragma once

#include <cstdint>
#include <variant>

namespace fractals::persist
{
    struct AddTorrents
    {

    };

    struct RemoveTorrents
    {
    };

    struct LoadTorrents
    {

    };

    struct AddPieces
    {
        uint32_t mPieceIndex;
    };

    struct RemovePieces
    {

    };

    struct LoadPieces
    {

    };

    struct AddAnnounces
    {

    };

    struct RemoveAnnounces
    {

    };

    struct LoadAnnounces
    {

    };

    using StorageEvent = std::variant
        <AddTorrents, RemoveTorrents, LoadTorrents
        ,AddPieces,   RemovePieces, LoadPieces
        ,AddAnnounces, RemoveAnnounces, LoadAnnounces>;
}