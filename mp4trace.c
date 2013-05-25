
#include <stdio.h>
#include <stdlib.h>
#include <gpac/isomedia.h>
#include <gpac/internal/isomedia_dev.h>

#include "bits.h"
#include "error.h"
#include "misc.h"
#include "rtp.h"
#include "socket.h"
#include "thread.h"
#include "timing.h"
#include "queue.h"

#define MP4_SSRC 6974316
#define VQ_LEN 100
#define AQ_LEN 10

static char *form_dur(u64 dur, u32 scale)
{
  static char fd[100];
  u32 h, m, s, ms, tmp = (u32) (1000 * dur / scale); /* max. 49 days */

  h = tmp / 3600000;
  m = (tmp -= h * 3600000) / 60000;
  s = (tmp -= m * 60000) / 1000;
  ms = tmp - s * 1000;
  sprintf(fd, "%02u:%02u:%02u.%03u", h, m, s, ms);

  return fd;
}

static void fillRTPheader(RTP_header *h, unsigned char *p)
{
  resetbits(0);
  h->V  = nextbits(p, 2);
  h->P  = nextbits(p, 1);
  h->X  = nextbits(p, 1);
  h->CC = nextbits(p, 4);
  h->M  = nextbits(p, 1);
  h->PT = nextbits(p, 7);
  h->id = nextbits(p, 16);
  h->timestamp = nextbits(p, 32);
  h->SSRC = MP4_SSRC;
}

u32 H264frametype(unsigned char *p)
{
  u32 nal = p[12] & 31, turns = 0;

N: switch (turns++, nal) {
     case  1:
     case  5:
     case  6: return 6 - (nal + 12) / 3;
     case 24:
     case 28:
     case 29: nal = p[13 + (30 - nal) / 3] & 31; if (turns < 2) goto N;
     default: return 4;
   }
}

u32 MPEG4frametype(unsigned char *p)
{
  u32 ft;

  resetbits(12);
  ft = nextbits(p, 24) == 1 ? nextbits(p, 8) == 0xb6 ? nextbits(p, 2) + 1 : 0 : 4;
  if (ft != 4) return ft;
  resetbits(16);
  return nextbits(p, 24) == 1 ? nextbits(p, 8) == 0xb6 ? nextbits(p, 2) + 1 : 0 : 4;
}

u32 H263frametype(unsigned char *p)
{
  int i, psc, vrc, plen, pebit, ft = 3;

  resetbits(12);
  if (nextbits(p, 5)) goto X;
  psc = nextbits(p, 1);
  vrc = nextbits(p, 1);
  plen =  nextbits(p, 6);
  pebit = nextbits(p, 3);
  if (!plen && pebit) goto X;
  if (vrc) {
    nextbits(p, 3);  /* thread id */
    nextbits(p, 4);  /* packet number */
    nextbits(p, 1);  /* sync frame */
  }
  for (i = 0; i < plen; i++) nextbits(p, 8); /* extra picture header */

  /* H.263+ Header */
  if (p) {
    if (nextbits(p, 6) != 32) goto X;  /* remaining picture start code */
    nextbits(p, 8);                    /* timestamp */
    nextbits(p, 5);
    if (nextbits(p, 3) == 7) goto X;   /* picture size */
    ft = nextbits(p, 1);               /* frame type */
    nextbits(p, 3);
    if (nextbits(p, 1)) ft = 2;        /* frame type (opt.) */
  }

X: return ft + 1;
}

typedef struct ctx {
  RTP_header *rtp;
  MODE mode;
  GF_ISOFile *f;
  volatile thread_t *t;
  queue_t *q;
  u32 hint_track;
  u32 sub_type;
  u32 time_scale;
  u32 segm;
  u32 framesize;
  u32 frametype;
  u32 count;
  volatile u32 active;
} context_t;

