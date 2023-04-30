#ifndef GDB_SERVER_H_
#define GDB_SERVER_H_

#define STRINGIFY_inner(name) #name
#define STRINGIFY(name) STRINGIFY_inner(name)

#define DEFAULT_LOGGING_LEVEL 50
#define DEBUG_LOGGING_LEVEL 100
#define DEFAULT_GDB_LISTEN_PORT 4242

#endif // GDB_SERVER_H_
