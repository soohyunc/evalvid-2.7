#ifndef TRACE_RTP
#define TRACE_RTP

#include "bits.h"

enum rfc { rfc_none, rfc_2429 };
enum ftype { H, I, P, B, S, A, X };
enum ptype { VIDEO, AUDIO };
typedef enum mode {
  INVALID,
  MODE_SEND   = 1,
  MODE_PACKET = 2,
  MODE_FRAME  = 4,
  MODE_AUDIO  = 8,
  MODE_STREAM = 16
} MODE;

typedef struct {
  unsigned long
    V  : 2,     /* version */
    P  : 1,     /* padding */
    X  : 1,     /* extension */
    CC : 4,     /* CSRC count */
    M  : 1,     /* marker */
    PT : 7,     /* payload type */
    id : 16,    /* sequence number */
    timestamp,  /* guess */
    SSRC,       /* synchronization source */
    CSRC[16];   /* contributing sources */
} RTP_header;

typedef struct packet {
  unsigned long
    blocked : 1,          /* sent flag */
    frame   : 3,          /* frame type */
    size    : 28,         /* packet size */
    segm    : 16,         /* segment nr. */
    nsegm   : 16,         /* segments in frame */
    id;                   /* packet id */
  double
    tstamp,               /* RTP time stamp */
    t[2];                 /* time send, received */
  unsigned char p[65535];  /* RTP packet */
  struct packet *next, *prev;
} packet_t;

#define GET_U8( p, n) (((u8 *)(p))[n])
#define GET_U16(p, n) (GET_U8( p, n) << 8 | GET_U8(p, (n) + 1))
#define GET_U24(p, n) (GET_U16(p, n) << 8 | GET_U8(p, (n) + 2))
#define GET_U32(p, n) (GET_U24(p, n) << 8 | GET_U8(p, (n) + 3))

#define RTP_GET_V(p)   (GET_U8(p, 0) >> 6 & 3)
#define RTP_GET_P(p)    GETBIT(GET_U8(p, 0), 5)
#define RTP_GET_X(p)    GETBIT(GET_U8(p, 0), 4)
#define RTP_GET_CC(p)  (GET_U8(p, 0) & 15)
#define RTP_GET_M(p)    GETBIT(GET_U8(p, 1), 7)
#define RTP_GET_PT(p)  (GET_U8(p, 1) & 127)
#define RTP_GET_SN(p)   GET_U16(p, 2)
#define RTP_GET_TS(p)   GET_U32(p, 4)
#define RTP_GET_SSRC(p) GET_U32(p, 8)

#define SET_U8( p, n, v) (((u8 *)(p))[n] = (u8)(v))
#define SET_U16(p, n, v) SET_U8(p, n, (v) >>  8 & 255), SET_U8( p, (n) + 1, (v) & 255)
#define SET_U24(p, n, v) SET_U8(p, n, (v) >> 16 & 255), SET_U16(p, (n) + 1, (v) & 255)
#define SET_U32(p, n, v) SET_U8(p, n, (v) >> 24 & 255), SET_U24(p, (n) + 1, (v) & 255)

#define RTP_SET_M(p)    SETBIT(GET_U8(p, 1), 7)
#define RTP_SET_SN(p)   SET_U16(p, 2)
#define RTP_SET_TS(p)   SET_U32(p, 4)
#define RTP_SET_SSRC(p) SET_U32(p, 8)
 
#endif
