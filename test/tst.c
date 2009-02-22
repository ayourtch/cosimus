#include "lib_debug.h"
#include "lib_httpd.h"
#include "lib_sock.h"

int main(int argc, char *argv[])
{
  set_debug_level(DBG_GLOBAL, 100);
  debug(0,0, "Hello there!");
  http_start_listener("127.0.0.1", 12345, NULL);
  http_start_listener("127.0.0.1", 12346, NULL);
  while (1) {
    sock_one_cycle(1000);
  }
  return 1;
}
