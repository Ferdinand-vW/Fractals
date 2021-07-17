#include <fstream>
#include <functional>
#include <netinet/in.h>
#include <curl/curl.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>
#include <optional>

#include <boost/outcome/basic_result.hpp>
#include <bencode/bdata.h>
#include <bencode/bdict.h>
#include <bencode/bint.h>
#include <bencode/blist.h>
#include <bencode/bstring.h>
#include <bencode/error.h>
#include <bencode/decode.h>
#include <bencode/encode.h>
#include <neither/neither.hpp>

#include "app/Client.h"
#include "common/encode.h"
#include "common/maybe.h"
#include "common/utils.h"
#include "network/http/Announce.h"
#include "network/http/Tracker.h"
#include "network/http/Peer.h"
#include "torrent/BencodeConvert.h"
#include "torrent/MetaInfo.h"

std::ostream & operator<<(std::ostream& os, const TrackerResponse & s) {
    auto peers_str = intercalate(", ",map_vector<Peer,std::string>(s.peers, [](const Peer &p) {
        return "("s + p.peer_name + "," + p.peer_id.m_ip + "," + std::to_string(p.peer_id.m_port) + ")";
    }));
    os << "Tracker Reponse: " << endl;
    os << "{ tracker id: "+from_maybe(s.tracker_id,""s) << endl;
    os << ", complete: " + std::to_string(s.complete) << endl;
    os << ", incomplete: " + std::to_string(s.incomplete) << endl;
    os << ", interval: " + std::to_string(s.interval) << endl;
    os << ", min interval: " + std::to_string(s.min_interval) << endl;
    os << ", warning message: " + from_maybe(s.warning_message,""s) << endl;
    os << ", peers: " + peers_str << endl;
    os << "}" << endl;

    return os;
}

