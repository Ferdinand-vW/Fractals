#pragma once

#include "fractals/common/encode.h"
#include "fractals/common/utils.h"
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace fractals::network::p2p
{
    class PieceState
    {
        public:
            PieceState(uint32_t pieceIndex, uint32_t maxSize);

            void initialize();

            void clear();

            uint32_t getRemainingSize() const;

            uint32_t getMaxSize() const;

            uint32_t getNextBeginIndex() const;

            uint32_t getPieceIndex() const;

            bool isComplete() const;

            void addBlock(const std::vector<char>& block);

            common::string_view getBuffer();
            std::vector<char>&& extractData();

        private:
            std::vector<char> mPieceData;
            uint32_t mPieceIndex;
            uint32_t mMaxSize{0};
    };

    class PieceStateManager
    {
        public:
            PieceStateManager(const std::vector<uint32_t>& allPieces
                             ,const std::vector<uint32_t>& completedPieces
                             ,uint64_t totalSize
                             ,const std::unordered_map<uint32_t, std::vector<char>>& hashes);

            PieceState* getPieceState(uint32_t pieceIndex);

            PieceState* nextAvailablePiece(const std::unordered_set<uint32_t>& peerPieces);

            bool isCompleted(uint32_t pieceIndex);
            void makeCompleted(uint32_t pieceIndex);

            template <typename Container>
            bool hashCheck(uint32_t pieceIndex, const Container& pieceData)
            {
                const auto it = mHashes.find(pieceIndex);

                if (it != mHashes.end())
                {
                    return common::sha1_encode(pieceData) == it->second;
                }

                return false;
            }

            std::unordered_map<uint32_t, PieceState>::iterator begin();

            std::unordered_map<uint32_t, PieceState>::iterator end();

        std::unordered_map<uint32_t, PieceState> mAllPieces;
        std::unordered_set<uint32_t> mAvailable;
        std::unordered_set<uint32_t> mInProgress;
        std::unordered_set<uint32_t> mCompletedPieces;
        std::unordered_map<uint32_t, std::vector<char>> mHashes;

        
    };
}