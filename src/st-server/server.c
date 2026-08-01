/**
  ******************************************************************************
  * @file           : server.c
  * @brief          : Tool: st-server
  *                   Serves a locally-attached ST-LINK over TCP so that
  *                   st-flash / st-info on another machine can drive it via
  *                   --remote. The server forwards backend operations;
  *                   target-specific logic runs on the client.
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @author         : James Walmsley (jameswalmsley)
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // _WIN32

#include <stlink.h>

#include <logging.h>
#include <remote.h>

static volatile sig_atomic_t stop_requested = 0;

static void request_stop(int32_t signum) {
    (void)signum;
    stop_requested = 1;
}

static void setup_signal_handlers(void) {
#if defined(_WIN32)
    signal(SIGINT, request_stop);
    signal(SIGTERM, request_stop);
#else
    signal(SIGPIPE, SIG_IGN); // a client disconnecting mid-write must not kill us

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = request_stop;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
#endif
}

static void usage(void) {
    puts("st-server - serve a local ST-LINK over TCP for st-flash/st-info --remote");
    puts("Usage: st-server [--bind=<address>[:port]] [--port=<port>] [--serial=<serial>] [--freq=<kHz>] [--verbose[=n]]");
    printf("  default bind: 127.0.0.1:%d\n", STLINK_REMOTE_DEFAULT_PORT);
    puts("  use --bind=0.0.0.0 or --bind=:4500 to listen on all IPv4 interfaces");
}

static int32_t parse_bind(const char *arg, char *bind_addr, size_t bind_addr_size, int32_t *port) {
    const char *colon = strrchr(arg, ':');

    if (colon) {
        size_t addr_len = (size_t)(colon - arg);
        if (addr_len >= bind_addr_size) {
            return (-1);
        }
        if (addr_len == 0) {
            strcpy(bind_addr, "0.0.0.0");
        } else {
            memcpy(bind_addr, arg, addr_len);
            bind_addr[addr_len] = '\0';
        }

        if (colon[1] != '\0') {
            char *end = NULL;
            long parsed_port = strtol(colon + 1, &end, 0);
            if (*end != '\0' || parsed_port <= 0 || parsed_port > UINT16_MAX) {
                return (-1);
            }
            *port = (int32_t)parsed_port;
        }
    } else {
        if (strlen(arg) >= bind_addr_size) {
            return (-1);
        }
        strcpy(bind_addr, arg);
    }

    return (0);
}

int32_t main(int32_t argc, char **argv) {
    int32_t port = STLINK_REMOTE_DEFAULT_PORT;
    int32_t freq = 0;
    int32_t verbose = UINFO;
    char *serial = NULL;
    char bind_addr[64] = "127.0.0.1";

    static struct option long_options[] = {
        {"bind",    required_argument, NULL, 'b'},
        {"port",    required_argument, NULL, 'p'},
        {"serial",  required_argument, NULL, 's'},
        {"freq",    required_argument, NULL, 'f'},
        {"verbose", optional_argument, NULL, 'v'},
        {"help",    no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int32_t c, option_index = 0;
    while ((c = getopt_long(argc, argv, "b:p:s:f:v::h", long_options, &option_index)) != -1) {
        switch (c) {
        case 'b':
            if (parse_bind(optarg, bind_addr, sizeof(bind_addr), &port)) {
                ELOG("Invalid bind address: %s\n", optarg);
                usage();
                return (1);
            }
            break;
        case 'p': {
            char *end = NULL;
            long parsed = strtol(optarg, &end, 0);
            if (*end != '\0' || parsed <= 0 || parsed > UINT16_MAX) {
                ELOG("Invalid port: %s\n", optarg);
                usage();
                return (1);
            }
            port = (int32_t)parsed;
            break;
        }
        case 's': serial = optarg; break;
        case 'f': freq = (int32_t)strtol(optarg, NULL, 0) * 1000; break;
        case 'v': verbose = optarg ? atoi(optarg) : UDEBUG; break;
        case 'h': usage(); return (0);
        default:  usage(); return (1);
        }
    }

    ugly_init(verbose);
    setup_signal_handlers();

    // Open the one probe up front, so a running server means it was claimed.
    // open_usb also runs chip discovery, which logs a "Can not connect" error
    // for parts that need the client's connect mode (e.g. STM32H5 on AP1); the
    // server never uses the chip id, so quiet the open unless -v was given.
    int32_t open_verbose = (verbose > UINFO) ? verbose : (UERROR - 1);
    stlink_t *sl = stlink_open_usb(open_verbose, CONNECT_HOT_PLUG, serial, freq);
    ugly_init(verbose); // restore the requested level for the serve loop

    if (sl == NULL) {
        ELOG("Failed to open an ST-LINK device\n");
        return (1);
    }
    ILOG("Serving ST-LINK (chip discovery is done by the client)\n");

    int32_t srv = (int32_t)socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        ELOG("socket() failed\n");
        stlink_close(sl);
        return (1);
    }
    int32_t one = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, bind_addr, &addr.sin_addr) != 1) {
        ELOG("Invalid bind address: %s\n", bind_addr);
        close(srv);
        stlink_close(sl);
        return (1);
    }
    addr.sin_port = htons((uint16_t)port);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(srv, 1) < 0) {
        ELOG("Failed to bind/listen on %s:%d\n", bind_addr, port);
        close(srv);
        stlink_close(sl);
        return (1);
    }

    ILOG("Listening on %s:%d\n", bind_addr, port);

    while (!stop_requested) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int32_t fd = (int32_t)accept(srv, (struct sockaddr *)&cli, &clilen);
        if (fd < 0) {
            if (errno == EINTR && stop_requested) { break; }
            continue;
        }

        ILOG("Client connected from %s\n", inet_ntoa(cli.sin_addr));
        stlink_remote_serve(sl, fd);
        close(fd);
        ILOG("Client disconnected\n");
    }

    ILOG("Shutting down\n");
    close(srv);
    stlink_close(sl);
    return (0);
}
