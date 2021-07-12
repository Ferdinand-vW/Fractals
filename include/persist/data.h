#pragma once

#include <filesystem>
#include <vector>
#include <optional>
#include <set>

class Storage;
class Torrent;
class Announce;

void save_torrent(Storage &st, std::string mi,const Torrent &t);
std::vector<std::shared_ptr<Torrent>> load_torrents(Storage &st);
bool has_torrent(Storage &st, const Torrent &t);
void delete_torrent (Storage &st,const Torrent &t);

void save_piece(Storage &st,const Torrent &t,int piece);
std::set<int> load_pieces(Storage &st,const Torrent &t);
void delete_pieces(Storage &st,const Torrent &t);

void save_announce(Storage &st,const Torrent &t,const Announce &ann);
void delete_announces(Storage &st,const Torrent &t);
std::optional<Announce> load_announce(Storage &st,const Torrent &t);