#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <bencode/bencode.h>
#include <neither/neither.hpp>

#include "fractals/common/utils.h"
#include "fractals/torrent/MetaInfo.h"

using namespace std;
using namespace bencode;
using namespace neither;
using namespace fractals::common;

namespace fractals::torrent {

    /**
    Conversions between bencoded data and a more structural form in terms of the MetaInfo ADT.
    */
    template <class A>
    Either<string,A> from_bdata(const bdata &bd);
    template <class A>
    Either<string,A> from_bdict(const bdict &bd);

    bdict to_bdict(const FileInfo &fi);
    bdict to_bdict(const SingleFile &sf);
    bdict to_bdict(const MultiFile &mf);
    bdict to_bdict(const InfoDict &id);

}