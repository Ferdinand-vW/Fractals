# Fractals

# Introduction

Fractals is a standard BitTorrent client written in c++. Unlike most BitTorrent clients, Fractals has a terminal user interface.
Users are expected to open the app in their terminals and write commands to interact with the client.

Currently only an experimental version is out. Bugs should therefore be expected.

# Usage

run the app in your favourite terminal.

> $ ./fractals

You now have a few options available to you. I will list them below.

Add a new torrent and start downloading

> add mypathtotorrentfile.torrent

Stop a torrent from downloading

> stop #0 -- #0 refers to the identifier under column #

Allow a stopped torrent to continue to download

> resume #0

Remove torrent from the client (only deletes internal state and not actual torrent data)

> remove #0

Exit the app by pressing escape or type the following

> exit 

# Build instructions

## Required dependencies
- BOOST 1.74
- OpenSSL
- cURL
- Sqlite3

Clone the repository and its submodules
> $ git clone --recursive https://github.com/Ferdinand-vW/Fractals.git

Move into the root folder
> $ cd Fractals

create a build directory
> $ mkdir build && cd build

generate the Make file
> $ cmake .. -DCMAKE_CXX_COMPILER=clang++

(due to a bug in g++ compiler the code currently only compiles with clang++)

run Make to generate the executable
> $ make


# TODO

This is a non-exhaustive list of tasks in random order that I wish to tackle in the future
- Add leecher functionality (i.e. upload torrent data to peers)
- Add testing framework and tests
- Add CI/CD infrastructure
- Implement BitTorrent protocol extensions
- Implement Optimisations (i.e. database queries, thread work load, generic pipeline improvements)
- Rework part of the code to workaround the g++ compiler bug