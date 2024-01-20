//
// Created by Ferdinand on 5/13/2022.
//

#include <fractals/torrent/Bencode.h>
#include <fractals/common/maybe.h>
#include <boost/bind/mem_fn.hpp>
#include <cstdint>
#include <type_traits>

namespace fractals::torrent
{
template <typename T, typename Ret> Maybe<Ret> bdataToValueM(const bdata *t)
{
    if (t)
    {
        if constexpr (std::is_same_v<T, bint>)
        {
            return std::get<bint>(t->value()).value();
        }

        if constexpr (std::is_same_v<T, bstring>)
        {
            return std::get<bstring>(t->value()).value();
        }

        if constexpr (std::is_same_v<T, blist>)
        {
            return std::get<blist>(t->value()).values();
        }

        if constexpr (std::is_same_v<T, bdict>)
        {
            return std::get<bdict>(t->value()).values();
        }
    }

    return maybe();
}

template <> Either<std::string, SingleFile> fromBdict<SingleFile>(const bdict &bd)
{
    auto name = toMaybe(bd.find("name")).flatMap(toBstring).map(mem_fn(&bstring::to_string));
    auto mLength = toMaybe(bd.find("length")).flatMap(toBint).map(std::mem_fn(&bint::value));
    auto eLength = maybeToEither(mLength, "Missing length field for structure SingleFile");

    auto md5sum = common::fromMaybe(bdataToValueM<bstring, std::vector<char>>(bd.find("md5sum")),
                                     std::vector<char>{});

    return eLength.rightMap(
        [name, md5sum](const uint64_t length) {
            return SingleFile{common::fromMaybe(name, ""s), length, md5sum};
        });
}

template <> Either<std::string, FileInfo> fromBdict<FileInfo>(const bdict &bd)
{
    auto mLength = bdataToValueM<bint, uint64_t>(bd.find("length"));
    auto eLength = maybeToEither(mLength, "Missing length field for structure SingleFile");

    auto md5sum = common::fromMaybe(bdataToValueM<bstring, std::vector<char>>(bd.find("md5sum")),
                                     std::vector<char>{});

    auto mBdPath = bdataToValueM<blist, std::vector<bdata>>(bd.find("path"));

    auto f = [](const auto &bd) -> Maybe<std::string>
    { return toBstring(bd).map(mem_fn(&bstring::to_string)); };
    auto mPath = mBdPath.flatMap([f](const auto &v) -> Maybe<std::vector<std::string>>
                                   { return mmapVector<bdata, std::string>(v, f); });
    std::vector<std::string> path;
    if (!mPath.hasValue)
    {
        return left("Missing path field for structure FileInfo"s);
    }
    else
    {
        path = mPath.value;
    }

    return eLength.rightMap(
        [md5sum, path](const uint64_t &length) {
            return FileInfo{length, md5sum, path};
        });
}

template <> Either<std::string, FileInfo> fromBdata<FileInfo>(const bdata &bd)
{
    const auto eBdId =
        maybeToEither(common::toBdict(bd), "Bencode should start with bdict for FileInfo");

    return eBdId.rightFlatMap(fromBdict<FileInfo>);
}

template <> Either<std::string, MultiFile> fromBdict<MultiFile>(const bdict &bd)
{
    auto name =
        common::fromMaybe(bdataToValueM<bstring, std::vector<char>>(bd.find("name"))
                               .map([](const auto &v) { return std::string(v.begin(), v.end()); }),
                           ""s);

    auto eBdFiles = maybeToEither(bdataToValueM<blist, std::vector<bdata>>(bd.find("files")),
                                     "Missing files field for structure MultiFile");

    auto eFiles = eBdFiles.rightFlatMap(
        [](const auto &v) -> Either<std::string, std::vector<FileInfo>>
        { return mmapVector<bdata, std::string, FileInfo>(v, fromBdata<FileInfo>); });

    return eFiles.rightMap([name](const auto &files) { return MultiFile{name, files}; });
}

template <> Either<std::string, InfoDict> fromBdict<InfoDict>(const bdict &bd)
{
    auto mPieceLength =
        toMaybe(bd.find("piece length")).flatMap(toBint).map(std::mem_fn(&bint::value));
    uint64_t pieceLength;
    if (!mPieceLength.hasValue)
    {
        return left("Attribute piece length missing from info"s);
    }
    else
    {
        pieceLength = mPieceLength.value;
    }

    auto mPieces = toMaybe(bd.find("pieces")).flatMap(toBstring).map(mem_fn(&bstring::value));
    std::vector<char> pieces;
    if (!mPieces)
    {
        return left("Attribute pieces missing from info"s);
    }
    else
    {
        pieces = mPieces.value;
    }

    auto publish = toMaybe(bd.find("private")).flatMap(toBint).map(std::mem_fn(&bint::value));

    const auto eSingleFile = fromBdict<SingleFile>(bd);
    const auto eMultiFile = fromBdict<MultiFile>(bd);

    const auto eFileMode = eitherOf(eSingleFile, eMultiFile);

    const auto infoDict = eFileMode.rightMap(
        [pieceLength, pieces, publish](const auto& fileMode)
        {
            return InfoDict{pieceLength, pieces, common::maybeToOpt(publish),
                            eitherToVariant(fileMode)};
        });
    return infoDict;
}

template <> Either<std::string, InfoDict> fromBdata<InfoDict>(const bdata &bd)
{
    const auto eBdId =
        maybeToEither(common::toBdict(bd), "Bencode should start with bdict for InfoDict");

    return eBdId.rightFlatMap(fromBdict<InfoDict>);
}

bdict toBdict(const FileInfo &fi)
{
    std::map<bstring, bdata> kvMap;
    const bstring keyL("length");
    const bint valL(fi.length);
    kvMap.insert({keyL, bdata(valL)});

    if (!fi.md5sum.empty())
    {
        const bstring keyMd5("md5sum");
        const bstring valMd5(fi.md5sum);
        kvMap.insert({keyMd5, valMd5});
    }

    const bstring keyPs("path");
    auto paths =
        mapVector<std::string, bdata>(fi.path, [](const auto &s) { return bdata(bstring(s)); });
    const blist valPs(paths);
    kvMap.insert({keyPs, bdata(valPs)});

    return {kvMap};
}

bdict toBdict(const SingleFile &sf)
{
    std::map<bstring, bdata> kvMap;
    const bstring keyL("length");
    const bint valL(sf.length);
    kvMap.insert({keyL, bdata(valL)});

    if (!sf.md5sum.empty())
    {
        const bstring keyMd5("md5sum");
        const bstring valMd5(sf.md5sum);
        kvMap.insert({keyMd5, valMd5});
    }

    const bstring keyName("name");
    const bstring valName(sf.name);
    kvMap.insert({keyName, valName});

    return bdict(kvMap);
}

bdict toBdict(const MultiFile &mf)
{
    std::map<bstring, bdata> kvMap;
    const bstring keyFs("files");

    std::function<bdata(FileInfo)> fiLam = [](const auto &fi) { return bdata(toBdict(fi)); };
    blist valFs(mapVector(mf.files, fiLam));

    const bstring keyName("name");
    const bstring valName(mf.name);
    kvMap.insert({keyName, bdata(valName)});

    kvMap.insert({keyFs, bdata(valFs)});
    return bdict(kvMap);
}

bdict toBdict(const InfoDict &id)
{
    bstring keyPl("piece length");
    bdata valPl(id.pieceLength);

    std::string keyPcs("pieces");
    bstring valPcs(id.pieces);

    std::function<bdict(SingleFile)> sfLam = [](const auto &sf) { return toBdict(sf); };
    std::function<bdict(MultiFile)> mfLam = [](const auto &mf) { return toBdict(mf); };
    bdict fm = eitherToVal<SingleFile, MultiFile, bdict>(id.fileMode, sfLam, mfLam);

    fm.insert(keyPl, valPl);
    fm.insert(keyPcs, valPcs);

    if (id.publish)
    {
        std::string keyPrvt("private");
        bint valPrvt(id.publish.value());
        fm.insert(keyPrvt, valPrvt);
    }

    return fm;
}

template <> Either<std::string, MetaInfo> fromBdata<MetaInfo>(const bdata &bd)
{
    auto mBdMi = common::toBdict(bd);

    auto toMetaInfo = [](const bdict &m) -> Either<std::string, MetaInfo>
    {
        auto mAnn =
            toMaybe(m.find("announce")).flatMap(toBstring).map(mem_fn(&bstring::to_string));

        auto makeAnnounceList = [](const blist &bl) -> std::vector<std::string>
        {
            auto blVal = bl.values();

            std::vector<std::string> result;
            for (const auto &item : blVal)
            {
                Maybe<std::vector<bdata>> innerVec =
                    bdataToValueM<blist, std::vector<bdata>>(&item);
                if (!innerVec.hasValue || innerVec.value.empty())
                {
                    continue;
                }

                for (const auto &innerItem : innerVec.value)
                {
                    const auto mstr = innerItem.get_bstring();

                    if (mstr)
                    {
                        result.emplace_back(mstr->to_string());
                    }
                }
            }

            return result;
        };
        auto annL =
            fromMaybe(toMaybe(m.find("announce-list")).flatMap(toBlist).map(makeAnnounceList),
                       std::vector<std::string>{});

        auto mCd =
            toMaybe(m.find("creation date")).flatMap(toBint).map(std::mem_fn(&bint::value));

        auto mCm =
            toMaybe(m.find("comment")).flatMap(toBstring).map(mem_fn(&bstring::to_string));

        auto mCb =
            toMaybe(m.find("created by")).flatMap(toBstring).map(mem_fn(&bstring::to_string));

        auto mEc =
            toMaybe(m.find("encoding")).flatMap(toBstring).map(mem_fn(&bstring::to_string));

        auto eId =
            maybeToEither(toMaybe(m.find("info")), "Missing field info in meta info bdict")
                .rightFlatMap(fromBdata<InfoDict>);

        auto makeMetaInfo = [mAnn, annL, mCb, mCd, mCm, mEc,
                              eId](const auto &inf) -> Either<std::string, MetaInfo>
        {
            if (!mAnn.hasValue)
            {
                return left("Missing field announce in meta info dict"s);
            }
            const MetaInfo mi = {mAnn.value,
                                 annL,
                                 common::maybeToOpt(mCd),
                                 common::maybeToOpt(mCm),
                                 common::maybeToOpt(mCb),
                                 common::maybeToOpt(mEc),
                                 inf};
            return right(mi);
        };
        return eId.rightFlatMap(makeMetaInfo);
    };

    auto eBdMi = maybeToEither(mBdMi, "Bencode should start with bdict for MetaInfo");
    return eBdMi.rightFlatMap(toMetaInfo);
}

} // namespace fractals::torrent
