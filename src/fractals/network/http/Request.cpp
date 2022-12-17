#include "fractals/network/http/Request.h"

#include <arpa/inet.h>
#include <fractals/app/Client.h>
#include <fractals/torrent/Bencode.h>
#include <fractals/torrent/MetaInfo.h>
#include <fractals/common/encode.h>
#include <fractals/common/maybe.h>
#include <fractals/common/utils.h>

#include <netinet/in.h>
#include <ostream>

namespace fractals::network::http
{
    std::ostream& operator<<(std::ostream& os, const TrackerRequest& tr)
    {
        auto info_hash_str = common::concat(tr.info_hash);


        os << "Tracker Request: " << std::endl;
        os << "{ announce: "+tr.announce << std::endl;
        os << ", info_hash:"+common::bytes_to_hex(tr.info_hash) << std::endl;
        os << ", url_info_hash:"+tr.url_info_hash << std::endl;
        os << ", peer_id:"+common::bytes_to_hex(tr.peer_id) << std::endl;
        os << ", url_peer_id:"+tr.url_peer_id << std::endl;
        os << ", port:" << tr.port << std::endl;
        os << ", uploaded:" << tr.uploaded << std::endl;
        os << ", downloaded:" << tr.downloaded << std::endl;
        os << ", left:" << tr.left << std::endl;
        os << ", compact:" << tr.compact << std::endl;
        os << "}" << std::endl;

        return os;
    }

    bool TrackerRequest::operator==(const TrackerRequest& tr) const
    {
        return announce == tr.announce
            && std::equal(info_hash.begin(), info_hash.end(), tr.info_hash.begin())
            && url_info_hash == tr.url_info_hash
            && std::equal(peer_id.begin(), peer_id.end(), tr.peer_id.begin())
            && url_peer_id == tr.url_peer_id
            && port == tr.port
            && uploaded == tr.uploaded
            && downloaded == tr.downloaded
            && left == tr.left
            && compact == tr.compact;
    }

    std::ostream & operator<<(std::ostream& os, const TrackerResponse & s) {
        auto peers_str = common::intercalate(", ",common::map_vector<Peer,std::string>(s.peers, [](const Peer &p) {
            return "(["s + p.peer_name + "]" + p.peer_id.m_ip + ":" + std::to_string(p.peer_id.m_port) + ")";
        }));
        os << "Tracker Response: " << std::endl;
        os << "{ tracker id: "+common::from_maybe(s.tracker_id,"<empty>"s) << std::endl;
        os << ", complete: " + std::to_string(s.complete) << std::endl;
        os << ", incomplete: " + std::to_string(s.incomplete) << std::endl;
        os << ", interval: " + std::to_string(s.interval) << std::endl;
        os << ", min interval: " + std::to_string(s.min_interval) << std::endl;
        os << ", warning message: " + common::from_maybe(s.warning_message,"<empty>"s) << std::endl;
        os << ", peers: " + peers_str << std::endl;
        os << "}" << std::endl;

        return os;
    }

    bool TrackerResponse::operator==(const TrackerResponse& tr) const
    {
        return warning_message == tr.warning_message
            && interval == tr.interval
            && min_interval == tr.min_interval
            && tracker_id == tr.tracker_id
            && complete == tr.complete
            && incomplete == tr.incomplete
            && std::equal(peers.begin(), peers.end(), tr.peers.begin());

    }

    TrackerRequest makeTrackerRequest(const torrent::MetaInfo &mi) {
        bencode::bdict info_dict = torrent::to_bdict(mi.info);

        auto encoded = bencode::encode(info_dict);
        auto info_hash = common::sha1_encode(encoded);
        //tracker expects url encoded sha1 hash of info
        auto uri_info  = common::url_encode(info_hash);
        auto peer_id   = app::generate_peerId(); //unique id for this instance
        //peer id must also be url encoded
        auto str_peer_id = common::url_encode(peer_id);
        int port = 6882; //port used by this app
        int uploaded = 0;
        int downloaded = 0;
        int left = 0;
        int compact = 0;
        const TrackerRequest request = TrackerRequest { 
                                    mi.announce
                                    , info_hash, uri_info
                                    , peer_id, str_peer_id, port
                                    , uploaded, downloaded
                                    , left, compact
                                    };
        return request;
    }

    std::string TrackerRequest::toHttpGetUrl() const {
        std::string ann_base = announce + "?";
        std::string ih_prm   = "info_hash=" +url_info_hash;
        std::string peer_prm = "peer_id="   +url_peer_id;
        std::string port_prm = "port="      +std::to_string(port);
        std::string upl_prm  = "uploaded="  +std::to_string(uploaded);
        std::string dl_prm   = "downloaded="+std::to_string(downloaded);
        std::string lft_prm  = "left="      +std::to_string(left);
        std::string cmpt_prm = "compact=1";//   +std::to_string(tr.compact);

        std::string url = ann_base + common::intercalate("&", {ih_prm,peer_prm,port_prm,upl_prm,dl_prm,lft_prm,cmpt_prm});
        return url;
    }