static int enqueue_v(void *context)
{
  char *p;
  Bool dis, rep;
  u32 l, video_ts = 0, sn_v = 0;
  double t;
  GF_Err e;
  struct ctx *ctx = context;

  if (!ctx->active) return 1;

  if (GF_OK != (e = gf_isom_next_hint_packet(ctx->f, ctx->hint_track, &p, &l, &dis, &rep, &video_ts, &sn_v))) {
    if (e != GF_EOS) {
      fprintf(stderr, "%p, %u, %p, %u, %d, %d, %u, %u\n", &ctx->f, ctx->hint_track, &p, l, dis, rep, video_ts, sn_v);
      fprintf(stderr, "gf_isom_next_hint_packet: %s\n", gf_error_to_string(e));
    }
    return 0;
  }

  if (ctx->rtp->M) {
    switch (ctx->sub_type) {
      case 'avc1': ctx->frametype = H264frametype(p); break;
      case 'mp4v':
      case 'MPEG': ctx->frametype = MPEG4frametype(p); break;
      case 's263': ctx->frametype = H263frametype(p); break;
      default    : ctx->frametype = 4;
    }
  }

  ctx->segm++;
  ctx->framesize += l;
  fillRTPheader(ctx->rtp, p);
  t = ctx->rtp->timestamp / (double)ctx->time_scale;

  while (!enqueue(ctx->q, ctx->rtp, ctx->frametype, p, l, ctx->time_scale)) {
    SLEEP(1);
  }

  if (ctx->mode & MODE_PACKET) {
    printf("%u\t%c\t%.3f\t%u\t%.0f\t%u\n",
      ctx->rtp->id, ctx->frametype["HIPBXXX"], t, l, sendrate(ctx->q), queuelen(ctx->q));
  }
  if (ctx->mode & MODE_FRAME && ctx->rtp->M) {
    printf("%u\t%c\t%u\t%u\t%.3f\n", ++ctx->count, ctx->frametype["HIPBXXX"], ctx->framesize, ctx->segm, curtime());
    ctx->segm = 0;
    ctx->framesize = 0;
  }
  return 1;
}

static int enqueue_a(void *context)
{
  char *p;
  Bool dis, rep;
  u32 l, audio_ts = 0, sn_a = 0;
  double t;
  GF_Err e;
  struct ctx *ctx = context;

  if (!ctx->active) return 1;

  if (GF_OK != (e = gf_isom_next_hint_packet(ctx->f, ctx->hint_track, &p, &l, &dis, &rep, &audio_ts, &sn_a))) {
    if (e != GF_EOS) {
      fprintf(stderr, "%p, %u, %p, %u, %d, %d, %u, %u\n", &ctx->f, ctx->hint_track, &p, l, dis, rep, audio_ts, sn_a);
      fprintf(stderr, "gf_isom_next_hint_packet: %s\n", gf_error_to_string(e));
    }
    return 0;
  }
  switch (ctx->sub_type) {
    case 'mp4a': ctx->frametype = A; break;
    default    : ctx->frametype = X;
  }
  fillRTPheader(ctx->rtp, p);
  t = ctx->rtp->timestamp / (double)ctx->time_scale;

  while (!enqueue(ctx->q, ctx->rtp, ctx->frametype, p, l, ctx->time_scale)) {
    SLEEP(1);
  }
  if (ctx->mode & MODE_PACKET) {
    printf("%u\t%c\t%.3f\t%u\t%.0f\t%u\n",
      ctx->rtp->id, ctx->frametype["XXXXXAX"], t, l, sendrate(ctx->q), queuelen(ctx->q));
  }
  if (ctx->mode & MODE_FRAME) {
    printf("%u\t%c\t%u\t%u\t%.3f\n", ctx->rtp->id, ctx->frametype["XXXXXAX"], l, 1, curtime());
  }
  return 1;
}

static ret_t v_queuer(void *p)
{
  context_t *ctx = p;
  while (enqueue_v(p))
    ;
  ctx->t->running = 0;
  return 0;
}

static ret_t a_queuer(void *p)
{
  context_t *ctx = p;
  while (enqueue_a(p))
    ;
  ctx->t->running = 0;
  return 0;
}

static ret_t sender(void *p)
{
  context_t *ctx = p;

  while (!ctx->active) {
    SLEEP(1);
  }

  while (ctx->t->running) {
    dequeue(ctx->q);
    SLEEP(1);
  }

  return 0;
}

