#pragma once

#include <fractals/common/Tagged.h>
#include <fractals/common/encode.h>
#include <fractals/common/utils.h>
#include <fractals/persist/Models.h>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fractals::network::p2p
{
class PieceState
{
  public:
    PieceState(uint32_t pieceIndex, uint64_t maxSize, uint64_t offset);

    void initialize();

    void clear();

    uint64_t getRemainingSize() const;

    uint64_t getMaxSize() const;

    uint32_t getNextBeginIndex() const;

    uint32_t getPieceIndex() const;

    uint64_t getOffset() const;

    bool isComplete() const;

    void addBlock(const std::string_view block);

    std::string_view getBuffer();
    std::vector<char> &&extractData();

  private:
    std::vector<char> mPieceData;
    uint32_t mPieceIndex;
    uint64_t mMaxSize{0};
    uint64_t offset;
};

class PieceStateManager
{
  public:
    PieceStateManager() = default;

    void populate(const std::vector<persist::PieceModel>& pieces);

    PieceState *getPieceState(uint32_t pieceIndex);

    PieceState *nextAvailablePiece(const std::unordered_set<uint32_t> &peerPieces);

    bool isCompleted(uint32_t pieceIndex);
    bool isAllComplete() const;
    bool isActive() const;
    void setActive(bool);
    void makeCompleted(uint32_t pieceIndex);

    template <typename Container> bool hashCheck(uint32_t pieceIndex, const Container &pieceData)
    {
        const auto it = mHashes.find(pieceIndex);
        if (it != mHashes.end())
        {
            const auto sha1 = common::sha1_encode<20>(pieceData);
            const auto sha1Hex = common::bytesToHex<20>(sha1);
            const auto storedHex = common::bytesToHex<20>(it->second.underlying);
            spdlog::info("PSM::hashCheck. Storedhash={} computedHash={}",storedHex, sha1Hex);
            return common::sha1_encode<20>(pieceData) == it->second;
        }

        spdlog::error("PSM::hashCheck. Could not find hash for piece={}", pieceIndex);
        return false;
    }

    std::unordered_map<uint32_t, PieceState>::iterator begin();

    std::unordered_map<uint32_t, PieceState>::iterator end();

    bool active{false};
    std::unordered_map<uint32_t, PieceState> mAllPieces;
    std::unordered_set<uint32_t> mAvailable;
    std::unordered_set<uint32_t> mInProgress;
    std::unordered_set<uint32_t> mCompletedPieces;
    std::unordered_map<uint32_t, common::PieceHash> mHashes;
};
} // namespace fractals::network::p2p