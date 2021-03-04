#include "app/Client.h"
#include "common/utils.h"

#include <unistd.h>

std::vector<char> generate_peerId(){
    std::string processId = std::to_string(::getpid());
    std::string peer_id_init = "-FC1000-"+processId+"-";
    std::string peer_id = peer_id_init + random_alphaNumerical(20 - peer_id_init.length());

    return std::vector<char>(peer_id.begin(),peer_id.end());
}