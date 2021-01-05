#include <functional>
#include <memory>
#include <string>
#include <utility>
#include "bencode/bencode.h"
#include "torrent/MetaInfo.h"
#include "maybe.h"
#include "neither/neither.hpp"

using namespace std;
using namespace bencode;

using namespace neither;

class BencodeConvert {
    public:
        template <class A>
        static Either<string,A> from_bdata(bencode::bdata bd);
        template <class A>
        static Either<string,A> from_bdict(bencode::bdict bd);

        static Either<string,MetaInfo> from_bdata(bencode::bdata bd) {
            auto m_bd_mi = to_maybe(bd.get_bdict());

            auto to_metainfo = [](const bdict &m) -> Either<string,MetaInfo> { 
                auto m_ann = to_maybe(m.find("announce"))
                            .flatMap(to_bstring)
                            .map(mem_fn(&bstring::value))
                            .map([](auto const &s) { return s; });
                auto e_ann = maybe_to_either(m_ann,"Could not find announce in bencode data");

                auto make_announce_list = 
                    [](const blist& bl) -> Maybe<vector<vector<string>>> {
                        auto bl_val = bl.value();
                        
                        auto bdata_to_vec = [](const auto &bd) -> Maybe<vector<string>> {
                            Maybe<vector<bdata>> outer_vec = to_blist(bd).map(mem_fn(&blist::value));
                            
                            auto bdata_to_string = [](const auto &bd) -> Maybe<string> {
                                return to_bstring(bd).map(mem_fn(&bstring::value)); 
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
                           .map(mem_fn(&bstring::value));

                auto m_cb = to_maybe(m.find("created by"))
                            .flatMap(to_bstring)
                            .map(mem_fn(&bstring::value));
                
                auto m_ec = to_maybe(m.find("encoding"))
                          .flatMap(to_bstring)
                          .map(mem_fn(&bstring::value));

                auto e_id = maybe_to_either(to_maybe(m.find("info")),"Missing field info in meta info bdict")
                            .rightFlatMap(from_bdata<InfoDict>);

                auto make_metainfo = [m_ann_l,m_cb,m_cd,m_cm,m_ec,e_id](const auto &ann) -> Either<string,MetaInfo> {
                    return e_id.rightMap([ann,m_ann_l,m_cb,m_cd,m_cm,m_ec](const InfoDict& inf) {
                        const MetaInfo mi = { ann,m_ann_l,m_cd,m_cm,m_cb,m_ec,inf };
                        return mi;
                    }); 
                };
                return e_ann.rightFlatMap(make_metainfo);
            };

            auto e_bd_mi = maybe_to_either(m_bd_mi, "Bencode should start with bdict for MetaInfo");
            return e_bd_mi.rightFlatMap(to_metainfo);
        }

        // bencode::bdata to_bencode(MetaInfo mi) {

        // }

        template <>
        Either<std::string,SingleFile> from_bdict<SingleFile>(bencode::bdict bd) {
            const string s("");
            return left(s);
        }

        template <>
        Either<std::string,MultiFile> from_bdict<MultiFile>(bencode::bdict bd) {
            const string s("");
            return left(s);
        }

        template<>
        Either<std::string,InfoDict> from_bdata<InfoDict>(bencode::bdata bd) {
            const auto e_bd_id = maybe_to_either(to_maybe(bd.get_bdict()), "Bencode should start with bdict for InfoDict");

            return e_bd_id.rightFlatMap(from_bdict<InfoDict>);
        }

        template<>
        Either<std::string,InfoDict> from_bdict<InfoDict>(bencode::bdict bd) {
            auto m_piece_length = to_maybe(bd.find("piece length"))
                                .flatMap(to_bint)
                                .map(mem_fn(&bint::value));
            int piece_length;
            if(!m_piece_length.hasValue) 
                 { return left("Attribute piece length missing from info"s);}
            else { piece_length = m_piece_length.value; }
            
            auto m_pieces = to_maybe(bd.find("pieces"))
                            .flatMap(to_bstring)
                            .map(mem_fn(&bstring::value));
            std::string pieces;
            if(!m_pieces)
                 { return left("Attribute pieces missing from info"s);}
            else { pieces = m_pieces.value; }
            
            auto publish = to_maybe(bd.find("private"))
                            .flatMap(to_bint)
                            .map(mem_fn(&bint::value));

            const auto e_single_file = from_bdict<SingleFile>(bd);
            const auto e_multi_file  = from_bdict<MultiFile>(bd);
            typedef Either<string,Either<SingleFile,MultiFile>> file_mode_type;

            const auto e_file_mode = either_of(e_single_file,e_multi_file);

            const auto infoDict = e_file_mode.rightMap(
                [piece_length,pieces,publish](auto file_mode) 
                    { return InfoDict {piece_length,pieces,publish,file_mode}; }
            );
            return infoDict;
        }  

        // bencode::bdata to_bencode(InfoDict id) {
            
        // }

        // FileInfo from_bencode(bencode::bdata bd) {

        // }  

        // bencode::bdata to_bencode(FileInfo fi) {
            
        // }
};