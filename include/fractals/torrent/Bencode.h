#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <bencode/bencode.h>
#include <neither/neither.hpp>

#include <fractals/common/utils.h>
#include <fractals/torrent/MetaInfo.h>

using namespace bencode;
using namespace neither;
using namespace fractals::common;

namespace fractals::torrent {

    /**
    Conversions between bencoded data and a more structural form in terms of the MetaInfo ADT.
    */
    template <class A>
    Either<std::string,A> fromBdata(const bdata &bd);
    template <class A>
    Either<std::string,A> fromBdict(const bdict &bd);

    bdict toBdict(const FileInfo &fi);
    bdict toBdict(const SingleFile &sf);
    bdict toBdict(const MultiFile &mf);
    bdict toBdict(const InfoDict &id);

}