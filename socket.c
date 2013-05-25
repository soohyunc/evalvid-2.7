
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <Ws2tcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
#endif

#include "error.h"
#include "rtp.h"
#include "socket.h"
#include "timing.h"

/* private */

static sock_t v_sock_, a_sock_;
static int max_packet_size_;

/* public */

int setdest(char *h, unsigned short port, enum prot proto, int audio)
{
  int type, v, l = sizeof v;

#if defined(_WIN32)
  WSADATA wsa = { 0 };
  if (WSAStartup(0x202, &wsa) == 0)
    if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2)
      goto SI;
#endif

  type = proto == TCP ? SOCK_STREAM : SOCK_DGRAM;

  if (strchr(h, ':')) {

    struct addrinfo *aiv;
    struct addrinfo *aia;
    char pv[6];
    char pa[6];

    sprintf(pv, "%d", port);
    sprintf(pa, "%d", port + 2);

    if (getaddrinfo(h, pa, 0, &aiv) != 0) goto UH;
    if ((v_sock_ = socket(AF_INET6, type, 0)) < 0) goto SI;
    if (connect(v_sock_, aiv->ai_addr, aiv->ai_addrlen) < 0) goto CF;
    freeaddrinfo(aiv);

#if defined(_WIN32)
    max_packet_size_ = 0 == getsockopt(v_sock_, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&v, &l) ? v : 1500;
#else
    max_packet_size_ = 0 == getsockopt(v_sock_, SOL_SOCKET, SO_SNDBUF, &v, &l) ? v : 1500;
#endif

    if (audio) {
      if (getaddrinfo(h, pv, 0, &aia) != 0) goto UH;
      if ((a_sock_ = socket(AF_INET6, type, 0)) < 0) goto SI;
      if (connect(a_sock_, aia->ai_addr, aia->ai_addrlen) < 0) goto CF;
      freeaddrinfo(aia);
    }

  } else {

    struct hostent *host;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;

    if ((host = gethostbyname(h)) == 0) goto UH;
    memcpy(&addr.sin_addr, host->h_addr, host->h_length);

    if ((v_sock_ = socket(AF_INET, type, 0)) < 0) goto SI;
    addr.sin_port = htons(port);
    if (connect(v_sock_, (struct sockaddr *) &addr, sizeof addr) < 0) goto CF;

#if defined(_WIN32)
    max_packet_size_ = 0 == getsockopt(v_sock_, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&v, &l) ? v : 1500;
#else
    max_packet_size_ = 0 == getsockopt(v_sock_, SOL_SOCKET, SO_SNDBUF, &v, &l) ? v : 1500;
#endif

    if (audio) {
      if ((a_sock_ = socket(AF_INET, type, 0)) < 0) goto SI;
      addr.sin_port = htons(port + 2);
      if (connect(a_sock_, (struct sockaddr *) &addr, sizeof addr) < 0) goto CF;
    }
  }

  return 1;

SI: seterror(err_SI); goto X;
UH: seterror(err_UH); goto X;
CF: seterror(err_CF);
X:  return 0;
}

int sendbuf(unsigned char *p, unsigned l, enum ptype t)
{
  int sent;

  if (l != (sent = send(t == VIDEO ? v_sock_ : a_sock_, (char *)p, l, 0))) {
#if defined(_WIN32)
    fprintf(stderr, "send (WSA Error: %d)\n", WSAGetLastError());
#else
    perror("send");
#endif
    fprintf(stderr, "- check if a receiver is listening on the destination port\n");
    fprintf(stderr, "- check the max. packet size (MTU) for your system and transport protocol (probably %u byte)\n", max_packet_size_);
    SLEEP(0);
  }

  return sent;
}

void cleanup()
{
#if defined(_WIN32)
  WSACleanup();
#endif
}
