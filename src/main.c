#include <libubus.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "datastorage.h"
#include "networksocket.h"
#include "ubus.h"
#include "dawn_uci.h"
#include "dawn_iwinfo.h"
#include "tcpsocket.h"
#include "crypto.h"

void daemon_shutdown();

void signal_handler(int sig);

struct sigaction signal_action;

void daemon_shutdown() {
    // kill threads
    close_socket();
    uci_clear();
    uloop_cancelled = true;

    destroy_mutex();
}

void signal_handler(int sig) {
    switch (sig) {
        case SIGHUP:
            daemon_shutdown();
            break;
        case SIGINT:
            daemon_shutdown();
            break;
        case SIGTERM:
            daemon_shutdown();
            exit(EXIT_SUCCESS);
        default:
            daemon_shutdown();
            break;
    }
}

int main(int argc, char **argv) {

    const char *ubus_socket = NULL;

    argc -= optind;
    argv += optind;

    // connect signals
    signal_action.sa_handler = signal_handler;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = 0;
    sigaction(SIGHUP, &signal_action, NULL);
    sigaction(SIGTERM, &signal_action, NULL);
    sigaction(SIGINT, &signal_action, NULL);

    uci_init();
    // TODO: Why the extra loacl struct to retuen into?
    struct network_config_s net_config = uci_get_dawn_network();
    network_config = net_config;

    // init crypto
    gcrypt_init();
    gcrypt_set_key_and_iv(net_config.shared_key, net_config.iv);

    // TODO: Why the extra loacl struct to retuen into?
    struct time_config_s time_config = uci_get_time_config();
    timeout_config = time_config; // TODO: Refactor...

    uci_get_dawn_hostapd_dir();
    uci_get_dawn_sort_order();

    init_mutex();

    switch (net_config.network_option) {
        case 0:
            init_socket_runopts(net_config.broadcast_ip, net_config.broadcast_port, 0);
            break;
        case 1:
            init_socket_runopts(net_config.broadcast_ip, net_config.broadcast_port, 1);
            break;
        default:
            break;
    }

    insert_macs_from_file();
    dawn_init_ubus(ubus_socket, hostapd_dir_glob);

    return 0;
}