/****************************************************************/
/*    NAME: Michael Shapiro                                     */
/*    ACCT: mws                                                 */
/*    FILE: StreamSocket.C                                      */
/*    DATE: Sat Apr 16 23:38:47 1994                            */
/****************************************************************/

#include "BTConfig.H"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif

#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#if HAVE_STROPTS_H && HAVE_STREAM_SOCKETS
# include <stropts.h>
#else
# include <sys/uio.h>
# include <stddef.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif


#include <iostream>
using namespace std;
#include <assert.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <cstring>

#include "StreamSocket.H"

#ifdef __EMSCRIPTEN__
EM_JS(int, bt_ws_connect, (int fd), {
  if (!Module.btNet) Module.btNet = { nextRoom: null, sock: {} };
  if (!Module.btNet.sock[fd]) {
    var base = Module.btWsUrl;
    if (!base) {
      var proto = location.protocol === "https:" ? "wss://" : "ws://";
      base = proto + location.hostname + ":8099";
    }
    var room = "battletris";
    try {
      var q = new URLSearchParams(location.search);
      room = q.get("room") || room;
    } catch (e) {}
    var url = base + (base.indexOf("?") >= 0 ? "&" : "?") + "room=" + encodeURIComponent(room);
    var s = { ws: null, q: [], open: 0, broken: 0, out: [] };
    var ws = new WebSocket(url);
    ws.binaryType = "arraybuffer";
    ws.onopen = function() {
      s.open = 1;
      for (var i = 0; i < s.out.length; i++) ws.send(s.out[i]);
      s.out = [];
    };
    ws.onmessage = function(ev) {
      var u = new Uint8Array(ev.data);
      for (var i = 0; i < u.length; i++) s.q.push(u[i]);
    };
    ws.onclose = function() { s.broken = 1; s.open = 0; };
    ws.onerror = function() { s.broken = 1; };
    s.ws = ws;
    Module.btNet.sock[fd] = s;
  }
  return 0;
});

EM_JS(int, bt_ws_send, (int fd, const char *buf, int len), {
  var s = Module.btNet && Module.btNet.sock ? Module.btNet.sock[fd] : null;
  if (!s || s.broken) return -1;
  var out = HEAPU8.slice(buf, buf + len);
  if (s.open) s.ws.send(out);
  else s.out.push(out);
  return len;
});

EM_JS(int, bt_ws_avail, (int fd), {
  var s = Module.btNet && Module.btNet.sock ? Module.btNet.sock[fd] : null;
  if (!s) return 0;
  return s.q.length | 0;
});

EM_JS(int, bt_ws_packet_ready, (int fd), {
  var s = Module.btNet && Module.btNet.sock ? Module.btNet.sock[fd] : null;
  if (!s) return 0;
  if (s.q.length < 8) return 0;
  var n = ((s.q[4] << 24) >>> 0) | ((s.q[5] << 16) >>> 0) | ((s.q[6] << 8) >>> 0) | (s.q[7] >>> 0);
  return s.q.length >= (8 + n) ? 1 : 0;
});

EM_JS(int, bt_ws_broken, (int fd), {
  var s = Module.btNet && Module.btNet.sock ? Module.btNet.sock[fd] : null;
  if (!s) return 0;
  return s.broken ? 1 : 0;
});

EM_JS(int, bt_ws_recv, (int fd, char *buf, int len, int peek), {
  var s = Module.btNet && Module.btNet.sock ? Module.btNet.sock[fd] : null;
  if (!s) return -1;
  if (s.q.length < len) return 0;
  for (var i = 0; i < len; i++) HEAPU8[buf + i] = s.q[i];
  if (!peek) s.q.splice(0, len);
  return len;
});

extern "C" int bt_wasm_socket_ready(int fd) {
  return bt_ws_packet_ready(fd);
}
#endif

StreamSocket::StreamSocket(InetAddress& addr)
#ifdef __EMSCRIPTEN__
: Socket(10000 + (rand() & 0x7fff)), in_peer_(0), un_peer_(0), in_addr_(addr)
#else
: Socket(socket(AF_INET, SOCK_STREAM, 0)), in_peer_(0), un_peer_(0),
  in_addr_(addr)
