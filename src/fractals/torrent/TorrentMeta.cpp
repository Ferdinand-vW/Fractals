//
// Created by Ferdinand on 5/14/2022.
//

#include <filesystem>

#include "fractals/common/encode.h"
#include "fractals/torrent/Bencode.h"
#include "fractals/torrent/TorrentMeta.h"

namespace fractals::torrent {

    TorrentMeta::TorrentMeta(const MetaInfo &mi,std::string fileName)
                            : m_mi(mi), m_name(fileName) {

        auto info = m_mi.info; // Info dict tells us how the underlying data is structured
        auto fm = info.file_mode;
        std::filesystem::path p(fileName);

        // unpack the file info structure in MetaInfo into something more easily usable
        if(fm.isLeft) {
            // Single file mode. All data goes into a single file.
            auto fname = from_maybe(fm.leftValue.name,fileName);
            std::vector<std::string> paths;
            paths.push_back(fname);
            // We only need to push one file
            m_files.push_back(
                    FileInfo { fm.leftValue.length
                               ,fm.leftValue.md5sum
                               ,paths});
        }
        else {
            // multi file mode
            auto mdir = fm.rightValue.name;
            //if no dir is specified then use filename without extension
            m_dir = from_maybe(mdir,p.filename().stem().string());
            // Push multiple files
            std::copy( fm.rightValue.files.begin()
                      ,fm.rightValue.files.end()
                      ,back_inserter(m_files));
        }

        // Compute info hash and store
        m_info_hash = sha1_encode(encode(torrent::to_bdict(m_mi.info)));
    }

    std::string TorrentMeta::getName() const {
        return m_name;
    }

    std::string TorrentMeta::getDirectory() const {
        return m_dir;
    }

    std::vector<FileInfo> TorrentMeta::getFiles() const {
        return m_files;
    }


    MetaInfo TorrentMeta::getMetaInfo() const {
        return m_mi;
    }

    std::vector<char> TorrentMeta::getInfoHash() const {
        return m_info_hash;
    }

    int64_t TorrentMeta::getSize() const {
        int64_t sum = 0;
        for(auto &f : m_files) {
            sum+= f.length;
        }

        return sum;
    }
}