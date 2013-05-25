/**
 * queue.c - packet queue handling
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "rtp.h"
#include "socket.h"
#include "timing.h"
#include "queue.h"

#define INC(q) ((q->s) = ((q->s) + 1) % q->n);

unsigned buffers(queue_t *q) { return q->n; }

void setbuffertime(queue_t *q, unsigned ms) { q->buffertime = ms; }
unsigned buffertime(queue_t *q) { return q->buffertime; }

void setdeadline(queue_t *q, unsigned ms) { q->deadline = ms; }
unsigned deadline(queue_t *q) { return q->deadline; }

unsigned queuelen(queue_t *q) { return q->len; }
double sendrate(queue_t *q) { return q->bytes_sent / curtime(); }

int createq(queue_t *q, unsigned packets)
{
  if (0 == (q->packets = calloc(packets, sizeof *q->packets))) {
    seterror(err_NM);
    return 0;
  }

  if (!createlock(&q->lock)) {
    return 0;
  }
  q->n = packets;
  q->s = 0;
  q->packetlist = 0;
  q->len = 0;
  q->bytes_sent = 0;
  q->buffertime = 0;
  q->deadline = 0;

  return 1;
}

void deleteq(queue_t *q)
{
  free(q->packets);
  deletelock(&q->lock);
}

static void addpacket(queue_t *q, packet_t *p)
{
  packet_t *pl = q->packetlist;

  if (!q->packetlist) {
    q->packetlist = p;
    q->packetlist->next = 0;
    q->packetlist->prev = 0;
    goto X;
  }

  while (pl->next) {
    pl = pl->next;
  }

  p->prev = pl;
  pl->next = p;

X: q->len++;
}

static packet_t *delpacket(queue_t *q, packet_t *p)
{
  if (p && q->packetlist) {
    if (p->next) p->next->prev = p->prev;
    if (p->prev) p->prev->next = p->next; else q->packetlist = p->next;
    p->blocked = 0;
    q->len--;
    return p->next;
  }
  return 0;
}

static void swappackets(packet_t *p1, packet_t *p2)
{
  packet_t *tmp;

  if (p1 && p2) {
    if (p1->next) p1->next->prev = p2;
    if (p1->prev) p1->prev->next = p2;
    if (p2->next) p2->next->prev = p1;
    if (p2->prev) p2->prev->next = p1;

    tmp = p1->next;
    p1->next = p2->next;
    p2->next = tmp;

    tmp = p1->prev;
    p1->prev = p2->prev;
    p2->prev = tmp;
  }
}

int enqueue(queue_t *q, RTP_header *h, unsigned ft, unsigned char *p, unsigned l, unsigned scale)
{
  unsigned j = 0;
  double t = h->timestamp / (double)scale;

  if (q->mode & MODE_STREAM || .99 * t <= curtime()) {
    if (!lock(&q->lock)) return 0;

    if (l > sizeof q->packets[q->s].p) goto PS;
    while (j++ < q->n && q->packets[q->s].blocked) INC(q);
    if (q->packets[q->s].blocked) goto PS;

    q->packets[q->s].id      = h->id;
    q->packets[q->s].tstamp  = t;
    q->packets[q->s].frame   = ft;
    q->packets[q->s].size    = l;
    q->packets[q->s].next    = 0;
    q->packets[q->s].prev    = 0;
    q->packets[q->s].blocked = 1;
    memcpy(q->packets[q->s].p, p, l);

    addpacket(q, &q->packets[q->s]);
    INC(q);

    unlock(&q->lock);
    return 1;
  } else {
    return 0;
  }

PS: seterror(err_PS);
    unlock(&q->lock);
    return 0;
}

void dequeue(queue_t *q)
{
  int r;
  packet_t *pl;

  if (!lock(&q->lock)) return;

  pl = q->packetlist;
  while (pl) {
    if (q->mode & MODE_STREAM || pl->tstamp <= curtime()) {
      if (0 < (r = sendbuf(pl->p, pl->size, pl->frame == A ? AUDIO : VIDEO))) {
        q->bytes_sent += r;
        pl = delpacket(q, pl);
      }
    }
  }

  unlock(&q->lock);
}

void sortqueue(queue_t *q)
{
  packet_t *pl, *p, *p1, *p2;
  unsigned n;

  if (!lock(&q->lock)) return;

  pl = q->packetlist;

  while (pl) {
    p1 = pl;
    p2 = 0;
    n = p1->id;
    p = pl->next;
    while (p) {
      if (p->id < n) {
        n = p->id;
        p2 = p;
      }
      p = p->next;
    }
    if (p2) swappackets(p1, p2);
    pl = pl->next;
  }

  unlock(&q->lock);
}

void printq(queue_t *q)
{
  packet_t *pl;

  if (!lock(&q->lock)) return;

  pl = q->packetlist;
  while (pl) {
    pl = pl->next;
  }

  unlock(&q->lock);
}
