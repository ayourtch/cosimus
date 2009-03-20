#include "lib_sock.h"

typedef struct {
  void *libsock_data;
} libsupp_data_t;

static libsupp_data_t *libsupp_data = NULL;

#define FREE_DATA(data) do { if (libsupp_data->data) { free(libsupp_data->data); libsupp_data->data = NULL; } } while(0)
#define IS_NULL(data) (libsupp_data->data == NULL)
#define CHECK_INIT(data, func) libsupp_data->data = func(libsupp_data->data)

void *libsupp_init(void *data)
{
  if(data == NULL) {
    data = calloc(1, sizeof(libsupp_data_t));
  }
  if(data) {
    libsupp_data = data;
    CHECK_INIT(libsock_data, libsock_init);
    if(IS_NULL(libsock_data) ) {
      FREE_DATA(libsock_data);
      free(libsupp_data);
      data = libsupp_data = NULL;
    }
  }
  return libsupp_data;
}

