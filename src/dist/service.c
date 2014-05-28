#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service.h"
#include "../time.h"
#include "../hash.h"
#include "../netpoll.h"

// ===== the record hash map =======
static hash_t __tsc_service_record_table;

static inline void __tsc_service_record_init(void) {
  atomic_hash_init(&__tsc_service_record_table);
}

static inline int __tsc_service_record_insert(tsc_service_id_t id,
                                              tsc_chan_t comm) {
  int ret = atomic_hash_insert(&__tsc_service_record_table, (uint64_t)id,
                               (void *)comm);
  if (ret == 0) tsc_refcnt_get(comm);
  return ret;
}

static inline tsc_chan_t __tsc_service_record_get(tsc_service_id_t id) {
  return (tsc_chan_t)atomic_hash_get(&__tsc_service_record_table, (uint64_t)id,
                                     false);
}

static inline int __tsc_service_record_delete(tsc_service_id_t id) {
  tsc_chan_t comm = (tsc_chan_t)atomic_hash_get(&__tsc_service_record_table,
                                                (uint64_t)id, true);
  if (comm != NULL) {
    tsc_refcnt_put(comm);
    return 0;
  }
  return -1;
}

static inline void __tsc_service_record_enum(void) {
  // TODO
}

static inline void __tsc_service_record_next(tsc_service_id_t *id,
                                             tsc_chan_t *comm) {
  // TODO
}

static int __tsc_pump_message(tsc_service_t service) {
  tsc_message_t message;
  int info, error;

  tsc_chan_t _chan = NULL;
  tsc_timer_t timer = tsc_timer_allocate(0, 0);
  tsc_chan_set_t set = tsc_chan_set_allocate(3);

  // add the channels to the set
  tsc_chan_t comm = tsc_refcnt_get(service->inbound);
  tsc_chan_t quit = tsc_refcnt_get(service->quit);

  tsc_chan_set_recv(set, comm, &message);
  tsc_chan_set_recv(set, quit, &info);
  tsc_chan_set_recv(set, (tsc_chan_t)timer, 0);

  // start the select loop ..
  for (;;) {
    // set the timer ..
    if (service->timeout != TSC_SERVICE_INFINITY)
      tsc_timer_after(timer, service->timeout);

    tsc_chan_set_select(set, &_chan);
    if (comm == _chan) {
      if (message->len < 0) {
        // TODO
        error = -1;
      } else {
        error = service->routine(service, message);
      }
      if (error && service->error_callback)
        service->error_callback(service, &error);
      // the message should be freed here!!
      tsc_message_dealloc(message);
      message = NULL;
    } else if ((tsc_chan_t)timer == _chan) {
      service->timeout_callback(service, NULL);
    } else if (quit == _chan) {
      goto __tsc_message_pump_exit;
    }
  }

__tsc_message_pump_exit:
  // release the resources ..
  tsc_chan_set_dealloc(set);
  tsc_timer_stop(timer);
  tsc_timer_dealloc(timer);

  // close the channels ..
  tsc_chan_close(comm);
  tsc_chan_close(quit);

  // drop the references ..
  tsc_refcnt_put(comm);
  tsc_refcnt_put(quit);

  if (service->type & TSC_SERVICE_REMOTE_TCP) {
    // TODO: notify the service side to quit !!
    close(service->socket);
  }

  return 0;
}

static int tsc_service_accept(tsc_service_t service);

int tsc_service_register(tsc_service_t service) {
  assert(service != NULL);

  // ignore the dummy service ..
  if (service->id == 0) return 0;

  // create the internal channel for messages ..
  service->inbound =
      tsc_refcnt_get(tsc_chan_allocate(sizeof(tsc_message_t), 10));
  // create the channel for quiting ..
  service->quit = tsc_refcnt_get(tsc_chan_allocate(sizeof(int), 1));

  // register current service as a local service ..
  __tsc_service_record_insert(service->id, service->inbound);

  // assemble the final id ..
  if (service->type & TSC_SERVICE_REMOTE_TCP) {
    uint32_t ip = 0;
    tsc_net_lookup(service->host, &ip);
    service->id |= (((uint64_t)ip) << 32);
    service->id |= ((uint32_t)(service->port) << 16);
  }

  return 0;
}

int tsc_service_start(tsc_service_t service, bool backend) {
  assert(service != NULL);

  // create a coroutine to establish the TCP server..
  if (service->type & TSC_SERVICE_REMOTE_TCP)
    tsc_coroutine_spawn(tsc_service_accept, (void *)service, "tcp");

  if (backend) {
    tsc_coroutine_spawn(__tsc_pump_message, (void *)service, "pump");
  } else {
    __tsc_pump_message(service);
  }
  return 0;
}

