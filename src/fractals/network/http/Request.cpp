#include "fractals/network/http/Request.h"
#include "fractals/common/Tagged.h"
#include "fractals/persist/Models.h"

#include <arpa/inet.h>
#include <bencode/encode.h>
#include <fractals/app/Client.h>
#include <fractals/common/encode.h>
#include <fractals/common/maybe.h>
#include <fractals/common/utils.h>
#include <fractals/torrent/Bencode.h>
#include <fractals/torrent/MetaInfo.h>

#include <netinet/in.h>
#include <ostream>
#include <spdlog/spdlog.h>
#include <sstream>

namespace fractals::network::http
{
std::ostream &operator<<(std::ostream &os, const TrackerRequest &tr)
{
    auto infoHashStr = common::concat<20>(tr.infoHash.underlying);

    os << "Tracker Request: " << std::endl;
    os << "{ announce: " + tr.announce << std::endl;
    os << ", infoHash:" + common::bytesToHex<20>(tr.infoHash.underlying) << std::endl;
    os << ", urlInfoHash:" + tr.urlInfoHash << std::endl;
    os << ", appId:" + common::bytesToHex<20>(tr.appId.underlying) << std::endl;
    os << ", urlAppId:" + tr.urlAppId << std::endl;
    os << ", port:" << tr.port << std::endl;
    os << ", uploaded:" << tr.uploaded << std::endl;
    os << ", downloaded:" << tr.downloaded << std::endl;
    os << ", left:" << tr.left << std::endl;
    os << ", compact:" << tr.compact << std::endl;
    os << "}" << std::endl;

    return os;
}

bool TrackerRequest::operator==(const TrackerRequest &tr) const
{
    return announce == tr.announce &&
           std::equal(infoHash.underlying.begin(), infoHash.underlying.end(),
                      tr.infoHash.underlying.begin()) &&
           urlInfoHash == tr.urlInfoHash &&
           std::equal(appId.underlying.begin(), appId.underlying.end(),
                      tr.appId.underlying.begin()) &&
           urlAppId == tr.urlAppId && port == tr.port && uploaded == tr.uploaded &&
           downloaded == tr.downloaded && left == tr.left && compact == tr.compact;
}

std::ostream &operator<<(std::ostream &os, const TrackerResponse &s)
{
    auto peersStr = common::intercalate(
        ", ", common::mapVector<Peer, std::string>(s.peers,
                                                   [](const Peer &p)
                                                   {
                                                       return "(["s + p.name + "]" + p.id.ip + ":" +
                                                              std::to_string(p.id.port) + ")";
                                                   }));
    os << "Tracker Response: " << std::endl;
    os << "{ tracker id: " + common::fromMaybe(s.trackerId, "<empty>"s) << std::endl;
    os << ", complete: " + std::to_string(s.complete) << std::endl;
    os << ", incomplete: " + std::to_string(s.incomplete) << std::endl;
    os << ", interval: " + std::to_string(s.interval) << std::endl;
    os << ", min interval: " + std::to_string(s.minInterval) << std::endl;
    os << ", warning message: " + common::fromMaybe(s.warningMessage, "<empty>"s) << std::endl;
    os << ", peers: " + peersStr << std::endl;
    os << "}" << std::endl;

    return os;
}

bool TrackerResponse::operator==(const TrackerResponse &tr) const
{
    return warningMessage == tr.warningMessage && interval == tr.interval &&
           minInterval == tr.minInterval && trackerId == tr.trackerId && complete == tr.complete &&
           incomplete == tr.incomplete && std::equal(peers.begin(), peers.end(), tr.peers.begin());
}

TrackerRequest::TrackerRequest(const std::string &announce, const torrent::MetaInfo &mi,
                               const common::AppId &appId)
    : announce(announce),
      infoHash(common::sha1_encode<20>(bencode::encode(torrent::toBdict(mi.info)))),
      urlInfoHash(common::urlEncode<20>(infoHash.underlying)), appId(appId),
      urlAppId(common::urlEncode<20>(appId.underlying)), port(6882), uploaded(0), downloaded(0),
      left(0), compact(0)
{
}

TrackerRequest::TrackerRequest(const std::string &announce, const persist::TorrentModel &model,
                               const common::AppId &appId)
    : announce(announce), infoHash(model.infoHash),
      urlInfoHash(common::urlEncode<20>(infoHash.underlying)), appId(appId),
      urlAppId(common::urlEncode<20>(appId.underlying)), port(6882), uploaded(0), downloaded(0),
      left(0), compact(0)
{
}

TrackerRequest::TrackerRequest(const std::string &announce, const common::InfoHash &infoHash,
                               const std::string urlInfoHash, const common::AppId &appId,
                               const std::string urlAppId, int port, int uploaded, int downloaded,
                               int left, int compact)
    : announce(announce), infoHash(infoHash), urlInfoHash(urlInfoHash), appId(appId),
      urlAppId(urlAppId), port(port), uploaded(uploaded), downloaded(downloaded), left(left),
      compact(compact){};

std::string TrackerRequest::toHttpGetUrl() const
{
    // do not change string formatting
    // this is BT protocol standard
    std::string annBase = announce + "?";
    std::string ihPrm = "info_hash=" + urlInfoHash;
    std::string peerPrm = "peer_id=" + urlAppId;
    std::string portPrm = "port=" + std::to_string(port);
    std::string uplPrm = "uploaded=" + std::to_string(uploaded);
    std::string dlPrm = "downloaded=" + std::to_string(downloaded);
    std::string lftPrm = "left=" + std::to_string(left);
    std::string cmptPrm = "compact=1"; //   +std::to_string(tr.compact);

    std::string url = annBase + common::intercalate(
                                    "&", {ihPrm, peerPrm, portPrm, uplPrm, dlPrm, lftPrm, cmptPrm});
    return url;
}

// tracker response peer model in dictionary format
// keys are peer id, ip and port
neither::Either<std::string, std::vector<Peer>> parsePeersDict(const blist &bl)
{
    auto parsePeer = [](const bdata &bd) -> Either<std::string, Peer>
    {
        auto mpeerDict = common::toBdict(bd);
        if (!mpeerDict.hasValue)
        {
            return neither::left("Could not coerce bdata to bdict for peer"s);
        }
        auto peerDict = mpeerDict.value;

        auto mpeerId = common::toMaybe(peerDict.find("peer id"))
                            .flatMap(toBstring)
                            .map(mem_fn(&bstring::to_string));
        auto mip = common::toMaybe(peerDict.find("ip"))
                       .flatMap(toBstring)
                       .map(mem_fn(&bstring::to_string));
        auto mport =
            common::toMaybe(peerDict.find("port")).flatMap(toBint).map(std::mem_fn(&bint::value));

        if (!mpeerId.hasValue)
        {
            return neither::left("Could not find field peer id in peers dictionary"s);
        }
        if (!mip.hasValue)
        {
            return neither::left("Could not find field ip in peers dictionary"s);
        }
        if (!mport.hasValue)
        {
            return neither::left("Could not find field port in peers dictionary"s);
        }

        return neither::right(Peer{mpeerId.value, PeerId(mip.value, (uint)mport.value)});
    };

    return common::mmapVector<bdata, std::string, Peer>(bl.values(), parsePeer);
}

// tracker response peer model in binary format
// multiples of 6 bytes. First 4 bytes are ip and remaining 2 bytes are port. big endian notation
neither::Either<std::string, std::vector<Peer>> parsePeersBin(std::vector<char> bytes)
{
    if (bytes.size() % 6 != 0)
    {
        return neither::left("Peer binary data is not a multiple of 6"s);
    }

    std::vector<Peer> peers;
    for (int i = 0; i < bytes.size(); i += 6)
    {
        struct sockaddr_in sa;
        char buffer[4];
        std::copy(bytes.begin() + i, bytes.begin() + i + 4, buffer);
        char result[INET_ADDRSTRLEN]; // 16

        // parse first 4 bytes to ip address
        inet_ntop(AF_INET, (void *)(&buffer[0]), result, sizeof result);

        std::string ip(result);
        // convert remaining 2 bytes to port
        ushort port = static_cast<unsigned char>(bytes[i + 4]) * 256 +
                      static_cast<unsigned char>(bytes[i + 5]);

        auto peerId = ip + ":" + std::to_string(port);
        peers.emplace_back(Peer{peerId, PeerId(ip, port)});
    }

    return neither::right<std::vector<Peer>>(peers);
}

std::variant<std::string, TrackerResponse> TrackerResponse::decode(const bdict &bd)
{
    auto optfailure = bd.find("failure reason");
    std::string failureReason;
    if (optfailure)
    {
        auto mfailure = common::toMaybe(optfailure).flatMap(common::toBstring);
        return mfailure.value.to_string();
    }

    auto warningMessage = common::toMaybe(bd.find("warning message"))
                               .flatMap(toBstring)
                               .map(mem_fn(&bstring::to_string));

    auto minterval =
        common::toMaybe(bd.find("interval")).flatMap(toBint).map(std::mem_fn(&bint::value));

    auto minInterval =
        common::toMaybe(bd.find("min interval")).flatMap(toBint).map(std::mem_fn(&bint::value));

    auto trackerId = common::toMaybe(bd.find("tracker id"))
                          .flatMap(toBstring)
                          .map(std::mem_fn(&bstring::to_string));

    auto mcomplete =
        common::toMaybe(bd.find("complete")).flatMap(toBint).map(std::mem_fn(&bint::value));

    auto mincomplete =
        common::toMaybe(bd.find("incomplete")).flatMap(toBint).map(std::mem_fn(&bint::value));

    auto mpeersUnk = common::toMaybe(bd.find("peers"));
    auto mpeersDict = mpeersUnk.flatMap(toBlist);
    auto mpeersBin = mpeersUnk.flatMap(toBstring).map(mem_fn(&bstring::value));

    std::vector<Peer> peers;
    if (mpeersDict.hasValue)
    {
        auto epeers = parsePeersDict(mpeersDict.value);
        if (epeers.isLeft)
        {
            return epeers.leftValue;
        }
        else
        {
            peers = epeers.rightValue;
        }
    }
    else
    {
        if (mpeersBin.hasValue)
        {
            auto epeers = parsePeersBin(mpeersBin.value);
            if (epeers.isLeft)
            {
                return epeers.leftValue;
            }
            else
            {
                peers = epeers.rightValue;
            }
        }
        else
        {
            return "Could not find field peers in tracker response"s;
        }
    }

    if (!minterval.hasValue)
    {
        return "Could not find field interval in tracker response"s;
    }
    if (!mcomplete.hasValue)
    {
        return "Could not find field complete in tracker respone"s;
    }
    if (!mincomplete.hasValue)
    {
        return "Could not find field incomplete in tracker response"s;
    }

    return TrackerResponse{warningMessage,
                           (int)minterval.value,
                           (int)minInterval,
                           trackerId,
                           (int)mcomplete.value,
                           (int)mincomplete.value,
                           peers};
}

Announce TrackerResponse::toAnnounce(const common::InfoHash &infoHash, time_t now) const
{
    std::vector<PeerId> peerIds;
    std::transform(peers.begin(), peers.end(), std::back_insert_iterator(peerIds),
                   [](auto &p)
                   {
                       return p.id;
                   });
    return Announce{infoHash, now, interval, minInterval, peerIds};
}
} // namespace fractals::network::http