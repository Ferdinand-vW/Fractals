#include "fractals/network/p2p/PieceStateManager.h"

#include "fractals/common/utils.h"
#include <cstdint>
#include <unordered_set>

namespace fractals::network::p2p
{
    PieceState::PieceState(uint64_t maxSize) : mMaxSize(maxSize) 
    {
    }

    void PieceState::initialize()
    {
        mPieceData.reserve(mMaxSize);
    }

    uint64_t PieceState::getRemainingSize() const
    {
        return mMaxSize - mPieceData.size();
    }

    uint64_t PieceState::getMaxSize() const
    {
        return mMaxSize;
    }

    uint64_t PieceState::getNextBeginIndex() const
    {
        return mPieceData.size();
    }

    bool PieceState::isComplete() const
    {
        return getRemainingSize() == 0;
    }

    void PieceState::addBlock(const std::vector<char>& block)
    {
        common::append(mPieceData, block);
    }

    common::string_view PieceState::getBuffer()
    {
        return common::string_view{mPieceData.begin(), mPieceData.end()};
    }

    PieceStateManager::PieceStateManager(std::unordered_set<uint32_t> allPieces, uint64_t totalSize)
    {
        if (allPieces.size() > 0)
        {
            const uint32_t numPiecesExceptLast = allPieces.size() - 1;
            const uint32_t pieceSize = 
                numPiecesExceptLast >= 1 ? totalSize / numPiecesExceptLast : totalSize;
            const uint32_t lastSize = totalSize - (pieceSize * numPiecesExceptLast);

            uint32_t i = 0;
            auto it = allPieces.begin();
            while(i < numPiecesExceptLast)
            {
                const auto pieceIndex = *it;
                mAllPieces.emplace(pieceIndex, PieceState{pieceSize});

                ++i;
                ++it;
            }

            mAllPieces.emplace(*it, PieceState{lastSize});
        }
    }

    PieceState* PieceStateManager::getPieceState(uint32_t pieceIndex)
    {
        auto it = mAllPieces.find(pieceIndex);

        if (it != mAllPieces.end())
        {
            return &(it->second);
        }

        return nullptr;
    }

    void PieceStateManager::initializePiece(uint32_t pieceIndex)
    {
        auto *ps = getPieceState(pieceIndex);

        if (ps)
        {
            ps->initialize();
        }
    }
}