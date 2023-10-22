#include "fractals/network/p2p/PieceStateManager.h"

#include "fractals/common/Tagged.h"
#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <unordered_set>

namespace fractals::network::p2p
{
PieceState::PieceState(uint32_t pieceIndex, uint64_t maxSize, uint64_t offset)
    : mPieceIndex(pieceIndex), mMaxSize(maxSize), offset(offset)
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

uint64_t PieceState::getRemainingSize() const
{
    return mMaxSize - mPieceData.size();
}

uint64_t PieceState::getMaxSize() const
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

uint64_t PieceState::getOffset() const
{
    return offset;
}

bool PieceState::isComplete() const
{
    return getRemainingSize() == 0;
}

void PieceState::addBlock(const std::string_view block)
{
    common::append(mPieceData, block);
}

std::vector<char> &&PieceState::extractData()
{
    return std::move(mPieceData);
}

std::string_view PieceState::getBuffer()
{
    return std::string_view{mPieceData.begin(), mPieceData.end()};
}

void PieceStateManager::populate(const std::vector<persist::PieceModel> &pieceModels)
{
    std::vector<uint32_t> completedPieces;
    completedPieces.reserve(pieceModels.size());
    for (const auto pm : pieceModels)
    {
        if (pm.complete)
        {
            completedPieces.emplace_back(pm.piece);
        }
        else
        {
            mAvailable.emplace(pm.piece);
        }

        mHashes.emplace(pm.piece, common::PieceHash{pm.hash});
    }

    uint64_t offset{0};
    for (const auto &pm : pieceModels)
    {
        mAllPieces.emplace(pm.piece, PieceState(pm.piece, pm.size, offset));
        offset += pm.size;
    }
}

PieceState *PieceStateManager::getPieceState(uint32_t pieceIndex)
{
    auto it = mAllPieces.find(pieceIndex);

    if (it != mAllPieces.end())
    {
        return &(it->second);
    }

    return nullptr;
}

PieceState *PieceStateManager::nextAvailablePiece(const std::unordered_set<uint32_t> &peerPieces)
{
    std::unordered_set<uint32_t> pieces;

    for (auto piece : peerPieces)
    {
        if (mAvailable.count(piece))
        {
            auto it = mAllPieces.find(piece);

            if (it != mAllPieces.end())
            {
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

bool PieceStateManager::isAllComplete() const
{
    return mAvailable.empty();
}

bool PieceStateManager::isActive() const
{
    return active;
}

void PieceStateManager::setActive(bool b)
{
    active = b;
}

void PieceStateManager::makeCompleted(uint32_t pieceIndex)
{
    auto it = mAllPieces.find(pieceIndex);
    if (it != mAllPieces.end())
    {
        mAvailable.erase(pieceIndex);
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
} // namespace fractals::network::p2p