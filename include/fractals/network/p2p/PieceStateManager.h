#pragma once

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
            PieceState(uint64_t maxSize);

            void initialize();

            uint64_t getRemainingSize() const;

            uint64_t getMaxSize() const;

            uint64_t getNextBeginIndex() const;

            bool isComplete() const;

            void addBlock(const std::vector<char>& block);

            common::string_view getBuffer();

        private:
            std::vector<char> mPieceData;
            uint64_t mMaxSize{0};
    };

    class PieceStateManager
    {
        public:
            PieceStateManager(std::unordered_set<uint32_t> allPieces, uint64_t totalSize);

            PieceState* getPieceState(uint32_t pieceIndex);

            void initializePiece(uint32_t pieceIndex);

        std::unordered_map<uint32_t, PieceState> mAllPieces;
    };
}