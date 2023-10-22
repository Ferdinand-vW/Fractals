//
// Created by Ferdinand on 5/14/2022.
//

#include <filesystem>
#include <variant>

#include "fractals/common/Tagged.h"
#include "fractals/common/encode.h"
#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/MetaInfo.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::torrent {

    TorrentMeta::TorrentMeta(const MetaInfo &mi,std::string fileName)
                            : metaInfo(mi), name(fileName) {

        auto info = metaInfo.info; // Info dict tells us how the underlying data is structured
        auto fm = info.fileMode;
        std::filesystem::path p(fileName);

        // unpack the file info structure in MetaInfo into something more easily usable
        if(std::holds_alternative<SingleFile>(fm)) {
            auto& singleFile = std::get<SingleFile>(fm);
            // Single file mode. All data goes into a single file.
            auto fname = singleFile.name.empty() ? fileName : singleFile.name;
            std::vector<std::string> paths;
            paths.push_back(fname);
            // We only need to push one file
            files.push_back(
                    FileInfo { singleFile.length
                               ,singleFile.md5sum
                               ,paths});
        }
        else {
            auto& multiFile = std::get<MultiFile>(fm);
            // multi file mode
            auto mdir = multiFile.name;
            //if no dir is specified then use filename without extension
            dir = mdir.empty() ? p.filename().stem().string() : mdir;
            // Push multiple files
            std::copy( multiFile.files.begin()
                      ,multiFile.files.end()
                      ,back_inserter(files));
        }

        // Compute info hash and store
        infoHash = sha1_encode<20>(encode(torrent::toBdict(metaInfo.info)));
    }

    std::string TorrentMeta::getName() const {
        return name;
    }

    std::string TorrentMeta::getDirectory() const {
        return dir;
    }

    std::vector<FileInfo> TorrentMeta::getFiles() const {
        return files;
    }


    const MetaInfo& TorrentMeta::getMetaInfo() const {
        return metaInfo;
    }

    const common::InfoHash& TorrentMeta::getInfoHash() const {
        return infoHash;
    }

    uint64_t TorrentMeta::getSize() const {
        uint64_t sum = 0;
        for(const auto &f : files) {
            sum+= f.length;
        }

        return sum;
    }
}