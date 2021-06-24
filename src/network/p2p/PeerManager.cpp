#include "network/p2p/PeerManager.h"
#include "network/http/Peer.h"
#include <mutex>

PeerManager::PeerManager(int max_conn) : m_max_conn(max_conn) {};

bool PeerManager::add_new_connection(PeerId p, std::shared_ptr<Connection> conn) {
    std::unique_lock<std::mutex> _lock(m_mutex);
    if(m_enabled && m_conns.size() < m_max_conn) {
        m_conns.insert({p,conn});
        return true;
    } else {
        return false;
    }
}

void PeerManager::remove_connection(PeerId p) {
    std::unique_lock<std::mutex> _lock(m_mutex);
    if(m_enabled) {
        m_conns.erase(p);
    }
}

void PeerManager::disable() {
    std::unique_lock<std::mutex> _lock(m_mutex);
    m_enabled = false;
    for(auto &e : m_conns) {
        e.second->cancel();
    }
    // m_conns.clear();
    // m_work.clear();
}

void PeerManager::enable() {
    m_enabled = true;
}

bool PeerManager::is_enabled() {
    return m_enabled;
}

void PeerManager::new_work(PeerId p,std::shared_ptr<PeerWork> work) {
    std::unique_lock<std::mutex> _lock(m_mutex);
    m_work.insert({p,work});
}

std::optional<std::shared_ptr<PeerWork>> PeerManager::lookup_work(PeerId p) {
    auto it = m_work.find(p);
    if(it != m_work.end()) {
        return it->second;
    } else {
        return {};
    }
}

void PeerManager::finished_work(PeerId p) {
    std::unique_lock<std::mutex> _lock(m_mutex);
    m_work.erase(p);
}

bool PeerManager::is_connected_to(PeerId p) {
    std::unique_lock<std::mutex> _lock(m_mutex);
    return m_conns.find(p) != m_conns.end();
}

std::optional<std::shared_ptr<Connection>> PeerManager::lookup(PeerId p) {
    auto it = m_conns.find(p);
    if(it != m_conns.end()) {
        return it->second;
    } else {
        return {};
    }
}

std::shared_ptr<Connection> const& PeerManager::operator[](PeerId p) {
    std::unique_lock<std::mutex> _lock(m_mutex);
    return m_conns[p];
}