int tsc_service_quit(tsc_service_t service, int info) {
  assert(service && service->quit);
  tsc_chan_send(service->quit, &info);
  tsc_refcnt_put(service->quit);
  return 0;
}

int tsc_service_reload(tsc_service_t service) {
  tsc_service_quit(service, 0);
  tsc_service_start(service, true);  // TODO
  return 0;
}

/// FIXME: we need to just define one service to create the section
TSC_SERVICE_DECLARE_LOCAL(__dummy_service, 0, "dummy", TSC_SERVICE_INFINITY,
                          false, NULL, NULL, NULL, NULL);

// start all the services ..
int tsc_init_all_services(void) {
  int id = 0;
  tsc_service_t *ser;

  // init the local records table ..
  __tsc_service_record_init();

  // first, register all the local services ..
  for (ser = TSC_SERVICE_BEGIN(); ser < TSC_SERVICE_END(); ++ser)
    tsc_service_register(*ser);

  // then, calling the initial functions of each service ..
  for (ser = TSC_SERVICE_BEGIN(); ser < TSC_SERVICE_END(); ++ser) {
    if ((*ser)->init_callback) (*ser)->init_callback(*ser, NULL);  // FIXME
    if ((*ser)->auto_start) tsc_service_start(*ser, true);
  }

  return id;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
typedef struct {
  int socket;
  tsc_chan_t comm;
  tsc_service_t service;
} tsc_snd_param_t;

// the routine of sender task.
//  at first, it sends my service id to remote service,
//  and then, it runs the message sending loop..
static int __tsc_service_sender(tsc_snd_param_t *param) {
  tsc_service_id_t id = param->service->id;

  // FIXME: check the return value!!
  tsc_net_write(param->socket, &id, sizeof(id));

  for (;;) {
    tsc_message_t message;
    if (tsc_chan_recv(param->comm, &message) == CHAN_CLOSED) break;

    tsc_net_write(param->socket, &message->len, sizeof(int));
    if (message->len == 0) {
      close(param->socket);  // stop the receiver !!
      __tsc_service_record_delete(message->src);
      tsc_message_dealloc(message);
      break;
    }

    if (message->len > 0) {
      tsc_net_write(param->socket, message, message->len + sizeof(*message));
    }
    tsc_message_dealloc(message);
  }

  // dealloc the resources
  tsc_refcnt_put(param->comm);
  close(param->socket);

  return 0;
}

typedef struct {
  int socket;
  tsc_chan_t comm;
  tsc_chan_t sync;
} tsc_recv_param_t;

// the routine of receiver task.
//  at first, it waits the remote end's service id,
//  then, it establishes the record entry for this id,
//  last, it runs the message receiving loop..
static int __tsc_service_receiver(tsc_recv_param_t *param) {
  tsc_service_id_t id;
  tsc_message_t message;
  int len;

  // FIXME: check the return value!!
  tsc_net_read(param->socket, &id, sizeof(id));

  // send the return number back to the connector ..
  tsc_chan_send(param->sync, &id);
  tsc_refcnt_put(param->sync);

  // get the message from the remote end ..
  while (tsc_net_read(param->socket, &len, sizeof(int)) == sizeof(int)) {
    // TODO : what about the usage for the messages with negative length ??
    if (len < 0) break;

    // a zero length message means to disconnect to the target!!
    if (len == 0) {
      tsc_chan_t comm = __tsc_service_record_get(id);
      if (comm != NULL) {
          tsc_chan_close(comm);  // stop the sender!!
          __tsc_service_record_delete(id);
      }
      break;
    }

    message = tsc_message_alloc(0, len);
    tsc_net_read(param->socket, message, len + sizeof(*message));

    // push this message into the service's message queue ..
    tsc_chan_sendp(param->comm, message);
  }

  // dealloc the resources ..
  tsc_refcnt_put(param->comm);
  TSC_DEALLOC(param);

  return 0;
}

int tsc_service_lookup(const char *name, tsc_service_id_t *id) {
    // TODO : get the service id by name (a string)..
    //   maybe query a "name service" to find the mapping.
    return -1;
}

int tsc_service_connect(tsc_service_t self, tsc_service_id_t target) {
    // check if the service with specific id is already recorded
    if (__tsc_service_record_get(target) != NULL) return -1;

    uint32_t ip = TSC_SERVICE_ID2IP(target);
    int port = TSC_SERVICE_ID2PORT(target);

    // FIXME: check if the service to be connected is a local one..
    assert(ip != TSC_SERVICE_LOCAL_IP);

    // try to connect to the remote server ..
    char server[16];
    snprintf(server, 16, "%d.%d.%d.%d", ((char *)(&ip))[0], ((char *)(&ip))[1],
             ((char *)(&ip))[2], ((char *)(&ip))[3]);

    int socket = tsc_net_dial(true, server, port);
    if (socket < 0) return -1;

    // start the sender ..
    tsc_chan_t comm = tsc_chan_allocate(sizeof(tsc_message_t), 10);
    tsc_snd_param_t *sp = TSC_ALLOC(sizeof(tsc_snd_param_t));
    sp->comm = tsc_refcnt_get(comm);
    sp->service = self;
    sp->socket = socket;

    tsc_coroutine_spawn(__tsc_service_sender, sp, "sender");

    // start the receiver ..
    tsc_chan_t sync = tsc_refcnt_get(tsc_chan_allocate(sizeof(tsc_service_id_t), 0));
    tsc_recv_param_t *rp = TSC_ALLOC(sizeof(tsc_recv_param_t));
    rp->comm = tsc_refcnt_get(self->inbound);
    rp->sync = tsc_refcnt_get(sync);
    rp->socket = socket;

    tsc_coroutine_spawn(__tsc_service_receiver, rp, "receiver");

    // waiting for the service id
    tsc_service_id_t id;
    tsc_chan_recv(sync, &id);

    if (id != target) {
        // TODO !! stop the sender / receiver !!
        close(socket);
        tsc_chan_close(comm);
        goto __exit_connect;
    }

    __tsc_service_record_insert(target, tsc_refcnt_get(comm));

__exit_connect:
    tsc_refcnt_put(comm);
    tsc_refcnt_put(sync);

    return 0;
}

int tsc_service_disconnect(tsc_service_t self, tsc_service_id_t target) {
    tsc_message_t message;
    tsc_chan_t comm = __tsc_service_record_get(target);
    if (comm == NULL) return -1;

    // sending the detach signal to the target service ..
    message = tsc_message_alloc(self->id, 0);
    return tsc_message_send(message, target);
}

static int tsc_service_accept(tsc_service_t service) {
    // start the server ..
    int socket = tsc_net_announce(true, service->host, service->port);

    // waiting for the clients ..
    for (;;) {
        int client = tsc_net_accept(socket, NULL, NULL);
        if (client < 0) break;  // FIXME!!

        // starting the receiver task..
        tsc_chan_t sync = tsc_refcnt_get(tsc_chan_allocate(sizeof(tsc_service_id_t), 0));
        tsc_recv_param_t *rp = TSC_ALLOC(sizeof(tsc_recv_param_t));
        rp->comm = tsc_refcnt_get(service->inbound);
        rp->sync = tsc_refcnt_get(sync);
        rp->socket = client;

        tsc_coroutine_spawn(__tsc_service_receiver, rp, "receiver");

        // starting the sender task ..
        tsc_chan_t outbound = tsc_chan_allocate(sizeof(tsc_message_t), 10);
        tsc_snd_param_t *sp = TSC_ALLOC(sizeof(tsc_snd_param_t));
        sp->comm = tsc_refcnt_get(outbound);
        sp->service = service;
        sp->socket = client;

        tsc_coroutine_spawn(__tsc_service_sender, sp, "sender");

        // synchronizing ..
        tsc_service_id_t id;
        tsc_chan_recv(sync, &id);

        if (__tsc_service_record_get(id) != NULL) {
            // FIXME: stop the sender/receiver tasks..
            close(client);
            tsc_chan_close(outbound);
        } else {
            __tsc_service_record_insert(id, tsc_refcnt_get(outbound));
        }

        tsc_refcnt_put(outbound);
        tsc_refcnt_put(sync);
    }

    return 0;
}

// ------- message API -----------
tsc_message_t tsc_message_alloc(tsc_service_id_t src, int len) {
    assert(len >= 0);
    tsc_message_t msg = TSC_ALLOC(sizeof(struct tsc_message) + len);
    msg->len = len;
    msg->src = src;

    return msg;
}

void tsc_message_dealloc(tsc_message_t msg) {
    assert(msg != NULL);
    TSC_DEALLOC(msg);
}

int tsc_message_send(tsc_message_t msg, tsc_service_id_t dst) {
    tsc_chan_t comm = __tsc_service_record_get(dst);
    if (comm == NULL) return -1;
    msg->dst = dst;
    tsc_chan_sendp(comm, msg);
    return 0;
}
