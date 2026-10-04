/**
  ******************************************************************************
  * @file           : remote.c
  * @brief          : Remote backend and server dispatch
  * @copyright      : Copyright (c) 2026 stlink-org. All rights reserved.
  * @author         : James Walmsley (jameswalmsley)
  * @date           : 2026-07-27
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * This file is licensed under the BSD 3-Clause License.
  * See the LICENSE file in the project root for full license information.
  ******************************************************************************
  */

/*
 * Wire format (all integers little-endian via read_uint32/write_uint32):
 *
 *   request: [op u32][ap u32][arg0 u32][arg1 u32][payload_len u32][payload...]
 *   reply:   [status u32][ret i32][payload_len u32][payload...]
 *   handshake (server -> client on connect):
 *            [magic][protocol_version][capabilities][stlink_v][jtag_v][swim_v]
 *            [st_vid][stlink_pid][jtag_api][flags][max_trace_freq]
 *
 * Capabilities is reserved for future optional protocol features. Version 1
 * servers send 0 and clients must ignore unknown future capability bits.
 *
 * The client carries sl->ap in every request because the USB command bytes
 * are built on the server, but the access port is selected by the client's
 * probe; the server applies the supplied ap before each operation.
 */

#if defined(_WIN32)
#include <win32_socket.h>
#endif

#include "remote.h"


#define REQ_HDR_LEN 20
#define REP_HDR_LEN 12
#define REMOTE_CONNECT_TIMEOUT_SEC 5
#define HANDSHAKE_LEN (44 + STLINK_REMOTE_SERIAL_WIRE_LEN)
#define REG_CORE_COUNT 16
#define REG_FLOAT_COUNT 32
#define REG_SCALAR_COUNT 10
#define REG_WIRE_LEN ((REG_CORE_COUNT + REG_FLOAT_COUNT + REG_SCALAR_COUNT) * 4)

struct stlink_remote {
    int32_t fd;
    bool dead; // set when a TCP send/recv breaks the connection
};

static void reg_to_wire(uint8_t *buf, const struct stlink_reg *regp) {
    uint32_t off = 0;

    for (uint32_t i = 0; i < REG_CORE_COUNT; i++, off += 4) { write_uint32(&buf[off], regp->r[i]); }
    for (uint32_t i = 0; i < REG_FLOAT_COUNT; i++, off += 4) { write_uint32(&buf[off], regp->s[i]); }

    write_uint32(&buf[off], regp->xpsr);       off += 4;
    write_uint32(&buf[off], regp->main_sp);    off += 4;
    write_uint32(&buf[off], regp->process_sp); off += 4;
    write_uint32(&buf[off], regp->rw);         off += 4;
    write_uint32(&buf[off], regp->rw2);        off += 4;
    write_uint32(&buf[off], regp->control);    off += 4;
    write_uint32(&buf[off], regp->faultmask);  off += 4;
    write_uint32(&buf[off], regp->basepri);    off += 4;
    write_uint32(&buf[off], regp->primask);    off += 4;
    write_uint32(&buf[off], regp->fpscr);
}

static void reg_from_wire(struct stlink_reg *regp, const uint8_t *buf) {
    uint32_t off = 0;

    for (uint32_t i = 0; i < REG_CORE_COUNT; i++, off += 4) { regp->r[i] = read_uint32(buf, off); }
    for (uint32_t i = 0; i < REG_FLOAT_COUNT; i++, off += 4) { regp->s[i] = read_uint32(buf, off); }

    regp->xpsr       = read_uint32(buf, off); off += 4;
    regp->main_sp    = read_uint32(buf, off); off += 4;
    regp->process_sp = read_uint32(buf, off); off += 4;
    regp->rw         = read_uint32(buf, off); off += 4;
    regp->rw2        = read_uint32(buf, off); off += 4;
    regp->control    = (uint8_t)read_uint32(buf, off); off += 4;
    regp->faultmask  = (uint8_t)read_uint32(buf, off); off += 4;
    regp->basepri    = (uint8_t)read_uint32(buf, off); off += 4;
    regp->primask    = (uint8_t)read_uint32(buf, off); off += 4;
    regp->fpscr      = read_uint32(buf, off);
}

