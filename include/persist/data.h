#pragma once

#include <filesystem>

#include "persist/storage.h"
#include "torrent/Torrent.h"
#include "network/http/Announce.h"

void save_torrent(const Storage &st, std::string mi,const Torrent &t);
std::vector<std::shared_ptr<Torrent>> load_torrents(const Storage &st);
bool has_torrent(const Storage &st, const Torrent &t);
void delete_torrent (const Storage &st,const Torrent &t);

void save_piece(const Storage &st,const Torrent &t,int piece);
std::set<int> load_pieces(const Storage &st,const Torrent &t);

void save_announce(const Storage &st,const Torrent &t,const Announce &ann);
void delete_announces(const Storage &st,const Torrent &t);
std::optional<Announce> load_announce(const Storage &st,const Torrent &t);