#endif
{
#ifndef __EMSCRIPTEN__
  int nodelay = 1;

  if(setsockopt(sock(), IPPROTO_TCP, TCP_NODELAY,
     (char *) &nodelay, sizeof(nodelay)) < 0)
    cerr << "StreamSocket: Warning: Failed to set TCP_NODELAY" << endl;
#endif
}

StreamSocket::StreamSocket(UnixAddress& addr)
: Socket(socket(AF_UNIX, SOCK_STREAM, 0)), in_peer_(0), un_peer_(0),
  un_addr_(addr)
{
}

StreamSocket::StreamSocket(const InetAddress& clientAddr, int clientSock,
			   const sockaddr_in& peerAddr)
: Socket(clientSock), in_peer_(new InetAddress(peerAddr)), un_peer_(0),
  in_addr_(clientAddr)
{
  int nodelay = 1;

  if(setsockopt(sock(), IPPROTO_TCP, TCP_NODELAY,
     (char *) &nodelay, sizeof(nodelay)) < 0)
    cerr << "StreamSocket: Warning: Failed to set TCP_NODELAY" << endl;
}

StreamSocket::StreamSocket(const UnixAddress& clientAddr, int clientSock,
			   const sockaddr_un& peerAddr)
: Socket(clientSock), un_peer_(new UnixAddress(peerAddr)), in_peer_(0),
  un_addr_(clientAddr)
{
}

StreamSocket::~StreamSocket()
{
  if(in_peer_)
    delete in_peer_;

  if(un_peer_)
    delete un_peer_;

#ifndef __EMSCRIPTEN__
  close(sock());
#endif
}

short StreamSocket::connect(InetAddress& peer)
{
#ifdef __EMSCRIPTEN__
  if (in_peer_ == 0)
    in_peer_ = new InetAddress(peer);
  if (bt_ws_connect(sock_) < 0)
    return ERRSTREAMCONNECT;
  return ERRSTREAMNOERR;
#else
  assert(in_peer_ == 0);
  in_peer_ = new InetAddress(peer);

  if(::connect(sock(), in_peer_->addr(), in_peer_->size()) < 0)
    return ERRSTREAMCONNECT;

#ifndef NDEBUG
  cout << "DEBUG: connected to port " << peer.port() << endl;
#endif

  return ERRSTREAMNOERR;
#endif
}

short StreamSocket::connect(InetAddress& peer, InetAddress& addr)
{
#ifdef __EMSCRIPTEN__
  (void)addr;
  return StreamSocket::connect(peer);
#else
  assert(in_peer_ == 0);
  in_peer_ = new InetAddress(peer);

  if(::connect(sock(), in_peer_->addr(), in_peer_->size()) < 0)
    return ERRSTREAMCONNECT;

#ifndef NDEBUG
  cout << "DEBUG: connected to port " << peer.port() << endl;
#endif

  sockaddr_in name;
  socklen_t namelen = sizeof(sockaddr_in);

  if(getsockname(sock(), (sockaddr *) &name, &namelen) < 0)
    return ERRSTREAMNAME;

  addr.addr((sockaddr *) &name, namelen);
  return ERRSTREAMNOERR;
#endif
}

short StreamSocket::connect(UnixAddress& peer)
{
  assert(un_peer_ == 0);
  un_peer_ = new UnixAddress(peer);

  if(::connect(sock(), un_peer_->addr(), un_peer_->size()) < 0)
    return ERRSTREAMCONNECT;

#ifndef NDEBUG
  cout << "DEBUG: connected to UNIX domain " << peer.path() << endl;
#endif

  return ERRSTREAMNOERR;
}

short StreamSocket::connect(UnixAddress& peer, UnixAddress& addr)
{
  assert(un_peer_ == 0);
  un_peer_ = new UnixAddress(peer);

  if(::connect(sock(), un_peer_->addr(), un_peer_->size()) < 0)
    return ERRSTREAMCONNECT;

#ifndef NDEBUG
  cout << "DEBUG: connected to UNIX domain " << peer.path() << endl;
#endif

  sockaddr_un name;
  socklen_t namelen = sizeof(sockaddr_un);

  if(getsockname(sock(), (sockaddr *) &name, &namelen) < 0)
    return ERRSTREAMNAME;

  addr.addr((sockaddr *) &name, namelen);
  return ERRSTREAMNOERR;
}

