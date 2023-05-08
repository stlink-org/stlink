#if defined(_WIN32)

#include <stdint.h>

#include "win32_socket.h"

#undef socket
#undef connect
#undef accept
#undef shutdown

#include <string.h>
#include <errno.h>
#include <assert.h>

int32_t win32_poll(struct pollfd *fds, uint32_t nfds, int32_t timo) {
    struct timeval timeout, *toptr;
    fd_set ifds, ofds, efds, *ip, *op;
    uint32_t i;
    int32_t rc;

#ifdef _MSC_VER
#pragma warning(disable: 4548)
#endif

    /* Set up the file-descriptor sets in ifds, ofds and efds. */
    FD_ZERO(&ifds);
    FD_ZERO(&ofds);
    FD_ZERO(&efds);

    for (i = 0, op = ip = 0; i < nfds; ++i) {
        fds[i].revents = 0;

        if (fds[i].events & (POLLIN | POLLPRI)) {
            ip = &ifds;
            FD_SET(fds[i].fd, ip);
        }

        if (fds[i].events & POLLOUT) {
            op = &ofds;
            FD_SET(fds[i].fd, op);
        }

        FD_SET(fds[i].fd, &efds);
    }

#ifdef _MSC_VER
#pragma warning(default: 4548)
#endif

    /* Set up the timeval structure for the timeout parameter */
    if (timo < 0) {
        toptr = 0;
    } else {
        toptr = &timeout;
        timeout.tv_sec = timo / 1000;
        timeout.tv_usec = (timo - timeout.tv_sec * 1000) * 1000;
    }

#ifdef DEBUG_POLL
    printf("Entering select() sec=%ld usec=%ld ip=%lx op=%lx\n",
           (long)timeout.tv_sec, (long)timeout.tv_usec, (long)ip, (long)op);
#endif

    rc = select(0, ip, op, &efds, toptr);

#ifdef DEBUG_POLL
    printf("Exiting select rc=%d\n", rc);
#endif

    if (rc <= 0) { return(rc); }

    if (rc > 0) {
        for ( i = 0; i < nfds; ++i) {
            SOCKET fd = fds[i].fd;

            if (fds[i].events & (POLLIN | POLLPRI) && FD_ISSET(fd, &ifds)) {
                fds[i].revents |= POLLIN;
            }

            if (fds[i].events & POLLOUT && FD_ISSET(fd, &ofds)) {
                fds[i].revents |= POLLOUT;
            }

            if (FD_ISSET(fd, &efds)) {
                // Some error was detected ... should be some way to know
                fds[i].revents |= POLLHUP;
            }

#ifdef DEBUG_POLL
    printf("%d %d %d revent = %x\n",
           FD_ISSET(fd, &ifds),
           FD_ISSET(fd, &ofds),
           FD_ISSET(fd, &efds),
           fds[i].revents);
#endif

        }
    }

    return(rc);
}

static void set_connect_errno(int32_t winsock_err) {
    switch (winsock_err) {
    case WSAEINVAL:
    case WSAEALREADY:
    case WSAEWOULDBLOCK:
        errno = EINPROGRESS;
        break;
    default:
        errno = winsock_err;
        break;
    }
}

static void set_socket_errno(int32_t winsock_err) {
    switch (winsock_err) {
    case WSAEWOULDBLOCK:
        errno = EAGAIN;
        break;
    default:
        errno = winsock_err;
        break;
    }
}

