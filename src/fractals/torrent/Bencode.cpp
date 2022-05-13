//
// Created by Ferdinand on 5/13/2022.
//

#include "fractals/torrent/Bencode.h"

namespace fractals::torrent {
    
    template<>
    Either<std::string, SingleFile> from_bdict<SingleFile>(const bdict &bd) {
        auto name = to_maybe(bd.find("name"))
                .flatMap(to_bstring)
                .map(mem_fn(&bstring::to_string));
        auto m_length = to_maybe(bd.find("length"))
                .flatMap(to_bint)
                .map(mem_fn(&bint::value));
        auto e_length = maybe_to_either(m_length,"Missing length field for structure SingleFile");

        auto md5sum = to_maybe(bd.find("md5sum"))
                .flatMap(to_bstring)
                .map(mem_fn(&bstring::value));

        return e_length.rightMap(
                [name,md5sum](const long long length) { return SingleFile {name,length,md5sum}; }
        );
    }

    template <>
    Either<std::string,FileInfo> from_bdict<FileInfo>(const bdict &bd) {
        auto m_length = to_maybe(bd.find("length"))
                .flatMap(to_bint)
                .map(mem_fn(&bint::value));
        auto e_length = maybe_to_either(m_length,"Missing length field for structure SingleFile");

        auto md5sum = to_maybe(bd.find("md5sum"))
                .flatMap(to_bstring)
                .map(mem_fn(&bstring::value));

        auto m_bdpath = to_maybe(bd.find("path"))
                .flatMap(to_blist)
                .map(mem_fn(&blist::value));

        auto f = [](const auto &bd) -> Maybe<string> { return to_bstring(bd).map(mem_fn(&bstring::to_string)); };
        auto m_path = m_bdpath.flatMap([f](const auto &v) -> Maybe<vector<string>> {
            return mmap_vector<bdata,string>(v, f);
        });
        vector<string> path;
        if(!m_path.hasValue) { return left("Missing path field for structure FileInfo"s); }
        else                 { path = m_path.value; }

        return e_length.rightMap(
                [md5sum,path](const long long &length) { return FileInfo {length,md5sum,path}; }
        );
    }

    template <>
    Either<std::string,FileInfo> from_bdata<FileInfo>(const bdata &bd) {
        const auto e_bd_id = maybe_to_either(to_maybe(bd.get_bdict()), "Bencode should start with bdict for FileInfo");

        return e_bd_id.rightFlatMap(from_bdict<FileInfo>);
    }


    template <>
    Either<std::string,MultiFile> from_bdict<MultiFile>(const bdict &bd) {
        auto name = to_maybe(bd.find("name"))
                .flatMap(to_bstring)
                .map(mem_fn(&bstring::to_string));

        auto e_bdfiles = maybe_to_either(
                to_maybe(bd.find("files"))
                        .flatMap(to_blist)
                        .map(mem_fn(&blist::value))
                ,"Missing files field for structure MultiFile");

        auto e_files = e_bdfiles.rightFlatMap([](const auto &v) -> Either<string,vector<FileInfo>> {
            return mmap_vector<bdata,string,FileInfo>(v,from_bdata<FileInfo>);
        });


        return e_files.rightMap(
                [name](const auto &files) { return MultiFile {name,files}; }
        );
    }

    template<>
    Either<std::string,InfoDict> from_bdict<InfoDict>(const bdict &bd) {
        auto m_piece_length = to_maybe(bd.find("piece length"))
                .flatMap(to_bint)
                .map(mem_fn(&bint::value));
        long long piece_length;
        if(!m_piece_length.hasValue)
        { return left("Attribute piece length missing from info"s);}
        else { piece_length = m_piece_length.value; }

        auto m_pieces = to_maybe(bd.find("pieces"))
                .flatMap(to_bstring)
                .map(mem_fn(&bstring::value));
        std::vector<char> pieces;
        if(!m_pieces)
        { return left("Attribute pieces missing from info"s);}
        else { pieces = m_pieces.value; }

        auto publish = to_maybe(bd.find("private"))
                .flatMap(to_bint)
                .map(mem_fn(&bint::value));

        const auto e_single_file = from_bdict<SingleFile>(bd);
        const auto e_multi_file  = from_bdict<MultiFile>(bd);

        const auto e_file_mode = either_of(e_single_file,e_multi_file);

        const auto infoDict = e_file_mode.rightMap(
                [piece_length,pieces,publish](auto file_mode)
                { return InfoDict {piece_length,pieces,publish,file_mode}; }
        );
        return infoDict;
    }

    template<>
    Either<std::string,InfoDict> from_bdata<InfoDict>(const bdata &bd) {
        const auto e_bd_id = maybe_to_either(to_maybe(bd.get_bdict()), "Bencode should start with bdict for InfoDict");

        return e_bd_id.rightFlatMap(from_bdict<InfoDict>);
    }

    bdict to_bdict(const FileInfo &fi) {
        const bstring key_l("length");
        const bint val_l(fi.length);

        const bstring key_ps("path");
        auto paths = map_vector<string,bdata>(fi.path,[](const auto &s) { return bdata(bstring(s)); });
        const blist val_ps(paths);

        map<bstring,bdata> kv_map;
        kv_map.insert({key_l,bdata(val_l)});
        kv_map.insert({key_ps,bdata(val_ps)});

        return {kv_map };
    }

