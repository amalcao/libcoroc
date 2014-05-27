#ifndef _LIBTSC_DIST_H_
#define _LIBTSC_DIST_H_

#include "libtsc.h"

#define TSC_SERVICE_INFINITY (uint64_t)(-1)

enum { TSC_SERVICE_LOCAL = 1, TSC_SERVICE_REMOTE_TCP = 2, };

struct tsc_service;
struct tsc_message;

/* the service callback function type */
typedef int (*tsc_service_cb_t)(struct tsc_service *, void *);
/* the service uniq id type */
typedef uint64_t tsc_service_id_t;

/* define the service structure */
typedef struct tsc_service {
  const char *name;
  tsc_service_id_t id;
  uint32_t type;
  const char *host;
  int port;
  int socket;
  uint64_t timeout;
  bool auto_start;
  uint32_t clients;  // total number of the connected clients
  tsc_chan_t inbound;
  tsc_chan_t quit;
  tsc_service_cb_t routine;
  tsc_service_cb_t init_callback;
  tsc_service_cb_t error_callback;
  tsc_service_cb_t timeout_callback;
} *tsc_service_t;

/* FIXME: the definition of common message structure */
typedef struct tsc_message {
  int len;
  tsc_service_id_t src;
  tsc_service_id_t dst;
  char content[0];
} *tsc_message_t;

extern struct tsc_service *__start_tsc_services_section;
extern struct tsc_service *__stop_tsc_services_section;

#define TSC_SERVICE_BEGIN() (tsc_service_t *)(&__start_tsc_services_section)
#define TSC_SERVICE_END() (tsc_service_t *)(&__stop_tsc_services_section)

#define TSC_SERVICE_LOCAL_IP 0

#define TSC_SERVICE_DECLARE_LOCAL(_service, _id, _name, _timeout, _auto,      \
                                  _routine, _init_cb, _error_cb, _timeout_cb) \
  struct tsc_service _service = {.id = _id,                                   \
                                 .name = _name,                               \
                                 .type = TSC_SERVICE_LOCAL,                   \
                                 .timeout = _timeout,                         \
                                 .auto_start = _auto,                         \
                                 .routine = _routine,                         \
                                 .init_callback = _init_cb,                   \
                                 .error_callback = _error_cb,                 \
                                 .timeout_callback = _timeout_cb};            \
  struct tsc_service *__attribute((__section__("tsc_services_section")))      \
  __attribute((__used__)) _service##_address = &(_service)

#define TSC_SERVICE_DECLARE_TCP(_service, _id, _name, _timeout, _auto, _host, \
                                _port, _routine, _init_cb, _error_cb,         \
                                _timeout_cb)                                  \
  struct tsc_service _service = {                                             \
      .id = _id,                                                              \
      .name = _name,                                                          \
      .type = TSC_SERVICE_LOCAL | TSC_SERVICE_REMOTE_TCP,                     \
      .host = _host,                                                          \
      .port = _port,                                                          \
      .timeout = _timeout,                                                    \
      .auto_start = _auto,                                                    \
      .routine = _routine,                                                    \
      .init_callback = _init_cb,                                              \
      .error_callback = _error_cb,                                            \
      .timeout_callback = _timeout_cb};                                       \
  struct tsc_service *__attribute((__section__("tsc_services_section")))      \
  __attribute((__used__)) _service##_address = &(_service)

#define TSC_SERVICE_ID2IP(id) (uint32_t)((id) >> 32)
#define TSC_SERVICE_ID2PORT(id) (int)(((id) >> 16) & 0xffffUL)

/* the public interfaces for server end */
int tsc_init_all_services(void);
int tsc_service_register(tsc_service_t service);
int tsc_service_start(tsc_service_t service, bool backend);
int tsc_service_reload(tsc_service_t service);
int tsc_service_quit(tsc_service_t service, int info);

/* the public interfaces for client end */
int tsc_service_lookup(const char *name, tsc_service_id_t *id);
int tsc_service_connect(tsc_service_t self, tsc_service_id_t target);
int tsc_service_disconnect(tsc_service_t self, tsc_service_id_t target);

/* the public interfaces for messages passing */
tsc_message_t tsc_message_alloc(tsc_service_id_t src, int len);
void tsc_message_dealloc(tsc_message_t message);
int tsc_message_send(tsc_message_t message, tsc_service_id_t dst);

#endif  // _LIBTSC_DIST_H_