short StreamSocket::listen(int backlog)
{
#ifdef __EMSCRIPTEN__
  return ERRSTREAMNOERR;
#endif

  if(un_addr_) {
    assert(un_peer_ == 0);

    if(::bind(sock(), un_addr_.addr(), un_addr_.size()) < 0)
      return ERRSTREAMBIND;

    if(::listen(sock(), backlog) < 0)
      return ERRSTREAMLISTEN;
  } else {
    assert(in_peer_ == 0);

    if(::bind(sock(), in_addr_.addr(), in_addr_.size()) < 0)
      return ERRSTREAMBIND;

    if(::listen(sock(), backlog) < 0)
      return ERRSTREAMLISTEN;
  }

  return ERRSTREAMNOERR;
}

short StreamSocket::listen(int backlog, InetAddress& addr)
{
#ifdef __EMSCRIPTEN__
  return ERRSTREAMNOERR;
#endif

  assert(in_peer_ == 0);

  if(::bind(sock(), in_addr_.addr(), in_addr_.size()) < 0)
    return ERRSTREAMBIND;

  sockaddr_in name;
  socklen_t namelen = sizeof(sockaddr_in);

  if(getsockname(sock(), (sockaddr *) &name, &namelen) < 0)
    return ERRSTREAMNAME;

  addr.addr((sockaddr *) &name, namelen);

  if(::listen(sock(), backlog) < 0)
    return ERRSTREAMLISTEN;

  return ERRSTREAMNOERR;
}

short StreamSocket::listen(int backlog, UnixAddress& addr)
{
#ifdef __EMSCRIPTEN__
  return ERRSTREAMNOERR;
#endif

  assert(un_peer_ == 0);

  if(::bind(sock(), un_addr_.addr(), un_addr_.size()) < 0)
    return ERRSTREAMBIND;

  sockaddr_un name;
  socklen_t namelen = sizeof(name);

  if(getsockname(sock(), (sockaddr *) &name, &namelen) < 0)
    return ERRSTREAMNAME;

  UnixAddress boundaddr(name);
  addr = boundaddr;

  if(::listen(sock(), backlog) < 0)
    return ERRSTREAMLISTEN;

  return ERRSTREAMNOERR;
}

short StreamSocket::accept(StreamSocket *& sockptr)
{
  if(un_addr_) {
    assert(un_peer_ == 0);

    sockaddr_un peerAddr;
    socklen_t peerSize = sizeof(sockaddr_un);

    int clientSock = ::accept(sock(), (sockaddr *) &peerAddr, &peerSize);

    sockaddr_un address;
    socklen_t size = sizeof(sockaddr_un);
   
    bzero((char *) &address, sizeof(sockaddr_un));

    if(getsockname(clientSock, (sockaddr *) &address, &size) < 0)
      return ERRSTREAMNAME;

    UnixAddress clientAddr(address);

#ifndef NDEBUG
    cout << "DEBUG: connection to client established at path "
	 << clientAddr.path() << endl;
#endif

    sockptr = new StreamSocket(clientAddr, clientSock, peerAddr);

  } else {

    assert(in_peer_ == 0);

    sockaddr_in peerAddr;
    socklen_t peerSize = sizeof(sockaddr_in);

    int clientSock = ::accept(sock(), (sockaddr *) &peerAddr, &peerSize);

    sockaddr_in address;
    socklen_t size = sizeof(sockaddr_in);

    bzero((char *) &address, sizeof(sockaddr_in));

    if(getsockname(clientSock, (sockaddr *) &address, &size) < 0)
      return ERRSTREAMNAME;

    InetAddress clientAddr(address);

#ifndef NDEBUG
    cout << "DEBUG: connection to client established on port "
	 << clientAddr.port() << endl;
#endif

    sockptr = new StreamSocket(clientAddr, clientSock, peerAddr);
  }

  return ERRSTREAMNOERR;
}