/* === transport helpers === */

static int32_t send_all(int32_t fd, const void *buf, uint32_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    while (len) {
        ssize_t n = send(fd, (const void *)p, len, 0);
        if (n < 0) {
            ELOG("remote: TCP send failed: %s\n", strerror(errno));
            return (-1);
        }
        if (n == 0) {
            ELOG("remote: TCP send returned 0\n");
            return (-1);
        }
        p += n;
        len -= (uint32_t)n;
    }
    return (0);
}

static int32_t recv_all(int32_t fd, void *buf, uint32_t len) {
    uint8_t *p = (uint8_t *)buf;
    while (len) {
        ssize_t n = recv(fd, (void *)p, len, 0);
        if (n < 0) {
            ELOG("remote: TCP receive failed: %s\n", strerror(errno));
            return (-1);
        }
        if (n == 0) {
            DLOG("remote: peer disconnected\n");
            return (-1);
        }
        p += n;
        len -= (uint32_t)n;
    }
    return (0);
}

// Disable Nagle; the request/reply pattern stalls on delayed-ACK otherwise.
static void set_tcp_nodelay(int32_t fd) {
    int32_t one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&one, sizeof(one));
}

// connect() with a timeout so an unreachable host fails fast instead of
// blocking on the OS default. POSIX only; Windows uses a blocking connect.
static int32_t remote_connect_timeout(int32_t fd, const struct sockaddr *addr,
                                      socklen_t addrlen, int32_t timeout_sec) {
#if defined(_WIN32)
    (void)timeout_sec;
    return (connect(fd, addr, addrlen) < 0) ? (-1) : (0);
#else
    int32_t flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) { return (-1); }

    int32_t rc = connect(fd, addr, addrlen);
    if (rc < 0 && errno == EINPROGRESS) {
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(fd, &wset);
        struct timeval tv = { timeout_sec, 0 };
        rc = select(fd + 1, NULL, &wset, NULL, &tv);
        if (rc == 0) {
            errno = ETIMEDOUT;
            rc = -1;
        } else if (rc > 0) {
            int32_t soerr = 0;
            socklen_t len = sizeof(soerr);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len) < 0 || soerr != 0) {
                if (soerr) { errno = soerr; }
                rc = -1;
            } else {
                rc = 0;
            }
        }
    }

    fcntl(fd, F_SETFL, flags); // restore blocking mode for the rest of the session
    return (rc < 0) ? (-1) : (0);
#endif
}

static int32_t send_reply(int32_t fd, uint32_t status, int32_t ret, const uint8_t *payload, uint32_t payload_len) {
    uint8_t rhdr[REP_HDR_LEN];
    write_uint32(&rhdr[0], status);
    write_uint32(&rhdr[4], (uint32_t)ret);
    write_uint32(&rhdr[8], payload_len);
    if (send_all(fd, rhdr, REP_HDR_LEN)) { return (-1); }
    if (payload_len && send_all(fd, payload, payload_len)) { return (-1); }
    return (0);
}

static int32_t send_protocol_error(int32_t fd) {
    return send_reply(fd, REMOTE_REPLY_PROTOCOL_ERROR, -1, NULL, 0);
}

/*
 * Client-side remote procedure call: send one request, read the reply.
 * Returns the backend return code (the reply's ret field), or -1 on a
 * transport error. Read payloads land in rpayload (capped at rmax).
 */
