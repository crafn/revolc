#ifndef REVOLC_CORE_SOCKET_H
#define REVOLC_CORE_SOCKET_H

#include "build.h"

typedef S64 Socket;
typedef struct IpAddress {
	U8 a, b, c, d;
	U16 port;
} IpAddress;

REVOLC_API const char *ip_str(IpAddress addr);
static bool ip_equals(IpAddress ip1, IpAddress ip2)
{ return	ip1.a == ip2.a && ip1.b == ip2.b &&
		ip1.c == ip2.c && ip1.d == ip2.d &&
		ip1.port == ip2.port; }

REVOLC_API int socket_error();
REVOLC_API Socket invalid_socket();

REVOLC_API Socket open_udp_socket(U16 port);
REVOLC_API void close_socket(Socket *fd);

REVOLC_API U32 send_packet(Socket sock, IpAddress addr, const void *data, U32 size);
REVOLC_API U32 recv_packet(Socket sock, IpAddress *addr, void *dst, U32 dst_size);

#endif // REVOLC_CORE_SOCKET_H
