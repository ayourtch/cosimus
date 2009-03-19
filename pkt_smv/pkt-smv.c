#include "lib_debug.h"
#include "lib_dbuf.h"
#include "lib_sock.h"
#include "lib_uuid.h"
#include "fmv.h"


int smv_packet(int idx, dbuf_t *d) {
  debug(0,0, "Packet: %s", global_id_str(get_packet_global_id(d->buf)));
  return 1;
}

int start_smv_listener(char *addr, int port) {
  sock_handlers_t *h;
  int idx;

  idx = bind_udp_listener_specific(addr, port, NULL);
  h = cdata_get_handlers(idx);
  h->ev_read = smv_packet;
  return idx;
}

int main(int argc, char *argv[])
{
  set_debug_level(DBG_GLOBAL, 100);
  debug(0,0, "Starting SL(tm)-compatible packet listener");

  start_smv_listener("0.0.0.0", 9000);

  while (1) {
    sock_one_cycle(1000);
  }
  return 1;
}
