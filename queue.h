#ifndef TRACE_QUEUE
#define TRACE_QUEUE

#include "rtp.h"
#include "lock.h"

typedef struct queue {
  packet_t *packets;                              /*! packet buffer */
  unsigned n;                                     /*! number of packets */
  unsigned s;                                     /*! packet buffer counter */
  packet_t *packetlist;                           /*! packet buffer linked list */
  unsigned len;                                   /*! list length */
  unsigned bytes_sent;                            /*! number of sent bytes */
  unsigned buffertime;                            /*! buffer time [ms] */
  unsigned deadline;                              /*! packet drop time [ms] */
  MODE mode;                                      /*! real-time (camera) or streaming (file transfer) */
  lock_t lock;                                    /*! queue mutex */
} queue_t;

int createq(queue_t *q, unsigned packets);
void deleteq(queue_t *q);

unsigned buffers(queue_t *);

void setbuffertime(queue_t *, unsigned ms);
unsigned buffertime(queue_t *);

void setdeadline(queue_t *, unsigned ms);
unsigned deadline(queue_t *);

int enqueue(queue_t *, RTP_header *, unsigned, unsigned char *, unsigned, unsigned);
void dequeue(queue_t *);

unsigned queuelen(queue_t *);
double sendrate(queue_t *);

void sortqueue(queue_t *);
void printq(queue_t *);

#endif