TrackerRequest makeTrackerRequest(const MetaInfo &mi) {
    bencode::bdict info_dict = BencodeConvert::to_bdict(mi.info);
    auto encoded = bencode::encode(info_dict);

    
    auto info_hash = sha1_encode(encoded);
    auto uri_info  = url_encode(info_hash);
    auto peer_id   = generate_peerId();
    auto str_peer_id = url_encode(peer_id);
    int port = 6882;
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

std::size_t writeTrackerResponseData(void *contents,std::size_t size,std::size_t nmemb,void *userp) {
    std::string *uptr = reinterpret_cast<string*>(userp);
    char * conts      = reinterpret_cast<char*>(contents);
    uptr->append(conts,size * nmemb);
    return size * nmemb;
}

std::string toHttpGetUrl(const TrackerRequest &tr) {
    std::string ann_base = tr.announce + "?";
    std::string ih_prm   = "info_hash="+tr.url_info_hash;
    std::string peer_prm = "peer_id="+tr.url_peer_id;
    std::string port_prm = "port="+std::to_string(tr.port);
    std::string upl_prm = "uploaded="+std::to_string(tr.uploaded);
    std::string dl_prm  = "downloaded="+std::to_string(tr.downloaded);
    std::string lft_prm = "left="+std::to_string(tr.left);
    std::string cmpt_prm = "compact="+std::to_string(tr.compact);

    std::string url = ann_base + intercalate("&", {ih_prm,peer_prm,port_prm,upl_prm,dl_prm,lft_prm,cmpt_prm});

    return url;
}

neither::Either<std::string, std::vector<Peer>> parsePeersDict(const blist &bl) {
    auto parse_peer = [](const bdata &bd) -> Either<std::string,Peer> {
        auto mpeer_dict = to_maybe(bd.get_bdict());
        if (!mpeer_dict.hasValue) { return neither::left("Could not coerce bdata to bdict for peer"s); }
        auto peer_dict = mpeer_dict.value;

        auto mpeer_id = to_maybe(peer_dict.find("peer id"))
                       .flatMap(to_bstring)
                       .map(mem_fn(&bstring::to_string));
        auto mip      = to_maybe(peer_dict.find("ip"))
                       .flatMap(to_bstring)
                       .map(mem_fn(&bstring::to_string));
        auto mport    = to_maybe(peer_dict.find("port"))
                       .flatMap(to_bint)
                       .map(mem_fn(&bint::value));
        
        if(!mpeer_id.hasValue) { return neither::left("Could not find field peer id in peers dictionary"s); }
        if(!mip.hasValue)      { return neither::left("Could not find field ip in peers dictionary"s); }
        if(!mport.hasValue)    { return neither::left("Could not find field port in peers dictionary"s); }

        return neither::right(Peer { mpeer_id.value, PeerId(mip.value, (uint)mport.value) });
    };

    return mmap_vector<bdata,std::string,Peer>(bl.value(),parse_peer);
}

neither::Either<std::string, std::vector<Peer>> parsePeersBin(vector<char> bytes) {
    if(bytes.size() % 6 != 0) { return neither::left("Peer binary data is not a multiple of 6"s); }

    std::vector<Peer> peers;
    for(int i = 0; i < bytes.size() - 6; i+=6) {
        struct sockaddr_in sa;
        char buffer[4];
        std::copy(bytes.begin()+i,bytes.begin()+i+4,buffer);
        char result[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,(void*)(&buffer[0]),result, sizeof result);
        
        std::string ip(result);
        ushort port = static_cast<unsigned char>(bytes[i+4]) * 256 + static_cast<unsigned char>(bytes[i+5]);

        auto peer_id = ip + ":" + std::to_string(port);
        peers.push_back(Peer { peer_id, PeerId(ip, port)});
    }

    return neither::right<std::vector<Peer>>(peers);
}


neither::Either<std::string, TrackerResponse> parseTrackerReponse(const bdict &bd) {
    //
    std::ofstream fs("err2.txt");
    fs << bd;
    fs.close();
    auto optfailure = bd.find("failure reason");
    std::string failure_reason;
    if(optfailure.has_value()) { 
        auto mfailure = to_maybe(optfailure).flatMap(to_bstring);
        return neither::left<std::string>(mfailure.value.to_string()); 
    }

    auto warning_message = to_maybe(bd.find("warning message"))
                           .flatMap(to_bstring)
                           .map(mem_fn(&bstring::to_string));
    
    auto minterval = to_maybe(bd.find("interval"))
                    .flatMap(to_bint)
                    .map(mem_fn(&bint::value));
    
    auto min_interval = to_maybe(bd.find("min interval"))
                       .flatMap(to_bint)
                       .map(mem_fn(&bint::value));

    auto tracker_id = to_maybe(bd.find("tracker id"))
                     .flatMap(to_bstring)
                     .map(mem_fn(&bstring::to_string));

    auto mcomplete = to_maybe(bd.find("complete"))
                    .flatMap(to_bint)
                    .map(mem_fn(&bint::value));

    auto mincomplete = to_maybe(bd.find("incomplete"))
                      .flatMap(to_bint)
                      .map(mem_fn(&bint::value));

    auto mpeers_unk = to_maybe(bd.find("peers"));
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

Announce toAnnounce(time_t now,const TrackerResponse &tr) {
    std::vector<PeerId> peerIds;
    std::transform(tr.peers.begin(),tr.peers.end(),std::back_insert_iterator(peerIds),[](auto &p) {
        return p.peer_id;
    });
    return Announce { now, tr.interval, tr.min_interval, peerIds };
}

neither::Either<std::string, TrackerResponse> sendTrackerRequest(const TrackerRequest &tr) {
    CURLcode res;
    curl_global_init(0);
    CURL *curl = curl_easy_init();
    std::string readBuffer;
    string url = toHttpGetUrl(tr);

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeTrackerResponseData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) { 
            std::string curl_err = curl_easy_strerror(res);
            /* always cleanup */ 
            curl_easy_cleanup(curl);
            return left("curl_easy_perform() failed: "+curl_err); 
        }
    }

    curl_easy_cleanup(curl);

    stringstream ss(readBuffer);
    auto mresp = bencode::decode<bencode::bdict>(ss);

    if (!mresp.has_value()) { return neither::left(mresp.error().message()); }

    auto resp = mresp.value();

    return parseTrackerReponse(resp);
}