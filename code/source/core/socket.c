#include "core/socket.h"
#include "core/string.h"

const char *ip_to_str(IpAddress addr)
{
	return frame_str("%i.%i.%i.%i:%i",
			addr.a, addr.b, addr.c, addr.d,
			addr.port);
}

IpAddress str_to_ip(const char *str)
{
	int a, b, c, d, port;
	int ret= sscanf(str, "%i.%i.%i.%i:%i", &a, &b, &c, &d, &port);
	if (ret < 4)
		fail("Ip parsing failed: %s", str);
	return (IpAddress) {a, b, c, d, port};
}