static int32_t remote_rpc(stlink_t *sl, uint8_t op, uint32_t a0, uint32_t a1,
                          const uint8_t *spayload, uint32_t splen,
                          uint8_t *rpayload, uint32_t rmax, uint32_t *rplen) {
    struct stlink_remote *rl = (struct stlink_remote *)sl->backend_data;
    uint8_t hdr[REQ_HDR_LEN];

    write_uint32(&hdr[0], op);
    write_uint32(&hdr[4], sl->ap);
    write_uint32(&hdr[8], a0);
    write_uint32(&hdr[12], a1);
    write_uint32(&hdr[16], splen);

    if (send_all(rl->fd, hdr, REQ_HDR_LEN)) { rl->dead = true; return (-1); }
    if (splen && send_all(rl->fd, spayload, splen)) { rl->dead = true; return (-1); }

    uint8_t rhdr[REP_HDR_LEN];
    if (recv_all(rl->fd, rhdr, REP_HDR_LEN)) { rl->dead = true; return (-1); }
    uint32_t status = read_uint32(rhdr, 0);
    int32_t ret = (int32_t)read_uint32(rhdr, 4);
    uint32_t plen = read_uint32(rhdr, 8);
    if (plen > rmax) {
        ELOG("remote: reply payload too large (%u > %u)\n", plen, rmax);
        rl->dead = true; // oversized reply leaves the stream desynced
        return (-1);
    }
    if (plen && recv_all(rl->fd, rpayload, plen)) { rl->dead = true; return (-1); }
    if (rplen) { *rplen = plen; }
    if (status != REMOTE_REPLY_OK) {
        ELOG("remote: server reported protocol error\n");
        return (-1);
    }
    return (ret);
}

/* === client backend === */

static void rb_close(stlink_t *sl) {
    if (sl && sl->backend_data) {
        struct stlink_remote *rl = (struct stlink_remote *)sl->backend_data;
        remote_rpc(sl, RPC_CLOSE, 0, 0, NULL, 0, NULL, 0, NULL);
        if (rl->fd >= 0) { close(rl->fd); }
        free(rl);
        sl->backend_data = NULL;
    }
}

