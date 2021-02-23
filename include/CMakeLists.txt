set(INCLUDES
    torrent/BencodeConvert.h
    torrent/MetaInfo.h
    network/TrackerRequest.h
    common/maybe.h
    common/utils.h
)

install(FILES ${INCLUDES} DESTINATION ${INSTALL_INCLUDE_DIR}/Fractals)