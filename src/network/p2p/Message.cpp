#include "network/p2p/Message.h"


HandShake::HandShake(unsigned char pstrlen,std::string pstr,std::string url_info_hash,std::string peer_id) {
    m_pstrlen = pstrlen;
    m_pstr = pstr;
    for(int i = 0; i < 8; i++) { m_reserved[i] = 0; }
    m_url_info_hash = url_info_hash;
    m_peer_id = peer_id;
}

std::vector<char> HandShake::to_bytes_repr() const {
    std::vector<char> v;
    v.push_back(m_pstrlen);
    std::copy(m_pstr.begin(),m_pstr.end(),std::back_inserter(v));
    v.insert(v.end(),m_reserved,m_reserved+8);
    v.insert(v.end(),m_url_info_hash.begin(),m_url_info_hash.end());
    v.insert(v.end(),m_peer_id.begin(),m_peer_id.end());
    return v;
}
