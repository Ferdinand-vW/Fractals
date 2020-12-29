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
        static Either<string,MetaInfo> from_bencode(bencode::bdata bd) {
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

                auto make_metainfo = [m_ann_l,m_cb,m_cd,m_cm,m_ec](const auto &ann) -> Either<string,MetaInfo> {
                    const MetaInfo mi = { ann,m_ann_l,m_cd,m_cm,m_cb,m_ec }; 
                    return right(mi);
                };
                return e_ann.rightFlatMap(make_metainfo);
            };

            auto e_bd_mi = maybe_to_either(m_bd_mi, "Bencode should start with bdict for MetaInfo");
            return e_bd_mi.rightFlatMap(to_metainfo);
        }

        // bencode::bdata to_bencode(MetaInfo mi) {

        // }

        // InfoDict from_bencode(bencode::bdata bd) {

        // }  

        // bencode::bdata to_bencode(InfoDict id) {
            
        // }

        // FileInfo from_bencode(bencode::bdata bd) {

        // }  

        // bencode::bdata to_bencode(FileInfo fi) {
            
        // }
};