static int32_t rb_exit_debug_mode(stlink_t *sl) { return remote_rpc(sl, RPC_EXIT_DEBUG, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_enter_swd_mode(stlink_t *sl)  { return remote_rpc(sl, RPC_ENTER_SWD, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_enter_jtag_mode(stlink_t *sl) { return remote_rpc(sl, RPC_ENTER_JTAG, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_exit_dfu_mode(stlink_t *sl)   { return remote_rpc(sl, RPC_EXIT_DFU, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_reset(stlink_t *sl)           { return remote_rpc(sl, RPC_RESET, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_jtag_reset(stlink_t *sl, int32_t value) { return remote_rpc(sl, RPC_JTAG_RESET, (uint32_t)value, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_run(stlink_t *sl, enum run_type type)   { return remote_rpc(sl, RPC_RUN, (uint32_t)type, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_step(stlink_t *sl)            { return remote_rpc(sl, RPC_STEP, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_current_mode(stlink_t *sl)    { return remote_rpc(sl, RPC_CURRENT_MODE, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_force_debug(stlink_t *sl)     { return remote_rpc(sl, RPC_FORCE_DEBUG, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_target_voltage(stlink_t *sl)  { return remote_rpc(sl, RPC_TARGET_VOLTAGE, 0, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_set_swdclk(stlink_t *sl, int32_t freq) { return remote_rpc(sl, RPC_SET_SWDCLK, (uint32_t)freq, 0, NULL, 0, NULL, 0, NULL); }
static int32_t rb_init_ap(stlink_t *sl, uint8_t ap)      { return remote_rpc(sl, RPC_INIT_AP, ap, 0, NULL, 0, NULL, 0, NULL); }

static int32_t rb_core_id(stlink_t *sl) {
    uint8_t p[4]; uint32_t pl;
    int32_t ret = remote_rpc(sl, RPC_CORE_ID, 0, 0, NULL, 0, p, sizeof(p), &pl);
    if (ret == 0 && pl == 4) { sl->core_id = read_uint32(p, 0); }
    return (ret);
}

static int32_t rb_status(stlink_t *sl) {
    uint8_t p[4]; uint32_t pl;
    int32_t ret = remote_rpc(sl, RPC_STATUS_REMOTE, 0, 0, NULL, 0, p, sizeof(p), &pl);
    if (ret == 0 && pl == 4) { sl->core_stat = (enum target_state)read_uint32(p, 0); }
    return (ret);
}

static int32_t rb_version(stlink_t *sl) {
    (void)sl; // version is delivered by the connect handshake
    return (0);
}

static int32_t rb_read_debug32(stlink_t *sl, uint32_t addr, uint32_t *data) {
    uint8_t p[4]; uint32_t pl;
    int32_t ret = remote_rpc(sl, RPC_READ_DEBUG32, addr, 0, NULL, 0, p, sizeof(p), &pl);
    if (ret == 0 && pl == 4 && data) { *data = read_uint32(p, 0); }
    return (ret);
}

static int32_t rb_write_debug32(stlink_t *sl, uint32_t addr, uint32_t data) {
    return remote_rpc(sl, RPC_WRITE_DEBUG32, addr, data, NULL, 0, NULL, 0, NULL);
}

static int32_t rb_read_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    uint32_t pl;
    int32_t ret = remote_rpc(sl, RPC_READ_MEM32, addr, len, NULL, 0,
                             (uint8_t *)sl->q_buf, Q_BUF_LEN, &pl);
    if (ret == 0) {
        if (pl != len) {
            ELOG("remote: READ_MEM32 returned %u bytes, expected %u\n", pl, len);
            return (-1);
        }
        sl->q_len = (int32_t)pl;
    }
    return (ret);
}

static int32_t rb_write_mem32(stlink_t *sl, uint32_t addr, uint16_t len) {
    return remote_rpc(sl, RPC_WRITE_MEM32, addr, len, (uint8_t *)sl->q_buf, len, NULL, 0, NULL);
}

static int32_t rb_write_mem8(stlink_t *sl, uint32_t addr, uint16_t len) {
    return remote_rpc(sl, RPC_WRITE_MEM8, addr, len, (uint8_t *)sl->q_buf, len, NULL, 0, NULL);
}

static int32_t rb_read_reg_payload(stlink_t *sl, uint8_t op, uint32_t arg, struct stlink_reg *regp) {
    uint8_t p[REG_WIRE_LEN];
    uint32_t pl;
    int32_t ret = remote_rpc(sl, op, arg, 0, NULL, 0, p, sizeof(p), &pl);
    if (ret == 0 && pl == REG_WIRE_LEN) { reg_from_wire(regp, p); }
    else if (ret == 0) { ret = -1; }
    return (ret);
}

static int32_t rb_read_all_regs(stlink_t *sl, struct stlink_reg *regp) {
    return rb_read_reg_payload(sl, RPC_READ_ALL_REGS, 0, regp);
}

static int32_t rb_read_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp) {
    return rb_read_reg_payload(sl, RPC_READ_REG, (uint32_t)r_idx, regp);
}

static int32_t rb_read_all_unsupported_regs(stlink_t *sl, struct stlink_reg *regp) {
    return rb_read_reg_payload(sl, RPC_READ_ALL_UNSUPPORTED_REGS, 0, regp);
}

static int32_t rb_read_unsupported_reg(stlink_t *sl, int32_t r_idx, struct stlink_reg *regp) {
    return rb_read_reg_payload(sl, RPC_READ_UNSUPPORTED_REG, (uint32_t)r_idx, regp);
}

static int32_t rb_write_unsupported_reg(stlink_t *sl, uint32_t value, int32_t r_idx, struct stlink_reg *regp) {
    uint8_t p[REG_WIRE_LEN];
    uint8_t rp[REG_WIRE_LEN];
    uint32_t pl;
    reg_to_wire(p, regp);
    int32_t ret = remote_rpc(sl, RPC_WRITE_UNSUPPORTED_REG, value, (uint32_t)r_idx,
                             p, sizeof(p), rp, sizeof(rp), &pl);
    if (ret == 0 && pl == REG_WIRE_LEN) { reg_from_wire(regp, rp); }
    else if (ret == 0) { ret = -1; }
    return (ret);
}

static int32_t rb_write_reg(stlink_t *sl, uint32_t reg, int32_t idx) {
    return remote_rpc(sl, RPC_WRITE_REG, reg, (uint32_t)idx, NULL, 0, NULL, 0, NULL);
}

static int32_t rb_trace_enable(stlink_t *sl, uint32_t frequency) {
    return remote_rpc(sl, RPC_TRACE_ENABLE, frequency, 0, NULL, 0, NULL, 0, NULL);
}

static int32_t rb_trace_disable(stlink_t *sl) {
    return remote_rpc(sl, RPC_TRACE_DISABLE, 0, 0, NULL, 0, NULL, 0, NULL);
}

static int32_t rb_trace_read(stlink_t *sl, uint8_t *buf, uint32_t size) {
    // trace_read returns a byte count: it comes back in ret, the bytes in the payload.
    return remote_rpc(sl, RPC_TRACE_READ, size, 0, NULL, 0, buf, size, NULL);
}

static stlink_backend_t _stlink_remote_backend = {
    rb_close,
    rb_exit_debug_mode,
    rb_enter_swd_mode,
    rb_enter_jtag_mode,
    rb_exit_dfu_mode,
    rb_core_id,
    rb_reset,
    rb_jtag_reset,
    rb_run,
    rb_status,
    rb_version,
    rb_read_debug32,
    rb_read_mem32,
    rb_write_debug32,
    rb_write_mem32,
    rb_write_mem8,
    rb_read_all_regs,
    rb_read_reg,
    rb_read_all_unsupported_regs,
    rb_read_unsupported_reg,
    rb_write_unsupported_reg,
    rb_write_reg,
    rb_step,
    rb_current_mode,
    rb_force_debug,
    rb_target_voltage,
    rb_set_swdclk,
    rb_trace_enable,
    rb_trace_disable,
    rb_trace_read,
    rb_init_ap,
};

/* === client open === */

stlink_t *stlink_open_remote(int32_t verbose, const char *host, int32_t port,
                             enum connect_type conn, int32_t freq) {
    ugly_init(verbose);

    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%d", port ? port : STLINK_REMOTE_DEFAULT_PORT);

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Report connection failures to stderr, not ELOG: they must show even at a
    // log level that suppresses errors (st-info opens with verbose 0).
    if (getaddrinfo(host, portstr, &hints, &res) != 0 || res == NULL) {
        fprintf(stderr, "remote: cannot resolve host '%s'\n", host);
        return (NULL);
    }

    int32_t fd = (int32_t)socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0 || remote_connect_timeout(fd, res->ai_addr, (socklen_t)res->ai_addrlen,
                                         REMOTE_CONNECT_TIMEOUT_SEC) < 0) {
        fprintf(stderr, "remote: cannot connect to st-server at %s:%s (%s)\n",
                host, portstr, strerror(errno));
        if (fd >= 0) { close(fd); }
        freeaddrinfo(res);
        return (NULL);
    }
    freeaddrinfo(res);
    set_tcp_nodelay(fd);

    uint8_t hs[HANDSHAKE_LEN];
    if (recv_all(fd, hs, HANDSHAKE_LEN) || read_uint32(hs, 0) != STLINK_REMOTE_MAGIC) {
        fprintf(stderr, "remote: no valid handshake from %s:%s (is st-server running there?)\n",
                host, portstr);
        close(fd);
        return (NULL);
    }
    if (read_uint32(hs, 4) != STLINK_REMOTE_PROTOCOL_VERSION) {
        fprintf(stderr, "remote: server at %s:%s uses protocol version %u, client expects %u\n",
                host, portstr, read_uint32(hs, 4), STLINK_REMOTE_PROTOCOL_VERSION);
        close(fd);
        return (NULL);
    }

    stlink_t *sl = calloc(1, sizeof(stlink_t));
    struct stlink_remote *rl = calloc(1, sizeof(struct stlink_remote));
    if (!sl || !rl) { free(sl); free(rl); close(fd); return (NULL); }
    rl->fd = fd;
    sl->backend = &_stlink_remote_backend;
    sl->backend_data = rl;

    sl->version.stlink_v   = read_uint32(hs, 12);
    sl->version.jtag_v     = read_uint32(hs, 16);
    sl->version.swim_v     = read_uint32(hs, 20);
    sl->version.st_vid     = read_uint32(hs, 24);
    sl->version.stlink_pid = read_uint32(hs, 28);
    sl->version.jtag_api   = (enum stlink_jtag_api_version)read_uint32(hs, 32);
    sl->version.flags      = read_uint32(hs, 36);
    sl->max_trace_freq     = read_uint32(hs, 40);
    memcpy(sl->serial, &hs[44], STLINK_SERIAL_BUFFER_SIZE); // wire field is zero-padded
    sl->serial[STLINK_SERIAL_BUFFER_SIZE - 1] = '\0';

    // Same connect sequence as stlink_open_usb(), driven over the backend ops.
    int32_t mode = stlink_current_mode(sl);
    if (mode == STLINK_DEV_DFU_MODE) { sl->backend->exit_dfu_mode(sl); }

    if (conn == CONNECT_UNDER_RESET) {
        if (mode == STLINK_DEV_DEBUG_MODE) { sl->backend->exit_debug_mode(sl); }
        sl->backend->jtag_reset(sl, STLINK_DEBUG_APIV2_DRIVE_NRST_LOW);
    }

    sl->freq = freq;
    sl->backend->set_swdclk(sl, freq);

    stlink_target_connect(sl, conn);

    // Bail if a TCP error broke the link during the sequence above; a failed
    // target_connect (link still up) is handled downstream instead.
    if (rl->dead) {
        fprintf(stderr, "remote: lost connection to st-server during connect\n");
        close(rl->fd);
        free(rl);
        free(sl);
        return (NULL);
    }
    return (sl);
}

stlink_t *stlink_open_remote_str(int32_t verbose, const char *hostport,
                                 enum connect_type connect, int32_t freq) {
    char host[256];
    int32_t port = STLINK_REMOTE_DEFAULT_PORT;
    const char *colon = strrchr(hostport, ':');

    if (colon) {
        size_t hlen = (size_t)(colon - hostport);
        if (hlen >= sizeof(host)) { hlen = sizeof(host) - 1; }
        memcpy(host, hostport, hlen);
        host[hlen] = '\0';

        char *end = NULL;
        long parsed = strtol(colon + 1, &end, 0);
        if (*end != '\0' || parsed <= 0 || parsed > UINT16_MAX) {
            fprintf(stderr, "remote: invalid port in '%s'\n", hostport);
            return (NULL);
        }
        port = (int32_t)parsed;
    } else {
        snprintf(host, sizeof(host), "%s", hostport);
    }

    return stlink_open_remote(verbose, host, port, connect, freq);
}

/* === server dispatch === */

int32_t stlink_remote_serve(stlink_t *sl, int32_t fd) {
    set_tcp_nodelay(fd);

    uint8_t hs[HANDSHAKE_LEN];
    write_uint32(&hs[0],  STLINK_REMOTE_MAGIC);
    write_uint32(&hs[4],  STLINK_REMOTE_PROTOCOL_VERSION);
    write_uint32(&hs[8],  0); // capabilities: reserved for future protocol options
    write_uint32(&hs[12], sl->version.stlink_v);
    write_uint32(&hs[16], sl->version.jtag_v);
    write_uint32(&hs[20], sl->version.swim_v);
    write_uint32(&hs[24], sl->version.st_vid);
    write_uint32(&hs[28], sl->version.stlink_pid);
    write_uint32(&hs[32], (uint32_t)sl->version.jtag_api);
    write_uint32(&hs[36], sl->version.flags);
    write_uint32(&hs[40], sl->max_trace_freq);
    memset(&hs[44], 0, STLINK_REMOTE_SERIAL_WIRE_LEN); // zero-pad the fixed serial field
    memcpy(&hs[44], sl->serial, STLINK_SERIAL_BUFFER_SIZE);
    if (send_all(fd, hs, HANDSHAKE_LEN)) { return (-1); }

    for (;;) {
        uint8_t hdr[REQ_HDR_LEN];
        if (recv_all(fd, hdr, REQ_HDR_LEN)) { return (0); } // client disconnected

        uint8_t  op  = (uint8_t)read_uint32(hdr, 0);
        sl->ap       = (uint8_t)read_uint32(hdr, 4);
        uint32_t a0  = read_uint32(hdr, 8);
        uint32_t a1  = read_uint32(hdr, 12);
        uint32_t splen = read_uint32(hdr, 16);

        if (splen) {
            if (splen > Q_BUF_LEN) {
                ELOG("remote: request payload too large (%u > %u)\n", splen, Q_BUF_LEN);
                send_protocol_error(fd);
                return (-1);
            }
            if (recv_all(fd, sl->q_buf, splen)) { return (-1); }
        }

        int32_t ret = -1;
        uint32_t status = REMOTE_REPLY_OK;
        uint8_t scratch[4];
        struct stlink_reg regbuf;
        uint8_t regwire[REG_WIRE_LEN];
        uint8_t *rpay = NULL;
        uint32_t rplen = 0;

        switch (op) {
        case RPC_EXIT_DEBUG:  ret = sl->backend->exit_debug_mode(sl); break;
        case RPC_ENTER_SWD:   ret = sl->backend->enter_swd_mode(sl); break;
        case RPC_ENTER_JTAG:  ret = sl->backend->enter_jtag_mode(sl); break;
        case RPC_EXIT_DFU:    ret = sl->backend->exit_dfu_mode(sl); break;
        case RPC_RESET:       ret = sl->backend->reset(sl); break;
        case RPC_JTAG_RESET:  ret = sl->backend->jtag_reset(sl, (int32_t)a0); break;
        case RPC_RUN:         ret = sl->backend->run(sl, (enum run_type)a0); break;
        case RPC_STEP:        ret = sl->backend->step(sl); break;
        case RPC_FORCE_DEBUG: ret = sl->backend->force_debug(sl); break;
        case RPC_CURRENT_MODE:ret = sl->backend->current_mode(sl); break;
        case RPC_TARGET_VOLTAGE: ret = sl->backend->target_voltage(sl); break;
        case RPC_SET_SWDCLK:  ret = sl->backend->set_swdclk(sl, (int32_t)a0); break;
        case RPC_INIT_AP:     ret = sl->backend->init_ap ? sl->backend->init_ap(sl, (uint8_t)a0) : -1; break;
        case RPC_TRACE_ENABLE:  ret = sl->backend->trace_enable ? sl->backend->trace_enable(sl, a0) : -1; break;
        case RPC_TRACE_DISABLE: ret = sl->backend->trace_disable ? sl->backend->trace_disable(sl) : -1; break;
        case RPC_TRACE_READ:
            // ret = byte count read into q_buf; ship those bytes back.
            if (a0 > Q_BUF_LEN) {
                ELOG("remote: TRACE_READ size too large (%u > %u)\n", a0, Q_BUF_LEN);
                status = REMOTE_REPLY_PROTOCOL_ERROR;
            } else if (sl->backend->trace_read) {
                ret = sl->backend->trace_read(sl, sl->q_buf, a0);
                if (ret > 0) { rpay = (uint8_t *)sl->q_buf; rplen = (uint32_t)ret; }
            }
            break;
        case RPC_CORE_ID:
            ret = sl->backend->core_id(sl);
            write_uint32(scratch, sl->core_id); rpay = scratch; rplen = 4;
            break;
        case RPC_STATUS_REMOTE:
            ret = sl->backend->status(sl);
            write_uint32(scratch, (uint32_t)sl->core_stat); rpay = scratch; rplen = 4;
            break;
        case RPC_VERSION_REMOTE:
            ret = sl->backend->version(sl);
            break;
        case RPC_READ_DEBUG32: {
            uint32_t d = 0;
            ret = sl->backend->read_debug32(sl, a0, &d);
            write_uint32(scratch, d); rpay = scratch; rplen = 4;
            break;
        }
        case RPC_READ_MEM32:
            ret = sl->backend->read_mem32(sl, a0, (uint16_t)a1);
            // send the bytes read (uint16), not the raw arg: a bogus large a1
            // would otherwise make send_all() run past q_buf.
            if (ret == 0) { rpay = (uint8_t *)sl->q_buf; rplen = (uint16_t)a1; }
            break;
        case RPC_WRITE_DEBUG32: ret = sl->backend->write_debug32(sl, a0, a1); break;
        case RPC_WRITE_MEM32:
        case RPC_WRITE_MEM8:
            // payload length must match what the backend will write, else it
            // programs stale q_buf bytes.
            if (splen != (uint16_t)a1) {
                ELOG("remote: write payload %u != declared length %u\n", splen, (uint16_t)a1);
                status = REMOTE_REPLY_PROTOCOL_ERROR;
            } else if (op == RPC_WRITE_MEM32) {
                ret = sl->backend->write_mem32(sl, a0, (uint16_t)a1);
            } else {
                ret = sl->backend->write_mem8(sl, a0, (uint16_t)a1);
            }
            break;
        case RPC_READ_ALL_REGS:
            memset(&regbuf, 0, sizeof(regbuf));
            ret = sl->backend->read_all_regs(sl, &regbuf);
            if (ret == 0) {
                reg_to_wire(regwire, &regbuf);
                rpay = regwire; rplen = REG_WIRE_LEN;
            }
            break;
        case RPC_READ_REG:
            memset(&regbuf, 0, sizeof(regbuf));
            ret = sl->backend->read_reg(sl, (int32_t)a0, &regbuf);
            if (ret == 0) {
                reg_to_wire(regwire, &regbuf);
                rpay = regwire; rplen = REG_WIRE_LEN;
            }
            break;
        case RPC_READ_ALL_UNSUPPORTED_REGS:
            memset(&regbuf, 0, sizeof(regbuf));
            ret = sl->backend->read_all_unsupported_regs(sl, &regbuf);
            if (ret == 0) {
                reg_to_wire(regwire, &regbuf);
                rpay = regwire; rplen = REG_WIRE_LEN;
            }
            break;
        case RPC_READ_UNSUPPORTED_REG:
            memset(&regbuf, 0, sizeof(regbuf));
            ret = sl->backend->read_unsupported_reg(sl, (int32_t)a0, &regbuf);
            if (ret == 0) {
                reg_to_wire(regwire, &regbuf);
                rpay = regwire; rplen = REG_WIRE_LEN;
            }
            break;
        case RPC_WRITE_UNSUPPORTED_REG:
            if (splen == REG_WIRE_LEN) {
                reg_from_wire(&regbuf, sl->q_buf);
                ret = sl->backend->write_unsupported_reg(sl, a0, (int32_t)a1, &regbuf);
                if (ret == 0) {
                    reg_to_wire(regwire, &regbuf);
                    rpay = regwire; rplen = REG_WIRE_LEN;
                }
            } else {
                ELOG("remote: invalid WRITE_UNSUPPORTED_REG payload length %u\n", splen);
                status = REMOTE_REPLY_PROTOCOL_ERROR;
            }
            break;
        case RPC_WRITE_REG:     ret = sl->backend->write_reg(sl, a0, (int32_t)a1); break;
        case RPC_CLOSE:         ret = 0; break;
        default:
            ELOG("remote: unknown opcode %u\n", op);
            status = REMOTE_REPLY_PROTOCOL_ERROR;
            ret = -1;
            break;
        }

        if (send_reply(fd, status, ret, rpay, rplen)) { return (-1); }

        if (op == RPC_CLOSE) { return (0); }
    }
}
