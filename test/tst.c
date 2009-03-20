#include "lib_debug.h"
#include "lib_httpd.h"
#include "lib_sock.h"
#include "lib_uuid.h"

int main(int argc, char *argv[])
{
  set_debug_level(DBG_GLOBAL, 100);
  libsupp_init(NULL);
  debug(0,0, "Hello there, sizeof of uuid_t: %d!", sizeof(uuid_t));
  http_start_listener("127.0.0.1", 12345, NULL);
  http_start_listener("127.0.0.1", 12346, NULL);
  while (1) {
    sock_one_cycle(1000, NULL);
  }
  return 1;
}