/*
 * A wrapper around the socket() function.
 * The purpose of this wrapper is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
SOCKET win32_socket(int32_t domain, int32_t type, int32_t protocol) {
    SOCKET fd = socket(domain, type, protocol);

    if (fd == INVALID_SOCKET) { set_socket_errno(WSAGetLastError()); }

    return(fd);
}

/* 
 * A wrapper around the connect() function.
 * The purpose of this wrapper is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
int32_t win32_connect(SOCKET fd, struct sockaddr *addr, socklen_t addr_len) {
    int32_t rc = connect(fd, addr, addr_len);
    assert(rc == 0 || rc == SOCKET_ERROR);

    if (rc == SOCKET_ERROR) { set_connect_errno(WSAGetLastError()); }

    return(rc);
}

/* A wrapper around the accept() function.
 * The purpose of this wrapper is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
SOCKET win32_accept(SOCKET fd, struct sockaddr *addr, socklen_t *addr_len) {
    SOCKET newfd = accept(fd, addr, addr_len);

    if (newfd == INVALID_SOCKET) {
        set_socket_errno(WSAGetLastError());
        newfd = (SOCKET)-1;
    }

    return(newfd);
}

/* A wrapper around the shutdown() function.
 * The purpose of this wrapper is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
int32_t win32_shutdown(SOCKET fd, int32_t mode) {
    int32_t rc = shutdown(fd, mode);
    assert(rc == 0 || rc == SOCKET_ERROR);

    if (rc == SOCKET_ERROR) { set_socket_errno(WSAGetLastError()); }

    return(rc);
}

int32_t win32_close_socket(SOCKET fd) {
    int32_t rc = closesocket(fd);

    if (rc == SOCKET_ERROR) { set_socket_errno(WSAGetLastError()); }

    return(rc);
}

ssize_t win32_write_socket(SOCKET fd, void *buf, int32_t n) {
    int32_t rc = send(fd, buf, n, 0);

    if (rc == SOCKET_ERROR) { set_socket_errno(WSAGetLastError()); }

    return(rc);
}

ssize_t win32_read_socket(SOCKET fd, void *buf, int32_t n) {
    int32_t rc = recv(fd, buf, n, 0);

    if (rc == SOCKET_ERROR) { set_socket_errno(WSAGetLastError()); }

    return(rc);
}


char * win32_strtok_r(char *s, const char *delim, char **lasts) {
    register char *spanp;
    register int32_t c, sc;
    char *tok;


    if (s == NULL && (s = *lasts) == NULL) { return (NULL); }

    // skip (span) leading delimiters (s += strspn(s, delim), sort of).
cont:
    c = *s++;

    for (spanp = (char *)delim; (sc = *spanp++) != 0;)
        if (c == sc) { goto cont; }


    if (c == 0) { // no non-delimiter characters
        *lasts = NULL;
        return (NULL);
    }

    tok = s - 1;

    /* Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for ( ; ;) {
        c = *s++;
        spanp = (char *)delim;

        do {
            if ((sc = *spanp++) == c) {
                if (c == 0) {
                    s = NULL;
                } else {
                    s[-1] = 0;
                }

                *lasts = s;
                return(tok);
            }

        } while (sc != 0);
    }

    // NOT REACHED
}

char *win32_strsep (char **stringp, const char *delim) {
    register char *s;
    register const char *spanp;
    register int32_t c, sc;
    char *tok;

    if ((s = *stringp) == NULL) {
        return(NULL);
    }

    for (tok = s; ;) {
        c = *s++;
        spanp = delim;

        do {
            if ((sc = *spanp++) == c) {
                if (c == 0) {
                    s = NULL;
                } else {
                    s[-1] = 0;
                }

                *stringp = s;
                return(tok);
            }

        } while (sc != 0);
    }

    // NOT REACHED
}

#ifndef STLINK_HAVE_UNISTD_H
int32_t usleep(uint32_t waitTime) {
    if (waitTime >= 1000) {
        /* Don't do long busy-waits.
         * However much it seems like the QPC code would be more accurate,
         * you can and probably will lose your time slice at any point during the wait,
         * so we might as well voluntarily give up the CPU with a WaitForSingleObject.
         */
        HANDLE timer;
        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -10 * (LONGLONG)waitTime;
        timer = CreateWaitableTimer(NULL, TRUE, NULL);
        SetWaitableTimer(timer, &dueTime, 0, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);

        return(0);
    }

    LARGE_INTEGER perf_cnt, start, now;

    QueryPerformanceFrequency(&perf_cnt);
    QueryPerformanceCounter(&start);

    do {
        QueryPerformanceCounter((LARGE_INTEGER*)&now);
    } while ((now.QuadPart - start.QuadPart) / (float)perf_cnt.QuadPart * 1000 * 1000 < waitTime);

    return(0);
}
#endif

#endif // defined(_WIN32)