short StreamSocket::sendbuf(char *buf, int buflen, Address *dst)
{
#ifdef __EMSCRIPTEN__
  (void)dst;
  if (!in_peer_ && !un_peer_)
    return ERRSTREAMSEND;
  if (bt_ws_send(sock_, buf, buflen) < 0)
    return bt_ws_broken(sock_) ? ERRSTREAMBROKEN : ERRSTREAMSEND;
  return ERRSTREAMNOERR;
#else
  assert(dst == 0);
  assert((in_peer_ != 0) || (un_peer_ != 0));

  const char *bufptr = buf;
  int nleft = buflen;
  int nsent;

  while(nleft > 0) {
    if((nsent = ::send(sock(), bufptr, nleft, 0)) < 0) {
      if(errno == EINTR)
        continue;
      return ERRSTREAMSEND;
    }

    nleft -= nsent;
    bufptr += nsent;
  }

  return ERRSTREAMNOERR;
#endif
}

short StreamSocket::recvbuf(char *buf, int buflen, Address *src)
{
#ifdef __EMSCRIPTEN__
  (void)src;
  if (bt_ws_broken(sock_))
    return ERRSTREAMBROKEN;
  int n = bt_ws_recv(sock_, buf, buflen, 0);
  if (n == buflen)
    return ERRSTREAMNOERR;
  return ERRSTREAMTIMEOUT;
#else
  assert(src == 0);
  assert((in_peer_ != 0) || (un_peer_ != 0));

  assert(buf != 0);
  assert(buflen > 0);

  char *bufptr = buf;
  int nleft = buflen;
  int nrecv;

  while(nleft > 0) {
    if((nrecv = ::recv(sock(), bufptr, nleft, 0)) < 0) {
      if(errno == EINTR)
        continue;
      return ERRSTREAMRECV;
    } else if(nrecv == 0) {
      return ERRSTREAMBROKEN;
    }

    nleft -= nrecv;
    bufptr += nrecv;
  }

  return ERRSTREAMNOERR;
#endif
}

short StreamSocket::peekbuf(char *buf, int buflen, Address *src)
{
#ifdef __EMSCRIPTEN__
  (void)src;
  if (bt_ws_broken(sock_))
    return ERRSTREAMBROKEN;
  int n = bt_ws_recv(sock_, buf, buflen, 1);
  if (n == buflen)
    return ERRSTREAMNOERR;
  return ERRSTREAMTIMEOUT;
#else
  assert(src == 0);
  assert((in_peer_ != 0) || (un_peer_ != 0));

  assert(buf != 0);
  assert(buflen > 0);

  char *bufptr = buf;
  int nleft = buflen;
  int npeek;

  while(nleft > 0) {
    if((npeek = ::recv(sock(), bufptr, nleft, MSG_PEEK)) < 0) {
      if(errno == EINTR)
        continue;
      return ERRSTREAMRECV;
    } else if(npeek == 0) {
      return ERRSTREAMBROKEN;
    }

    nleft -= npeek;
    bufptr += npeek;
  }

  return ERRSTREAMNOERR;
#endif
}

short StreamSocket::recvbuf(char *buf, int buflen, timeval& delay, Address *src)
{
#ifdef __EMSCRIPTEN__
  (void)delay;
  return StreamSocket::recvbuf(buf, buflen, src);
#else
  assert(src == 0);
  assert((in_peer_ != 0) || (un_peer_ != 0));

  assert(buf != 0);
  assert(buflen > 0);

  SELECTARGTYPE set;
  char *bufptr = buf;
  int nleft = buflen;
  int nrecv;

  while(nleft > 0) {
    FD_ZERO(&set);
    FD_SET(sock(), &set);

    if(select(sock() + 1, (SELECTARGTYPE *) &set, (SELECTARGTYPE *) 0,
       (SELECTARGTYPE *) 0, &delay) < 0)
      return ERRSTREAMSELECT;

    if(!(FD_ISSET(sock(), &set)))
      return ERRSTREAMTIMEOUT;

    if((nrecv = ::recv(sock(), bufptr, nleft, 0)) < 0) {
      if(errno == EINTR)
        continue;
      return ERRSTREAMRECV;
    } else if(nrecv == 0) {
      return ERRSTREAMBROKEN;
    }

    nleft -= nrecv;
    bufptr += nrecv;
  }

  return ERRSTREAMNOERR;
#endif
}

