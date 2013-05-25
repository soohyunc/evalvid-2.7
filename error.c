/**
 * error.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "error.h"

/* private */

static enum err E_;

/* public */

void seterror(enum err e) { E_ = e; }

int error()
{
  switch (E_) {
    case err_IO  : fprintf(stderr, "File I/O error.\n"); break;
    case err_NM  : fprintf(stderr, "Not enough memory.\n"); break;
    case err_EOF : /* fprintf(stderr, "Unexpected end of file.\n"); */ break;
    case err_SE  : fprintf(stderr, "Bit-stream syntax error.\n"); break;
    case err_PT  : fprintf(stderr, "Extended PTYPE not yet supported.\n"); break;
    case err_PS  : fprintf(stderr, "Packet too big.\n"); break;
    case err_SI  : fprintf(stderr, "Socket initialization failed.\n"); break;
    case err_UH  : fprintf(stderr, "Unknown host.\n"); break;
    case err_CF  : fprintf(stderr, "Socket connect failed.\n"); break;
    case err_CT  : fprintf(stderr, "Thread creation failed.\n"); break;
    case err_LT  : fprintf(stderr, "Thread locking failed.\n"); break;
    case err_UT  : fprintf(stderr, "Thread unlocking failed.\n"); break;
    case err_CS  : fprintf(stderr, "MP4 container structure error.\n"); break;
    case err_MS  : fprintf(stderr, "More sample entries in stsz than in stts.\n"); break;
    case err_SM  : fprintf(stderr, "Strange 'mdat'.\n"); break;
    case err_none:
    default      : return 0;
  }
  E_ = err_none;
  return EXIT_FAILURE;
}
