#ifndef GDB_REMOTE_H
#define GDB_REMOTE_H

#include <stdint.h>

int32_t gdb_send_packet(int32_t fd, char* data);
int32_t gdb_recv_packet(int32_t fd, char** buffer);
int32_t gdb_check_for_interrupt(int32_t fd);

#endif // GDB_REMOTE_H
