#pragma once

#include "IOLayer.h"

#include "fractals/common/Tagged.h"
#include "fractals/common/utils.h"
#include "fractals/disk/DiskEventQueue.h"
#include "fractals/persist/Models.h"
#include "fractals/sync/QueueCoordinator.h"
#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/MetaInfo.h"
#include "fractals/torrent/Piece.h"
#include "fractals/torrent/TorrentMeta.h"
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <unordered_map>

namespace fractals::disk
{
template <typename IOLayer> class DiskIOServiceImpl
{
  public:
    DiskIOServiceImpl(sync::QueueCoordinator &coordinator, DiskEventQueue::RightEndPoint queue)
        : coordinator(coordinator), queue(queue)
    {
        coordinator.addAsPublisherForDiskService<DiskEventQueue>(queue);
    }

    void run()
    {
        isActive = true;
        while (isActive)
        {
            coordinator.waitOnDiskServiceUpdate();
            assert(queue.canPop());

            while (queue.canPop())
            {
                processRequest();
            }
        }

        spdlog::info("DIS::run. Shutdown");
    }

    void processRequest()
    {
        auto &&request = std::move(queue.pop());
        std::visit(common::overloaded{[&](const Shutdown _)
                                      {
                                          stop();
                                      },
                                      [&](const auto &request)
                                      {
                                          process(request);
                                      }},
                   request);
    }

    void process(const Read &readTorr)
    {
        spdlog::info("DIS::process(Read)");

        const auto fpath = readTorr.filepath;
        if (!ioLayer.open(fpath))
        {
            spdlog::error("DIS::Cannot open file: {}", fpath);
            return;
        }

        auto mbd = bencode::decode<bencode::bdict>(ioLayer.getStream());
        ioLayer.close();

        if (mbd.has_error())
        {
            spdlog::error("DIS::Cannot parse file: {}", fpath);
            return;
        }

        bencode::bdict bd = mbd.value();
        auto mMetaInfo = torrent::from_bdata<torrent::MetaInfo>(bd);

        if (mMetaInfo.isLeft)
        {
            spdlog::error("DIS::Cannot convert bencoded file to MetaInfo object: {}", fpath);
            return;
        }

        torrent::TorrentMeta tm(mMetaInfo.rightValue, fpath);
        queue.push(ReadSuccess{tm, fpath, "./"});
    }

    void process(const WriteData &writeData)
    {
        spdlog::info("DIS::process(WriteData)");

        const auto it = torrents.find(writeData.infoHash);
        if (it == torrents.end())
        {
            spdlog::error("DIS::process could not match infoHash to Torrent", writeData.infoHash);
            return;
        }

        uint64_t index = 0;
        uint64_t offset = writeData.offset;
        common::string_view view(writeData.mData.begin(), writeData.mData.end());
        std::string dirName = it->second.first.dirName;
        for (auto &fi : it->second.second)
        {
            if (offset >= index && offset < index + fi.length)
            {
                uint64_t remainingBytesForFile = index + fi.length - offset;
                if (remainingBytesForFile > 0)
                {
                    const auto offsetInFile = offset - index;
                    const int64_t bytesToWrite = std::min(view.size(), remainingBytesForFile);
                    writeToFile(writeData.mPieceIndex, toFilePath(dirName, fi.dirName, fi.fileName), offsetInFile,
                                view, bytesToWrite);

                    // offset should be starting point for next file
                    view.remove_prefix(bytesToWrite);
                    offset += bytesToWrite;
                }
            }

            index += fi.length;
        }

        queue.push(WriteSuccess{writeData.infoHash, writeData.mPieceIndex});
    }

    void process(const InitTorrent &it)
    {
        const auto &infoHash = it.tm.infoHash;
        spdlog::info("DIS::process(InitTorrent) infoHash={}", infoHash);
        if (torrents.count(infoHash))
        {
            return;
        }

        torrents.emplace(infoHash, std::make_pair(it.tm, it.files));

        for (auto &fi : it.files)
        {
            const auto path = toFilePath(it.tm.dirName, fi.dirName, fi.fileName);
            bool dirResult = ioLayer.createDirectories(path);

            if (dirResult)
            {
                if (!ioLayer.createFile(path))
                {
                    spdlog::error("DIS::Unable to create torrent: {}", it.tm.name);
                    return;
                }
            }
        }

        queue.push(TorrentInitialized{infoHash});
    }

    void process(const DeleteTorrent &del)
    {
        spdlog::info("DIS::process(DeleteTorrent)");
        spdlog::error("DIS::process(DeleteTorrent) not implemented");
    }

    void process(const Shutdown &shut)
    {
        spdlog::info("DIS::process(Shutdown)");
        isActive = false;
    }

    IOLayer &getIO()
    {
        return ioLayer;
    }

  private:
    void writeToFile(uint32_t piece, const std::filesystem::path &path, uint64_t offset,
                     common::string_view buffer, uint64_t numBytes)
    {
        if (!ioLayer.open(path))
        {
            spdlog::error("DIS::Cannot open file: {}", path.string());
            return;
        }

        spdlog::debug(
            "DIS::writeToFile. Piece={} OffsetBeg={} OffsetEnd={} numBytes={} bufSize={} path={}",
            piece, offset, offset + numBytes, numBytes, buffer.size(), path.c_str());
        ioLayer.writeFrom(offset, buffer, numBytes);

        ioLayer.close();
    }

    void stop()
    {
        isActive = false;
        coordinator.forceNotifyDiskService();
    }

    std::filesystem::path toFilePath(const std::string &torrDir,
                                     const std::string &fileDir,
                                     const std::string &fileName)
    {
        return std::filesystem::path(torrDir + "/" + fileDir + "/" + fileName);
    }

    bool isActive{false};
    sync::QueueCoordinator &coordinator;
    DiskEventQueue::RightEndPoint queue;
    IOLayer ioLayer;

    std::unordered_map<common::InfoHash,
                       std::pair<persist::TorrentModel, std::vector<persist::FileModel>>>
        torrents;
};

using DiskIOService = DiskIOServiceImpl<IOLayer>;

} // namespace fractals::disk