short StreamSocket::peekbuf(char *buf, int buflen, timeval& delay, Address *src)
{
#ifdef __EMSCRIPTEN__
  (void)delay;
  return StreamSocket::peekbuf(buf, buflen, src);
#else
  assert(src == 0);
  assert((in_peer_ != 0) || (un_peer_ != 0));

  assert(buf != 0);
  assert(buflen > 0);

  SELECTARGTYPE set;
  char *bufptr = buf;
  int nleft = buflen;
  int npeek;

  while(nleft > 0) {
    FD_ZERO(&set);
    FD_SET(sock(), &set);

    if(select(sock() + 1, (SELECTARGTYPE *) &set, (SELECTARGTYPE *) 0,
       (SELECTARGTYPE *) 0, &delay) < 0)
      return ERRSTREAMSELECT;

    if(!(FD_ISSET(sock(), &set)))
      return ERRSTREAMTIMEOUT;

    if((npeek = ::recv(sock(), bufptr, nleft, MSG_PEEK)) < 0) {
      if(errno == EINTR)
        continue;
      return ERRSTREAMRECV;
    } else if(npeek == 0) {
      return ERRSTREAMBROKEN;
    }

    nleft -= npeek;
    bufptr += npeek;
  }

  return ERRSTREAMNOERR;
#endif
}

int StreamSocket::ready()
{
#ifdef __EMSCRIPTEN__
  return bt_ws_avail(sock_) > 0;
#else
  timeval now;
  SELECTARGTYPE set;

  FD_ZERO(&set);
  FD_SET(sock(), &set);

  return select(sock() + 1, (SELECTARGTYPE *) &set, (SELECTARGTYPE *) 0,
                (SELECTARGTYPE *) 0, &now) > 0;
#endif
}

int StreamSocket::ready(timeval& delay)
{
#ifdef __EMSCRIPTEN__
  (void)delay;
  return StreamSocket::ready();
#else
  SELECTARGTYPE set;

  FD_ZERO(&set);
  FD_SET(sock(), &set);

  return select(sock() + 1, (SELECTARGTYPE *) &set, (SELECTARGTYPE *) 0,
                (SELECTARGTYPE *) 0, &delay) > 0;
#endif
}

short StreamSocket::sendfd(int fd)
{
  assert(un_peer_ != 0);
  assert(fd >= 0);

  char buf[2];		// Our own 2-byte header

  buf[0] = 0;		// Header byte 0: Flags
  buf[1] = 0;		// Header byte 1: Status (Non-zero status is an error)

  if(fd < 0)
    buf[1] = 1;		// Catch bad fd arg at runtime using header info

#if HAVE_STROPTS_H && HAVE_STREAM_SOCKETS

  if(write(sock(), buf, sizeof(buf)) != sizeof(buf))
    return ERRSTREAMSEND;

  if(fd >= 0) {
    if(ioctl(sock(), I_SENDFD, (char *) fd) < 0)
      return ERRSTREAMSEND;
  }

#else			// Either Pre-4.4BSD-based or 4.4BSD-based

  struct iovec iov[1];
  struct msghdr msg;

  iov[0].iov_base = buf;
  iov[0].iov_len = sizeof(buf);

  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;

# ifdef SCM_RIGHTS	// 4.4BSD-based or SunOS 5.6+

  char cmbuf[sizeof(struct cmsghdr) + sizeof(int)];
  struct cmsghdr *cmptr = (struct cmsghdr *) cmbuf;

  if(fd < 0) {
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
  } else {
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    cmptr->cmsg_len = sizeof(struct cmsghdr) + sizeof(int);

    msg.msg_control = (caddr_t) cmptr;
    msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);
    *((int *) CMSG_DATA(cmptr)) = fd;
  }

  if(sendmsg(sock(), &msg, 0) != 2)
    return ERRSTREAMSEND;

# else			// Assume Pre-4.4BSD-based

  if(fd < 0) {
    msg.msg_accrights = NULL;
    msg.msg_accrightslen = 0;
  } else {
    msg.msg_accrights = (caddr_t) &fd;
    msg.msg_accrightslen = sizeof(int);
  }

  if(sendmsg(sock(), &msg, 0) != sizeof(buf))
    return ERRSTREAMSEND;

# endif

#endif

  return ERRSTREAMNOERR;
}