int main(int cn, char **cl)
{
  int j, n = 0, loop = 1, delay = 0;
  char *p, *s, *host = 0;
  Bool dis, rep;
  u16 port = 0;
  u32 tracks, type = 0, samples, sub, scale, width = 0, height = 0, sn_v = 0, sn_a = 0, i, l, ev, ea;
  u32 video_hint_track = -1, video_track = -1, video_hint_samples = 0, segm = 0, framesize = 0;
  u32 audio_hint_track = -1, audio_track = -1, audio_hint_samples = 0, ref_track = -1;
  u32 video_type, audio_type, video_sub_type = 0, audio_sub_type = 0, video_ft = 0, audio_ft = 0;
  u32 video_time_scale= 0, audio_time_scale = 0, last_video_ts = 0, last_audio_ts = 0;
  u32 audio_ts = 0, video_ts = 0, last_video_sn = 0, last_audio_sn = 0;
  u64 dur;
  MODE mode = INVALID;
  queue_t qa = { 0 }, qv = { 0 };
  thread_t tap = { 0 }, tvp = { 0 }, tac = { 0 }, tvc = { 0 };
  context_t ctx_vp = { 0 }, ctx_ap = { 0 }, ctx_vc = { 0 }, ctx_ac = { 0 };
  RTP_header rtp_video = { 0 }, rtp_audio = { 0 };
  enum prot protocol = UDP;

  GF_ISOFile *fv = 0, *fa = 0;
  GF_Err e;

  if (cn < 2) {
U:  puts("Usage: mp4trace [options] file");
    puts("options:");
    puts("  -[p/f]\tpacket or frame mode (alternative)");
    puts("  -s host port\tsend the RTP packets to specified host and UDP port");
    puts("  -l n\t\tloop the video n times");
    puts("  -t protocol\ttransport protocol (UDP|TCP)");
    puts("  -m mode\t(cam|stream) // camera-like behavior or streaming");
    puts("  -d n\t\tdelay looping n seconds");
    return EXIT_FAILURE;
  }

  while (++n < cn - 1) {
    s = cl[n];
    if (*s++ == '-')
      switch(*s) {
        case 'p': if (mode & MODE_FRAME) goto U; mode |= MODE_PACKET; break;
        case 'f': if (mode & MODE_PACKET) goto U; mode |= MODE_FRAME; break;
        case 's': if (n + 3 >= cn) goto U;
          host = cl[++n];
          port = (u16) atoi(cl[++n]);
          mode |= MODE_SEND;
          break;
        case 'l': if (n + 2 >= cn) goto U;
          loop = atoi(cl[++n]);
          break;
        case 't': if (n + 2 >= cn) goto U;
          protocol = casecmp(cl[n + 1], "TCP") == 0 ? TCP : UDP;
          break;
        case 'm': if (n + 2 >= cn) goto U;
          if (casecmp(cl[n + 1], "stream") == 0) mode |= MODE_STREAM;
          break;
        case 'd': if (n + 2 >= cn) goto U;
          delay = atoi(cl[++n]);
          break;
        default : goto U;
      }
  }

  if (0 == (fv = gf_isom_open(cl[n], GF_ISOM_OPEN_READ, 0))) {
    fprintf(stderr, "Cannot open file %s: %s\n", cl[n], gf_error_to_string(gf_isom_last_error(0)));
    return EXIT_FAILURE;
  }

  tracks = gf_isom_get_track_count(fv);
  for (i = 0; i < tracks; i++) {
    char *s, *t;
    type = gf_isom_get_media_type(fv, i + 1);
    sub = gf_isom_get_mpeg4_subtype(fv, i + 1, 1);
    if (!sub) sub = gf_isom_get_media_subtype(fv, i + 1, 1);
    samples = gf_isom_get_sample_count(fv, i + 1);
    dur = gf_isom_get_media_duration(fv, i + 1);
    scale = gf_isom_get_media_timescale(fv, i + 1);
    if (type == 'vide') gf_isom_get_visual_info(fv, i + 1, 1, &width, &height);

    switch (type) {
      case 'vide': t = "Video"; break;
      case 'soun': t = "Audio"; break;
      case 'odsm': t = "OD";    break;
      case 'sdsm': t = "BIFS";  break;
      case 'hint': t = "Hint";
        ref_track = 0;
        if (video_hint_track == -1 || audio_hint_track == -1) {
           if (GF_OK == gf_isom_get_reference(fv, i + 1, GF_ISOM_REF_HINT, 1, &ref_track)) {
              if (ref_track) {
                switch (gf_isom_get_media_type(fv, ref_track)) {
                  case 'vide':
                    if (video_hint_track == -1) {
                      video_hint_track = i + 1;
                      video_track = ref_track;
                      video_hint_samples = samples;
                      video_type = gf_isom_get_media_type(fv, video_track);
                      video_sub_type = gf_isom_get_mpeg4_subtype(fv, video_track, 1);
                      if (!video_sub_type) video_sub_type = gf_isom_get_media_subtype(fv, video_track, 1);
                      video_time_scale = scale;
                    }
                    break;
                  case 'soun':
                    if (audio_hint_track == -1) {
                      audio_hint_track = i + 1;
                      audio_track = ref_track;
                      audio_hint_samples = samples;
                      audio_type = gf_isom_get_media_type(fv, audio_track);
                      audio_sub_type = gf_isom_get_mpeg4_subtype(fv, audio_track, 1);
                      if (!audio_sub_type) audio_sub_type = gf_isom_get_media_subtype(fv, audio_track, 1);
                      audio_time_scale = scale;
                    }
                    break;
                }
              }
            }
          }
        break;
      default    : t = "Unsupported"; break;
    }
    switch (sub) {
      case 'avc1': s = "H.264"; break;
      case 'MPEG':
      case 'mp4v': s = "MPEG-4"; break;
      case 's263': s = "H.263"; break;
      case 'mp4a': s = "AAC"; break;
      case 'rtp ': s = "RTP"; break;
      default    : s = "Unsupported"; break;
    }
    switch (type) {
      case 'hint':
        if (ref_track)
          fprintf(stderr, "Track %u: %s (%s) for track %u\t - %u samples, %s\n",
            i + 1, t, s, ref_track, samples, form_dur(dur, scale));
        break;
      case 'soun':
        fprintf(stderr, "Track %u: %s (%s)\t - %u samples, %s\n",
          i + 1, t, s, samples, form_dur(dur, scale));
        break;
      case 'vide':
        fprintf(stderr, "Track %u: %s (%s)\t - %ux%u pixel, %u samples, %s\n",
          i + 1, t, s, width, height, samples, form_dur(dur, scale));
        break;
    }
  }

  if (video_hint_track == -1) {
    fprintf(stderr, "No suitable hint track found.\n");
    return EXIT_FAILURE;
  }

  if (fv) gf_isom_close(fv);

  for (j = 0; j < loop; j++) {
    if (0 == (fv = gf_isom_open(cl[n], GF_ISOM_OPEN_READ, 0))) {
      fprintf(stderr, "Cannot open file %s: %s\n", cl[n], gf_error_to_string(gf_isom_last_error(0)));
      return EXIT_FAILURE;
    }
    if (GF_OK != (e = gf_isom_reset_hint_reader(fv, video_hint_track, 1, last_video_ts, last_video_sn, MP4_SSRC))) {
      fprintf(stderr, "gf_isom_reset_hint_reader: %s\n", gf_error_to_string(e));
      return EXIT_FAILURE;
    }
    if (audio_hint_track != -1) {
      if (0 == (fa = gf_isom_open(cl[n], GF_ISOM_OPEN_READ, 0))) {
        fprintf(stderr, "Cannot open file %s: %s\n", cl[n], gf_error_to_string(gf_isom_last_error(0)));
        return EXIT_FAILURE;
      }
      if (GF_OK != (e = gf_isom_reset_hint_reader(fa, audio_hint_track, 1, last_audio_ts, last_audio_sn, MP4_SSRC))) {
        fprintf(stderr, "gf_isom_reset_hint_reader: %s\n", gf_error_to_string(e));
        return EXIT_FAILURE;
      }
    }  

    rtp_video.M = 1;
    rtp_audio.M = 1;
    i = 0;

    if (mode & MODE_SEND) {
      ctx_vp.f          = fv;
      ctx_vp.framesize  = framesize;
      ctx_vp.frametype  = video_ft;
      ctx_vp.hint_track = video_hint_track;
      ctx_vp.sub_type   = video_sub_type;
      ctx_vp.time_scale = video_time_scale;
      ctx_vp.mode       = mode;
      ctx_vp.rtp        = &rtp_video;
      ctx_vp.segm       = segm;
      ctx_vp.count      = i;

      ctx_ap.f          = fa;
      ctx_ap.framesize  = 0;
      ctx_ap.frametype  = audio_ft;
      ctx_ap.hint_track = audio_hint_track;
      ctx_ap.sub_type   = audio_sub_type;
      ctx_ap.time_scale = audio_time_scale;
      ctx_ap.mode       = mode;
      ctx_ap.rtp        = &rtp_audio;
      ctx_ap.segm       = segm;
      ctx_ap.count      = i;

      ctx_vp.q          = &qv;
      ctx_ap.q          = &qa;
      ctx_vc.q          = &qv;
      ctx_ac.q          = &qa;

      get_cpu_freq();
      starttimer();
      if (!setdest(host, port, protocol, audio_hint_track != -1)) return error();
      if (!createq(&qv, VQ_LEN)) return error();
      qv.mode = mode;
      if (!createthread(&tvp, v_queuer, &ctx_vp)) return error();
      ctx_vp.t = &tvp;
      ctx_vp.active = 1;
      if (audio_hint_track != -1) {
        if (!createq(&qa, AQ_LEN)) return error();
        qa.mode = mode;
        if (!createthread(&tap, a_queuer, &ctx_ap)) return error();
        ctx_ap.t = &tap;
        ctx_ap.active = 1;
        if (!createthread(&tac, sender, &ctx_ac)) return error();
        ctx_ac.t = &tac;
        ctx_ac.active = 1;
      }
      if (!createthread(&tvc, sender, &ctx_vc)) return error();
      ctx_vc.t = &tvc;
      ctx_vc.active = 1;

      while (tvp.running || tap.running) { /* wait for producer thread[s] (reader) */
        SLEEP(1);
      }
      while (qv.len || qa.len) { /* wait for consumer thread[s] (sender) */
        SLEEP(1);
      }
      stopthread(&tvc); /* stop video sender */
      deleteq(&qv);
      if (audio_hint_track != -1) {
        stopthread(&tac); /* stop audio sender */
        deleteq(&qa);
      }
      cleanup();
    } else {
      for (ev = 0, ea = audio_hint_track == -1; !ev || !ea;) {
        if (!ev) {
          if (GF_OK != (e = gf_isom_next_hint_packet(fv, video_hint_track, &p, &l, &dis, &rep, &video_ts, &sn_v))) {
            if (e == GF_EOS) {
              ev = 1;
              continue;
            }
            fprintf(stderr, "gf_isom_next_hint_packet: %s\n", gf_error_to_string(e));
            return EXIT_FAILURE;
          }

          if (rtp_video.M) {
            switch (video_sub_type) {
              case 'avc1': video_ft = H264frametype(p); break;
              case 'mp4v':
              case 'MPEG': video_ft = MPEG4frametype(p); break;
              case 's263': video_ft = H263frametype(p); break;
              default    : video_ft = 4;
            }
          }

          segm++;
          framesize += l;
          fillRTPheader(&rtp_video, p);

          if (mode & MODE_PACKET) {
            printf("%u\t%c\t%.3f\t%u\t%.0f\t%u\n",
              rtp_video.id, video_ft["HIPBXXX"], rtp_video.timestamp / (double)video_time_scale, l, sendrate(&qv), queuelen(&qv));
          }
          if (mode & MODE_FRAME && rtp_video.M) {
            printf("%u\t%c\t%u\t%u\t%.3f\n", ++i, video_ft["HIPBXXX"], framesize, segm, curtime());
            segm = 0;
            framesize = 0;
          }
        }

        if (!ea) {
          if (GF_OK != (e = gf_isom_next_hint_packet(fa, audio_hint_track, &p, &l, &dis, &rep, &audio_ts, &sn_a))) {
            if (e == GF_EOS) {
              ea = 1;
              continue;
            }
            fprintf(stderr, "gf_isom_next_hint_packet: %s\n", gf_error_to_string(e));
            return EXIT_FAILURE;
          }
          switch (audio_sub_type) {
            case 'mp4a': audio_ft = A; break;
            default    : audio_ft = X;
          }
          fillRTPheader(&rtp_audio, p);

          if (mode & MODE_PACKET) {
            printf("%u\t%c\t%.3f\t%u\t%.0f\t%u\n",
              rtp_audio.id, audio_ft["XXXXXAX"], rtp_audio.timestamp / (double)audio_time_scale, l, sendrate(&qa), queuelen(&qa));
          }
          if (mode & MODE_FRAME) {
            printf("%u\t%c\t%u\t%u\t%.3f\n", rtp_audio.id, audio_ft["XXXXXAX"], l, 1, curtime());
          }
        }
      }
    }

    last_video_sn += sn_v;
    last_audio_sn += sn_a;
    last_video_ts += video_ts;
    last_audio_ts += audio_ts;

    if (fv) gf_isom_close(fv);
    if (fa) gf_isom_close(fa);
  }

  return 0;
}
