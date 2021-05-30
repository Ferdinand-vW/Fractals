#pragma once

#include "persist/storage.h"
#include "torrent/Torrent.h"
#include "network/http/Announce.h"

void add_torrent(const Storage &st, const Torrent &t);
std::vector<std::unique_ptr<Torrent>> load_torrents(const Storage &st);
void delete_torrent (const Storage &st,const Torrent &t);

void add_piece(const Storage &st,const Torrent &t);
std::vector<int> load_pieces(const Storage &st,const Torrent &t);

void save_announce(const Storage &st,const Torrent &t,const Announce &ann);
void delete_announces(const Storage &st,const Torrent &t);
std::optional<Announce> load_announce(const Storage &st,const Torrent &t);