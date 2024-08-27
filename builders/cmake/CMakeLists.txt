cmake_minimum_required (VERSION 3.5)

project (libpeer)

LIST(APPEND CMAKE_MODULE_PATH "$ENV{AWOXCVS}/AwoxSoftware/builders/cmake")
include (aw2)

set_subdirectory (peer-module)
get_filename_component(ccCompDir ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)

set (sourceFiles
		${ccCompDir}/src/address.c
		${ccCompDir}/src/agent.c
		${ccCompDir}/src/base64.c
		${ccCompDir}/src/buffer.c
		${ccCompDir}/src/dtls_srtp.c
		${ccCompDir}/src/ice.c
		${ccCompDir}/src/mdns.c
		${ccCompDir}/src/peer.c
		${ccCompDir}/src/peer_connection.c
#		${ccCompDir}/src/peer_signaling.c
		${ccCompDir}/src/ports.c
		${ccCompDir}/src/rtcp.c
		${ccCompDir}/src/rtp.c
		${ccCompDir}/src/sctp.c
		${ccCompDir}/src/sdp.c
		${ccCompDir}/src/socket.c
#		${ccCompDir}/src/ssl_transport.c
		${ccCompDir}/src/stun.c
#		${ccCompDir}/src/tcp.c
#		${ccCompDir}/src/udp.c
		${ccCompDir}/src/utils.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

add_subdirectory_once (mbedtls-module ${awRootDir}/AwoxSoftware/Libs/External/mbedtls${mbedtls_version}/builders/cmake)
add_subdirectory_once (usrsctp-module ${awRootDir}/AwoxSoftware/Libs/External/usrsctp/builders/cmake)
add_subdirectory_once (libsrtp2-module ${ccRootDir}/components/libsrtp2/builders/cmake)
add_subdirectory_once (cjson-module ${ccRootDir}/components/cJSON/builders/cmake)
list(APPEND link_Libraries cjson mbedtls mbedcrypto mbedx509 libsrtp2 usrsctp-static)

add_definitions(-DCONFIG_MQTT=0 -DCONFIG_HTTP=0 -DLOG_LEVEL=LEVEL_DEBUG -DLOG_REDIRECT)
add_library(peer ${sourceFiles})
target_link_libraries(peer ${link_Libraries})

#link_libraries(${link_Libraries})
include_directories(peer ${ccCompDir})

target_include_directories (peer PUBLIC ${ccCompDir}/src)

install(TARGETS peer ARCHIVE DESTINATION ${AW_ARCHIVE_DIR})
install(TARGETS peer LIBRARY DESTINATION ${AW_LIBRARY_DIR})

