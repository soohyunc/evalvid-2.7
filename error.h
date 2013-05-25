#ifndef TRACE_ERR
#define TRACE_ERR

#define USE(x) (void)(x)

enum err {
  err_none,
  err_IO = 0x8a4500, /* file I/O error */
  err_NM,            /* not enough memory */
  err_EOF,           /* unexpected end of file */
  err_SE,            /* bit-stream syntax error */
  err_PT,            /* extended PTYPE not yet supported */
  err_PS,            /* RTP packet too big */
  err_SI,            /* socket initialization failed */
  err_UH,            /* unknown host */
  err_CF,            /* socket connect failed */
  err_CT,            /* thread creation failed */
  err_CL,            /* lock creation failed */
  err_LT,            /* locking failed */
  err_UT,            /* unlocking failed */
  err_CS,            /* mp4 container structure error */
  err_MS,            /* more sample entries in stsz than in stts */
  err_SM             /* strange mdat */
};

#ifdef __cplusplus
extern "C" {
#endif

void seterror(enum err);
int error(void);

#ifdef __cplusplus
}
#endif

#endif