    bdict to_bdict(const SingleFile &sf) {
        map<bstring, bdata> kv_map;
        const bstring key_l("length");
        const bint val_l(sf.length);
        kv_map.insert({key_l,bdata(val_l)});

        if(sf.md5sum.hasValue) {
            const bstring key_md5("md5sum");
            const bstring val_md5(sf.md5sum.value);
            kv_map.insert({key_md5,val_md5});
        }

        if(sf.name.hasValue) {
            const bstring key_name("name");
            const bstring val_name(sf.name.value);
            kv_map.insert({key_name,val_name});
        }


        return {kv_map };
    }

    bdict to_bdict(const MultiFile &mf) {
        map<bstring, bdata> kv_map;
        const bstring key_fs("files");

        std::function<bdata(FileInfo)> fi_lam = [](const auto &fi) { return bdata(to_bdict(fi)); };
        blist val_fs(map_vector(mf.files, fi_lam));

        if(mf.name.hasValue) {
            const bstring key_name("name");
            const bstring val_name(mf.name.value);
            kv_map.insert({key_name,bdata(val_name)});
        }

        kv_map.insert({key_fs,bdata(val_fs)});
        return {kv_map };
    }

    bdict to_bdict(const InfoDict &id) {
        string key_pl("piece length");
        bint val_pl(id.piece_length);

        string key_pcs("pieces");
        bstring val_pcs(id.pieces);

        std::function<bdict(SingleFile)> sf_lam = [](const auto &sf){ return to_bdict(sf); };
        std::function<bdict(MultiFile)> mf_lam = [](const auto &mf){ return to_bdict(mf); };
        bdict fm = either_to_val<SingleFile,MultiFile,bdict>(id.file_mode, sf_lam,mf_lam);
        fm.insert({key_pl,val_pl});
        fm.insert({key_pcs,val_pcs});

        if(id.publish.hasValue) {
            string key_prvt("private");
            bint val_prvt(id.publish.value);
            fm.insert({key_prvt,val_prvt});
        }

        return fm;
    }

    template <>
    Either<string,MetaInfo> from_bdata<MetaInfo>(const bdata &bd) {
        auto m_bd_mi = to_maybe(bd.get_bdict());

        auto to_metainfo = [](const bdict &m) -> Either<string,MetaInfo> {
            auto m_ann = to_maybe(m.find("announce"))
                    .flatMap(to_bstring)
                    .map(mem_fn(&bstring::to_string));

            auto make_announce_list =
                    [](const blist& bl) -> Maybe<vector<vector<string>>> {
                            auto bl_val = bl.value();

                            auto bdata_to_vec = [](const auto &bd) -> Maybe<vector<string>> {
                                Maybe<vector<bdata>> outer_vec = to_blist(bd).map(mem_fn(&blist::value));

                                auto bdata_to_string = [](const auto &bd) -> Maybe<string> {
                                    return to_bstring(bd).map(mem_fn(&bstring::to_string));
                                };

                                Maybe<vector<string>> vec_of_string =
                                        outer_vec.flatMap([bdata_to_string](auto &v) {
                                            return mmap_vector<bdata,string>(v,bdata_to_string);
                                        });

                                return vec_of_string;
                            };

                            return mmap_vector<bdata,vector<string>>(bl_val,bdata_to_vec);
                    };
            auto m_ann_l = to_maybe(m.find("announce-list"))
                    .flatMap(to_blist)
                    .flatMap(make_announce_list);

            auto m_cd = to_maybe(m.find("creation date"))
                    .flatMap(to_bint)
                    .map(mem_fn(&bint::value));

            auto m_cm = to_maybe(m.find("comment"))
                    .flatMap(to_bstring)
                    .map(mem_fn(&bstring::to_string));

            auto m_cb = to_maybe(m.find("created by"))
                    .flatMap(to_bstring)
                    .map(mem_fn(&bstring::to_string));

            auto m_ec = to_maybe(m.find("encoding"))
                    .flatMap(to_bstring)
                    .map(mem_fn(&bstring::to_string));

            auto e_id = maybe_to_either(to_maybe(m.find("info")),"Missing field info in meta info bdict")
                    .rightFlatMap(from_bdata<InfoDict>);

            auto make_metainfo = [m_ann,m_ann_l,m_cb,m_cd,m_cm,m_ec,e_id](const auto &inf) -> Either<string,MetaInfo> {
                if(!m_ann.hasValue) { return left("Missing field announce in meta info dict"s); }
                const MetaInfo mi = { m_ann.value,m_ann_l,m_cd,m_cm,m_cb,m_ec,inf };
                return right(mi);
            };
            return e_id.rightFlatMap(make_metainfo);
        };

        auto e_bd_mi = maybe_to_either(m_bd_mi, "Bencode should start with bdict for MetaInfo");
        return e_bd_mi.rightFlatMap(to_metainfo);
    }

}
