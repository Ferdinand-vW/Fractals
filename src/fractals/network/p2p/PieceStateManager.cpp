#include "fractals/network/p2p/PieceStateManager.h"

#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

namespace fractals::network::p2p
{
    PieceState::PieceState(uint32_t pieceIndex, uint32_t maxSize) 
                         : mPieceIndex(pieceIndex)
                         , mMaxSize(maxSize) 
    {
    }

    void PieceState::initialize()
    {
        mPieceData.reserve(mMaxSize);
    }

    void PieceState::clear()
    {
        mPieceData.clear();
    }

    uint32_t PieceState::getRemainingSize() const
    {
        return mMaxSize - mPieceData.size();
    }

    uint32_t PieceState::getMaxSize() const
    {
        return mMaxSize;
    }

    uint32_t PieceState::getNextBeginIndex() const
    {
        return mPieceData.size();
    }

    uint32_t PieceState::getPieceIndex() const
    {
        return mPieceIndex;
    }

    bool PieceState::isComplete() const
    {
        return getRemainingSize() == 0;
    }

    void PieceState::addBlock(const std::vector<char>& block)
    {
        common::append(mPieceData, block);
    }

    std::vector<char>&& PieceState::extractData()
    {
        return std::move(mPieceData);
    }

    common::string_view PieceState::getBuffer()
    {
        return common::string_view{mPieceData.begin(), mPieceData.end()};
    }

    PieceStateManager::PieceStateManager(const std::vector<uint32_t>& allPieces
                                        ,const std::vector<uint32_t>& completedPieces
                                        ,uint64_t totalSize
                                        ,const std::unordered_map<uint32_t, std::vector<char>>& hashes)
                                        :mAvailable(common::setDifference(
                                            std::unordered_set(allPieces.begin(), allPieces.end())
                                            , std::unordered_set(completedPieces.begin(), completedPieces.end())))
                                        ,mCompletedPieces(completedPieces.begin(), completedPieces.end())
                                        ,mHashes(hashes)
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
                mAllPieces.emplace(pieceIndex, PieceState{pieceIndex, pieceSize});

                ++i;
                ++it;
            }

            mAllPieces.emplace(*it, PieceState{*it, lastSize});
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

    PieceState* PieceStateManager::nextAvailablePiece(const std::unordered_set<uint32_t>& peerPieces)
    {
        std::unordered_set<uint32_t> pieces;

        for(auto piece : peerPieces)
        {
            if (mAvailable.count(piece))
            {
                auto it = mAllPieces.find(piece);

                if (it != mAllPieces.end())
                {
                    mAvailable.erase(piece);
                    mInProgress.insert(piece);

                    it->second.initialize();
                    return &it->second;
                }
            }
        }

        return nullptr;
    }

    bool PieceStateManager::isCompleted(uint32_t pieceIndex)
    {
        return mCompletedPieces.count(pieceIndex);
    }

    void PieceStateManager::makeCompleted(uint32_t pieceIndex)
    {
        auto it = mAllPieces.find(pieceIndex);
        if (it != mAllPieces.end())
        {
            mCompletedPieces.emplace(pieceIndex);
            it->second.clear();
        }
    }

    std::unordered_map<uint32_t, PieceState>::iterator PieceStateManager::begin()
    {
        return mAllPieces.begin();
    }

    std::unordered_map<uint32_t, PieceState>::iterator PieceStateManager::end()
    {
        return mAllPieces.end();
    }
}