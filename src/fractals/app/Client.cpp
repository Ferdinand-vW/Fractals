#include <string>

#include <unistd.h>

#include <fractals/app/Client.h>
#include <fractals/common/utils.h>

namespace fractals::app {

    std::array<char, 20> generateAppId(){
        //use current process id to allow identification of current instance of app
        std::string processId = std::to_string(::getpid());
        std::string peerIdInit = "-FC1000-"+processId+"-";

        //fill remained with random characters
        std::string peerIdStr = peerIdInit + common::randomAlphaNumerical(20 - peerIdInit.length());

        std::array<char, 20> peerId; 
        std::copy(peerIdStr.begin(), peerIdStr.end(), peerId.data());
        return peerId;
    }

}