short StreamSocket::recvsock_in(StreamSocket *& sockptr)
{
  sockaddr_in peerAddr;
  socklen_t peerSize = sizeof(sockaddr_in);
  int clientSock;
  short err;

  if((err = StreamSocket::recvfd(clientSock)) < 0)
    return err;

  sockaddr_in address;
  socklen_t size = sizeof(sockaddr_in);

  bzero((char *) &address, sizeof(sockaddr_in));

  if(getsockname(clientSock, (sockaddr *) &address, &size) < 0)
    return ERRSTREAMNAME;

  InetAddress clientAddr(address);

#ifndef NDEBUG
  cout << "DEBUG: established connection to client on port "
       << clientAddr.port() << endl;
#endif

  bzero((char *) &peerAddr, sizeof(sockaddr_in));
  size = sizeof(sockaddr_in);

  if(getpeername(clientSock, (sockaddr *) &peerAddr, &size) < 0)
    return ERRSTREAMNAME;

  sockptr = new StreamSocket(clientAddr, clientSock, peerAddr);
  return ERRSTREAMNOERR;
}

short StreamSocket::recvfd(int& filedes)
{
  assert(un_peer_ != 0);

  int newfd, nread, flag, status;
  char *ptr, buf[256];

  status = -1;

#if HAVE_STROPTS_H && HAVE_STREAM_SOCKETS

  struct strbuf dat;
  struct strrecvfd recvfd;

  for(;;) {
    dat.buf = buf;
    dat.maxlen = sizeof(buf);

    flag = 0;

    if(::getmsg(sock(), NULL, &dat, &flag) < 0)
      return ERRSTREAMRECV;

    nread = dat.len;

    if(nread == 0)
      return ERRSTREAMBROKEN;

    for(ptr = buf; ptr < &buf[nread];) {
      if(*ptr++ == 0) {
	if(ptr != &buf[nread - 1])
	  return ERRSTREAMRECV;

	status = *ptr & 255;

	if(status == 0) {
	  if(::ioctl(sock(), I_RECVFD, &recvfd) < 0)
	    return ERRSTREAMRECV;
	  newfd = recvfd.fd;
	}

	nread -= 2;	// Our protocol header is 2 bytes long
      }
    }

    if(status >= 0) {
      filedes = newfd;
      return ERRSTREAMNOERR;
    }
  }

#else			// Most likely BSD-based or SunOS 5.6+ based

# ifdef SCM_RIGHTS	// 4.4BSD-based or SunOS 5.6+ based

  struct iovec iov[1];
  struct msghdr msg;

  char cmbuf[sizeof(struct cmsghdr) + sizeof(int)];
  struct cmsghdr *cmptr = (struct cmsghdr *) cmbuf;

  for(;;) {
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    msg.msg_control = (caddr_t) cmptr;
    msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);

    if((nread = recvmsg(sock(), &msg, 0)) < 0)
      return ERRSTREAMRECV;
    else if(nread == 0)
      return ERRSTREAMBROKEN;

    for(ptr = buf; ptr < &buf[nread];) {
      if(*ptr++ == 0) {
	if(ptr != &buf[nread - 1])
	  return ERRSTREAMRECV;

	status = *ptr & 255;

	if(status == 0) {
	  if(msg.msg_controllen != sizeof(struct cmsghdr) + sizeof(int))
	    return ERRSTREAMRECV;
	  newfd = *(int *) CMSG_DATA(cmptr);
	}

	nread -= 2;	// Our protocol header is 2 bytes
      }
    }

    if(status >= 0) {
      filedes = newfd;
      return ERRSTREAMNOERR;
    }
  }

# else			// Assume Pre-4.4BSD-based

  struct iovec iov[1];
  struct msghdr msg;

  for(;;) {
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_accrights = (caddr_t) &newfd;
    msg.msg_accrightslen = sizeof(int);

    if((nread = recvmsg(sock(), &msg, 0)) < 0)
      return ERRSTREAMRECV;
    else if(nread == 0)
      return ERRSTREAMBROKEN;

    for(ptr = buf; ptr < &buf[nread];) {
      if(*ptr++ == 0) {
	if(ptr != &buf[nread - 1])
	  return ERRSTREAMRECV;

	status = *ptr & 255;

	if(status == 0) {
	  if(msg.msg_accrightslen != sizeof(int))
	    return ERRSTREAMRECV;
	}

	nread -= 2;
      }
    }

    if(status >= 0) {
      filedes = newfd;
      return ERRSTREAMNOERR;
    }
  }

# endif

#endif			// End of #ifdef juju

}
