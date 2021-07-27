#include <string>

#include <unistd.h>

#include "fractals/app/Client.h"
#include "fractals/common/utils.h"

namespace fractals::app {

    std::vector<char> generate_peerId(){
        //use current process id to allow identification of current instance of app
        std::string processId = std::to_string(::getpid());
        std::string peer_id_init = "-FC1000-"+processId+"-";

        //fill remained with random characters
        std::string peer_id = peer_id_init + common::random_alphaNumerical(20 - peer_id_init.length());

        return std::vector<char>(peer_id.begin(),peer_id.end());
    }

}