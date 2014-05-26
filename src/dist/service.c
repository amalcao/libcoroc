#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service.h"
#include "../time.h"

struct tsc_client_param {
  int socket;
  tsc_chan_t comm;
};


static int __tsc_dispatch_message(tsc_service_t *service) {
    struct tsc_message message;
    int sig = -1;

    for (;;) {
        tsc_chan_recv(service->outbound, &message);
        if (message.serial.type == TSC_SERVICE_LOCAL)
            continue; // ERROR!!

        int socket = message.serial.socket;
        if (message.len <= 0) {
            // if the client close the channel, we just send a negtive sized message
            tsc_net_write(socket, &sig, sizeof(int));
            continue;
        } else {
            // firstly, sending the length of this message
            assert(tsc_net_write(socket, &(message.len), sizeof(int)) ==
                   sizeof(uint32_t));
            // and then, sending the content of this message
            assert(tsc_net_write(socket, message.buf, message.len) ==
                   message.len);

            // check if we should delete the buffer here!!
            if (message.auto_delete) 
              TSC_DEALLOC(message.buf);
        }
    }

    return 0;
}

static int __tsc_pump_message(tsc_service_t *service) {
    struct tsc_message message;
    int info, error;

    tsc_chan_t _chan = NULL;
    tsc_timer_t timer = tsc_timer_allocate(0, 0);
    tsc_chan_set_t set = tsc_chan_set_allocate(3);

    // add the channels to the set
    tsc_chan_t comm = tsc_refcnt_get(service->inbound);
    tsc_chan_t quit = tsc_refcnt_get(service->quit);

    tsc_chan_set_recv(set, inbound, &message);
    tsc_chan_set_recv(set, quit, &info);
    tsc_chan_set_recv(set, (tsc_chan_t)timer, 0);

    // start the select loop ..
    for (;;) {
        // set the timer ..
        if (service->timeout != TSC_SERVICE_INFINITY)
          tsc_timer_after(timer, service->timeout);

        tsc_chan_set_select(set, &_chan);
        if (comm == _chan) {
            if (message.len < 0) {
                TSC_ATOMIC_DEC(service->clients);
                error = -1;  // client disconnect ..
            } else {
                error = service->routine(service, &message);
            }
            if (error && service->error_callback)
              service->error_callback(service, &error);
            // the message should be freed here!!
            if (message.serial >= 0 && message.len > 0) TSC_DEALLOC(message.buf);
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

    if (service->stype & TSC_SERVICE_REMOTE_TCP) {
        // TODO: notify the service side to quit !!
        close(service->socket);
    }

    return 0;
}

static int __tsc_replay_message(struct tsc_message *message) {

    if (message->serial.type == TSC_SERVICE_REMOTE_TCP) {
        assert(tsc_net_write(message->serial.socket, &(message->len),
                             sizeof(message->len)) == sizeof(message->len));

        if (message->len > 0) {
            assert(tsc_net_write(message->serial.socket, message->buf, message->len) ==
                   message->len);
            TSC_DEALLOC(message->buf);
        }
        TSC_DEALLOC(message);
    }

    return 0;
}

static int __tsc_receive_message(struct tsc_client_param *param) {
    struct tsc_message message;
    message.serial = param->socket;

    while (tsc_net_read(param->socket, &(message.len), sizeof(int)) > 0) {
        if (message.len == 0) continue;

        // a negtive len means the remote client decide to disconnect
        // TODO
        if (message.len > 0) {
            // alloc new message buffer ..
            message.buf = TSC_ALLOC(message.len);
            // read the result of the package content ..
            assert(tsc_net_read(param->socket, message.buf, message.len) ==
                   message.len);
        }

        // dispatch the message to the `pump' task ..
        if (tsc_chan_send(param->comm, &message) == CHAN_CLOSED) break;
    }

    // dealloc the resources ..
    tsc_refcnt_put(param->comm);
    close(param->socket);
    TSC_DEALLOC(param);  // !!

    return 0;
}

static int __tsc_server_loop(tsc_service_t *service) {
    // start the server ..
    int socket = tsc_net_announce(true, "*", service->port);
    assert(socket >= 0);

    // loop, waiting for new clients ..
    for (;;) {
        int client = tsc_net_accept(socket, NULL, NULL);
        if (client < 0) break;

        // start a new task to monitor the incomming data from this client.
        struct tsc_client_param *recv_param = TSC_ALLOC(sizeof(*recv_param));

        recv_param->socket = client;
        recv_param->comm = tsc_refcnt_get(service->inbound);

        TSC_ATOMIC_INC(service->clients);
        tsc_coroutine_spawn(__tsc_receive_message, recv_param, "receiver");
    }

    // TODO: how to notice the server to exit ??
    return 0;
}

static int __tsc_send_message(struct tsc_client_param *param) {
    struct tsc_message message;
    int sig = -1;

    for (;;) {
        // get the message element from the send operations ..
        tsc_chan_recv(param->comm, &message);
        if (message.len <= 0) {
            // if the client close the channel, we just send a negtive sized message
            // ..
            tsc_net_write(param->socket, &sig, sizeof(int));
            break;
        } else {
            // firstly, sending the length of this message
            assert(tsc_net_write(param->socket, &(message.len), sizeof(int)) ==
                   sizeof(uint32_t));
            // and then, sending the content of this message
            assert(tsc_net_write(param->socket, message.buf, message.len) ==
                   message.len);

            // check if we should delete the buffer here!!
            if (message.auto_delete) 
              TSC_DEALLOC(message.buf);
        }
    }

    // release the resources!!
    close(param->socket);
    tsc_chan_close(param->comm);
    tsc_refcnt_put(param->comm);
    TSC_DEALLOC(param);

    return 0;
}

int tsc_service_register(tsc_service_t *service, int uniqid) {
    assert(service && uniqid >= 0);
    service->suniqid = uniqid;
    if (service->stype & TSC_SERVICE_REMOTE_TCP)
      service->suniqid |= (service->port << 16);
    return 0;
}

int tsc_service_start(tsc_service_t *service, bool backend) {
    assert(service != NULL);

    // create the internal channel for messages ..
    service->comm =
      tsc_refcnt_get(tsc_chan_allocate(sizeof(struct tsc_message), 0));
    // create the channel for quiting ..
    service->quit = tsc_refcnt_get(tsc_chan_allocate(sizeof(int), 1));

    // create a coroutine to establish the TCP server..
    if (service->stype & TSC_SERVICE_REMOTE_TCP)
      tsc_coroutine_spawn(__tsc_server_loop, (void *)service, "tcp");

    // create a coroutine to service the message channel..
    tsc_coroutine_spawn(__tsc_dispatch_message, (void *)service, "dispatch");
    if (backend) {
        tsc_coroutine_spawn(__tsc_pump_message, (void *)service, "pump");
    } else {
        __tsc_pump_message(service);
    }
    return 0;
}

int tsc_service_quit(tsc_service_t *service, int info) {
    assert(service && service->quit);
    tsc_chan_send(service->quit, &info);
    tsc_refcnt_put(service->quit);
    return 0;
}

int tsc_service_reload(tsc_service_t *service) {
    tsc_service_quit(service, 0);
    tsc_service_start(service, true);  // TODO
    return 0;
}

// FIXME: using a hash map to enhance the searching ..
tsc_conn_t tsc_service_connect_by_name(const char *name, tsc_chan_t inbound) {
    tsc_service_t **iter = TSC_SERVICE_BEGIN();
    for (; iter < TSC_SERVICE_END(); ++iter) {
        if (!strcmp((*iter)->sname, name)) {
            TSC_ATOMIC_INC((*iter)->clients);

            // alloc connection element ..
            tsc_conn_t conn = TSC_ALLOC(sizeof(struct tsc_conn));
            conn->serial.type = TSC_SERVICE_LOCAL;
            conn->serial.chan = inbound;
            conn->outbound = tsc_refcnt_get((*iter)->comm);
            conn->inbound = tsc_refcnt_get(inbound);
            return conn;
        }
    }
    return NULL;
}

tsc_conn_t tsc_service_connect_by_addr(uint32_t addr, int port, tsc_chan_t inbound) {
    char server[16];
    uint8_t *ip = (uint8_t *)(&addr);
    snprintf(server, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return tsc_service_connect_by_host(server, port, inbound);
}

tsc_conn_t tsc_service_connect_by_host(const char *host, int port, tsc_chan_t inbound) {
    // FIXME: we don't support the multi-param passing to a new spawned task now!
    struct tsc_client_param *snd_param, *recv_param;
    int socket;
    tsc_conn_t conn = NULL;

    socket = tsc_net_dial(true, host, port);
    if (socket >= 0) {
        conn = TSC_ALLOC(sizeof(struct tsc_conn));
        conn->serial.type = TSC_SERVICE_REMOTE_TCP;
        conn->serial.socket = socket;
        conn->outbound = tsc_refcnt_get(tsc_chan_allocate(sizeof(struct tsc_message), 5));
        conn->inbound = tsc_refcnt_get(inbound);

        // start the coroutine to send the outbound messages
        snd_param->socket = socket;
        snd_param->comm = tsc_refcnt_get(conn->outbound);

        tsc_coroutine_spawn(__tsc_send_message, snd_param, "sender");

        // start the coroutine to recv the inbound messages
        if (inbound != NULL) {
            recv_param->socket = socket;
            recv_param->comm = tsc_refcnt_get(inbound);

            tsc_coroutine_spawn(__tsc_receive_message, recv_param, "receiver");
        }
    }
    return conn;
}

int tsc_service_disconnect(tsc_conn_t conn) {
    tsc_message_t message;
    tsc_message_init(&message, conn, 0);
    tsc_chan_send(conn->outbound, &message);

    tsc_refcnt_put(conn->outbound);
    tsc_refcnt_put(conn->inbound);

    return 0;
}

// FIXME: we need to just define one service to create the section
TSC_SERVICE_DECLARE_LOCAL(__dummy_service, "dummy", TSC_SERVICE_INFINITY, false,
                          NULL, NULL, NULL, NULL);

// start all the services ..
int tsc_init_all_services(void) {
    int id = 0;
    tsc_service_t **ser = TSC_SERVICE_BEGIN();

    for (; ser < TSC_SERVICE_END(); ++ser, ++id) {
        tsc_service_register(*ser, id);
        if ((*ser)->init_callback) (*ser)->init_callback(*ser, &id);
        if ((*ser)->auto_start) tsc_service_start(*ser, true);
    }

    return id;
}
