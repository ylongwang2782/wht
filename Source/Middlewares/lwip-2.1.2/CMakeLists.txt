# lwip-2.1.2/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

project(lwip C)

# 设置 lwIP 源目录
set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LWIP_SRC
    ./src/api/api_lib.c
    ./src/api/api_msg.c
    ./src/api/err.c
    ./src/api/if_api.c
    ./src/api/netbuf.c
    ./src/api/netdb.c
    ./src/api/netifapi.c
    ./src/api/sockets.c
    ./src/api/tcpip.c
    ./src/core/altcp.c
    ./src/core/altcp_alloc.c
    ./src/core/altcp_tcp.c
    ./src/core/def.c
    ./src/core/dns.c
    ./src/core/inet_chksum.c
    ./src/core/init.c
    ./src/core/ip.c
    ./src/core/mem.c
    ./src/core/memp.c
    ./src/core/netif.c
    ./src/core/pbuf.c
    ./src/core/raw.c
    ./src/core/stats.c
    ./src/core/sys.c
    ./src/core/tcp.c
    ./src/core/tcp_in.c
    ./src/core/tcp_out.c
    ./src/core/timeouts.c
    ./src/core/udp.c
    ./src/core/ipv4/autoip.c
    ./src/core/ipv4/dhcp.c
    ./src/core/ipv4/etharp.c
    ./src/core/ipv4/icmp.c
    ./src/core/ipv4/igmp.c
    ./src/core/ipv4/ip4.c
    ./src/core/ipv4/ip4_addr.c
    ./src/core/ipv4/ip4_frag.c
    ./src/netif/ethernet.c
    ./port/GD32F4xx/FreeRTOS/ethernetif.c
    ./port/GD32F4xx/FreeRTOS/sys_arch.c)

# 编译成静态库
add_library(lwip STATIC ${LWIP_SRC})

# 包含路径
target_include_directories(
  lwip
  PUBLIC ${LWIP_DIR}/src/include ${LWIP_DIR}/src/include/netif
         ${LWIP_DIR}/src/include/lwip ${LWIP_DIR}/src/include/ipv4
         ${LWIP_DIR}/port/GD32F4xx/ ${LWIP_DIR}/port/GD32F4xx/FreeRTOS)

# 宏定义
# target_compile_definitions(lwip PUBLIC NO_SYS=0 LWIP_TIMEVAL_PRIVATE=0)

# 编译选项（可选） target_compile_options(lwip PRIVATE -Wall)
