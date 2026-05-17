#ifndef TCP_H_
#define TCP_H_

#include <network/ip.h>

#include <netutils.h>
#include <string.h>
#include <stdint.h>


#define TCP_WINDOW_SIZE     8192
#define TCP_MSL             120000      // Maximum Segment Lifetime (ms)

#define SEQ_LT(x,y) ((int)((x)-(y)) < 0)
#define SEQ_LE(x,y) ((int)((x)-(y)) <= 0)
#define SEQ_GT(x,y) ((int)((x)-(y)) > 0)
#define SEQ_GE(x,y) ((int)((x)-(y)) >= 0)

// flags
#define TCP_FIN                         (1 << 0)
#define TCP_SYN                         (1 << 1)
#define TCP_RST                         (1 << 2)
#define TCP_PSH                         (1 << 3)
#define TCP_ACK                         (1 << 4)
#define TCP_URG                         (1 << 5)

#define OPT_END                         0
#define OPT_NOP                         1
#define OPT_MSS                         2

#define TCP_CLOSED                      0
#define TCP_LISTEN                      1
#define TCP_SYN_SENT                    2
#define TCP_SYN_RECEIVED                3
#define TCP_ESTABLISHED                 4
#define TCP_FIN_WAIT_1                  5
#define TCP_FIN_WAIT_2                  6
#define TCP_CLOSE_WAIT                  7
#define TCP_CLOSING                     8
#define TCP_LAST_ACK                    9
#define TCP_TIME_WAIT                   10

#define TCP_CONN_RESET                  1
#define TCP_CONN_REFUSED                2
#define TCP_CONN_CLOSING                3


typedef struct TcpHeader {
    uint16_t srcPort;
    uint16_t dstPort;
    uint32_t seq;
    uint32_t ack;
    uint8_t off;
    uint8_t flags;
    uint16_t windowSize;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) TcpHeader;

typedef struct TcpOptions {
    uint16_t mss;
} TcpOptions;

typedef struct TcpConn {
    // Link link;
    unsigned int state;

    uint8_t* intf;

    uint8_t* localAddr;
    uint8_t* nextAddr;
    uint8_t* remoteAddr;

    uint16_t localPort;
    uint16_t remotePort;

    // send state
    uint32_t sndUna;                         // send unacknowledged
    uint32_t sndNxt;                         // send next
    uint32_t sndWnd;                         // send window
    uint32_t sndUP;                          // send urgent pointer
    uint32_t sndWl1;                         // segment sequence number used for last window update
    uint32_t sndWl2;                         // segment acknowledgment number used for last window update
    uint32_t iss;                            // initial send sequence number

    // receive state
    uint32_t rcvNxt;                        // receive next
    uint32_t rcvWnd;                        // receive window
    uint32_t rcvUP;                         // receive urgent pointer
    uint32_t irs;                            // initial receive sequence number

    // queues
    // Link resequence;

    // timers
    uint32_t mslWait;                       // when does the 2MSL time wait expire?

    // callbacks
    void *ctx;
    void (*onError)(struct TcpConn *conn, unsigned int error);
    void (*onState)(struct TcpConn *conn, unsigned int oldState, unsigned int newState);
    void (*onData)(struct TcpConn *conn, const uint8_t *data, unsigned int len);
} TcpConn;


// extern Link g_tcpActiveConns;

// void TcpInit();
// void TcpRecv(NetIntf *intf, const Ipv4Header *ipHdr, NetBuf *pkt);
// void TcpPoll();
// void TcpSwap(TcpHeader *hdr);

// TcpConn *TcpCreate();
// bool TcpConnect(TcpConn *conn, const Ipv4Addr *addr, uint16_t port);
// void TcpClose(TcpConn *conn);
// void TcpSend(TcpConn *conn, const void *data, unsigned int count);

#endif