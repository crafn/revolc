#include "core/socket.h"
#include "core/string.h"

const char *ip_str(IpAddress addr)
{
	return frame_str("%i.%i.%i.%i:%i",
			addr.a, addr.b, addr.c, addr.d,
			addr.port);
}