    //tracker response peer model in dictionary format
    //keys are peer id, ip and port
    neither::Either<std::string, std::vector<Peer>> parsePeersDict(const blist &bl) {
        auto parse_peer = [](const bdata &bd) -> Either<std::string,Peer> {
            auto mpeer_dict = common::to_maybe(bd.get_bdict());
            if (!mpeer_dict.hasValue) { return neither::left("Could not coerce bdata to bdict for peer"s); }
            auto peer_dict = mpeer_dict.value;

            auto mpeer_id = common::to_maybe(peer_dict.find("peer id"))
                        .flatMap(to_bstring)
                        .map(mem_fn(&bstring::to_string));
            auto mip      = common::to_maybe(peer_dict.find("ip"))
                        .flatMap(to_bstring)
                        .map(mem_fn(&bstring::to_string));
            auto mport    = common::to_maybe(peer_dict.find("port"))
                        .flatMap(to_bint)
                        .map(std::mem_fn(&bint::value));
            
            if(!mpeer_id.hasValue) { return neither::left("Could not find field peer id in peers dictionary"s); }
            if(!mip.hasValue)      { return neither::left("Could not find field ip in peers dictionary"s); }
            if(!mport.hasValue)    { return neither::left("Could not find field port in peers dictionary"s); }

            return neither::right(Peer { mpeer_id.value, PeerId(mip.value, (uint)mport.value) });
        };

        return common::mmap_vector<bdata,std::string,Peer>(bl.values(),parse_peer);
    }

    //tracker response peer model in binary format
    //multiples of 6 bytes. First 4 bytes are ip and remaining 2 bytes are port. big endian notation 
    neither::Either<std::string, std::vector<Peer>> parsePeersBin(std::vector<char> bytes) {
        if(bytes.size() % 6 != 0) { return neither::left("Peer binary data is not a multiple of 6"s); }

        std::vector<Peer> peers;
        for(int i = 0; i < bytes.size(); i+=6) {
            struct sockaddr_in sa;
            char buffer[4];
            std::copy(bytes.begin()+i,bytes.begin()+i+4,buffer);
            char result[INET_ADDRSTRLEN]; //16

            //parse first 4 bytes to ip address
            inet_ntop(AF_INET,(void*)(&buffer[0]),result, sizeof result);
            
            std::string ip(result);
            //convert remaining 2 bytes to port
            ushort port = static_cast<unsigned char>(bytes[i+4]) * 256 + static_cast<unsigned char>(bytes[i+5]);

            auto peer_id = ip + ":" + std::to_string(port);
            peers.push_back(Peer { peer_id, PeerId(ip, port)});
        }

        return neither::right<std::vector<Peer>>(peers);
    }


    neither::Either<std::string, TrackerResponse> TrackerResponse::decode(const bdict &bd) {
        auto optfailure = bd.find("failure reason");
        std::string failure_reason;
        if(optfailure) { 
            auto mfailure = common::to_maybe(optfailure).flatMap(common::to_bstring);
            return neither::left<std::string>(mfailure.value.to_string()); 
        }

        auto warning_message = common::to_maybe(bd.find("warning message"))
                            .flatMap(to_bstring)
                            .map(mem_fn(&bstring::to_string));
        
        auto minterval = common::to_maybe(bd.find("interval"))
                        .flatMap(to_bint)
                        .map(std::mem_fn(&bint::value));
        
        auto min_interval = common::to_maybe(bd.find("min interval"))
                        .flatMap(to_bint)
                        .map(std::mem_fn(&bint::value));

        auto tracker_id = common::to_maybe(bd.find("tracker id"))
                        .flatMap(to_bstring)
                        .map(std::mem_fn(&bstring::to_string));

        auto mcomplete = common::to_maybe(bd.find("complete"))
                        .flatMap(to_bint)
                        .map(std::mem_fn(&bint::value));

        auto mincomplete = common::to_maybe(bd.find("incomplete"))
                        .flatMap(to_bint)
                        .map(std::mem_fn(&bint::value));

        auto mpeers_unk = common::to_maybe(bd.find("peers"));
        auto mpeers_dict = mpeers_unk.flatMap(to_blist);
        auto mpeers_bin = mpeers_unk.flatMap(to_bstring)
                        .map(mem_fn(&bstring::value));

        std::vector<Peer> peers;
        if(mpeers_dict.hasValue) { 
            auto epeers = parsePeersDict(mpeers_dict.value);
            if (epeers.isLeft) { return left<std::string>(epeers.leftValue); }
            else { peers = epeers.rightValue; }
        }
        else {
            if(mpeers_bin.hasValue) {
                auto epeers = parsePeersBin(mpeers_bin.value);
                if(epeers.isLeft) { return left<std::string>(epeers.leftValue); }
                else { peers = epeers.rightValue; }
            }
            else {
                return neither::left("Could not find field peers in tracker response"s);
            }
        }

        if(!minterval.hasValue) { return neither::left("Could not find field interval in tracker response"s); }
        if(!mcomplete.hasValue) { return neither::left("Could not find field complete in tracker respone"s); }
        if(!mincomplete.hasValue) { return neither::left("Could not find field incomplete in tracker response"s); }
        
        return neither::right(
                TrackerResponse { warning_message
                                , (int)minterval.value
                                , (int)min_interval
                                , tracker_id
                                , (int)mcomplete.value
                                , (int)mincomplete.value
                                , peers
                                });                   
        
    }

    Announce TrackerResponse::toAnnounce(time_t now) const {
        std::vector<PeerId> peerIds;
        std::transform(peers.begin(),peers.end(),std::back_insert_iterator(peerIds),[](auto &p) {
            return p.peer_id;
        });
        return Announce { now, interval, min_interval, peerIds };
    }
}