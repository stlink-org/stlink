# stlink-lib backend notes

This directory contains the transport backends used by the common stlink code.
The common code drives targets through `stlink_backend_t` function pointers;
backend implementations translate those operations into the actual transport.

The main backends are:

- `usb.c`: native USB/libusb ST-LINK backend.
- `sg_legacy.c`: legacy SCSI passthrough backend.
- `remote.c`: TCP backend used by `st-server` and client-side `--remote`.

## Remote Backend

The remote backend lets a client tool drive an ST-LINK attached to another
machine. The server process owns the real USB connection. The client runs the
normal stlink logic: connect mode, chip detection, flash erase, program,
verify, and GDB server behavior.

`st-server` only forwards backend operations. Target policy remains in the
client.

Typical flow:

1. `st-server` opens a local ST-LINK with the USB backend.
2. A client opens `st-flash --remote HOST:PORT`, `st-info --remote HOST:PORT`,
   or `st-util --remote HOST:PORT`.
3. The server sends a handshake with protocol and ST-LINK version data.
4. The client creates a `stlink_t` using the remote backend.
5. Common code calls backend operations on the client.
6. `remote.c` serializes those calls over TCP.
7. The server dispatches each RPC to the real USB backend and sends the result
   back.

## Device Ownership

One `st-server` serves one already-open ST-LINK. To serve multiple probes, run
multiple server processes on different ports:

```sh
st-server --serial aaa --port 4500
st-server --serial bbb --port 4501
```

This keeps each probe independent. A long `st-util` session or flash operation
on one probe cannot block another probe behind the same server process.

## Protocol

All integer fields are little-endian and encoded with `read_uint32()` and
`write_uint32()`.

Handshake, server to client:

```text
magic u32
protocol_version u32
capabilities u32
stlink_v u32
jtag_v u32
swim_v u32
st_vid u32
stlink_pid u32
jtag_api u32
flags u32
max_trace_freq u32
serial char[STLINK_REMOTE_SERIAL_WIRE_LEN]
```

`protocol_version` is currently `STLINK_REMOTE_PROTOCOL_VERSION`.
`capabilities` is reserved for future optional protocol features and is
currently sent as zero.

The `serial` field has a fixed on-wire size, `STLINK_REMOTE_SERIAL_WIRE_LEN`.
This is separate from the internal `STLINK_SERIAL_BUFFER_SIZE`, so the protocol
layout does not change if the internal buffer changes. The server zero-pads the
field and a compile-time check (`remote.h`) guarantees the buffer fits. If the
wire size ever has to grow, bump `STLINK_REMOTE_PROTOCOL_VERSION`.

Request, client to server:

```text
op u32
ap u32
arg0 u32
arg1 u32
payload_len u32
payload bytes
```

Reply, server to client:

```text
status u32
ret i32
payload_len u32
payload bytes
```

`status` describes the remote protocol result. `ret` is the backend return
value and is valid when `status == REMOTE_REPLY_OK`, even when the backend
returned `-1`. Protocol errors use `REMOTE_REPLY_PROTOCOL_ERROR`.

Transport errors are not represented in the protocol. Once the TCP stream has
failed, it cannot reliably carry a reply. `send_all()` and `recv_all()` log TCP
errors locally.

## Register Encoding

The remote protocol does not send C structs directly. `struct stlink_reg` is
encoded field-by-field as fixed little-endian `u32` values. This avoids
depending on compiler padding, host ABI, or host endianness.

If `struct stlink_reg` changes, update both `reg_to_wire()` and
`reg_from_wire()` in `remote.c`. If the wire layout changes incompatibly, bump
`STLINK_REMOTE_PROTOCOL_VERSION`.

## Access Ports

The client owns the logical AP state. Each RPC carries the client's current
`sl->ap`, and the server applies it before dispatching the backend operation.

When common code calls `backend->init_ap()`, the remote backend sends
`RPC_INIT_AP`; the server then calls the real USB backend's `init_ap()`.
Therefore AP selection decisions stay client-side, while ST-LINK USB commands
are executed server-side.

## Connect Mode And Reset

Connect mode (normal, hot-plug, connect-under-reset) is handled by the client
over RPC. `st-server` has no `--connect-under-reset` flag:

- The server's `stlink_open_usb()` opens the *ST-LINK USB device* and returns a
  valid handle even when the target connection fails. Target firmware cannot
  stop the server from claiming the probe; it can only fail the *target*
  connect, which the server does not need.
- The connect sequence is transport-agnostic and runs on the client over RPC.
  When a client uses `--connect-under-reset`, `stlink_open_remote()` mirrors the
  tail of `stlink_open_usb()`: it asserts `NRST` low (`RPC_JTAG_RESET`), then
  runs `stlink_target_connect()` (enter SWD, probe AP, halt/reset). The reset
  sequence is issued on the real probe, over RPC.

Use client-side `--connect-under-reset` when firmware prevents normal attach:

```sh
st-flash --remote HOST:PORT --connect-under-reset write firmware.bin 0x08000000
```

Server-side reset policy would duplicate client connect logic and reset the
target on each server start. This still relies on `NRST` being wired, as local
`--connect-under-reset` does.

## Trace (SWO)

SWO/SWV trace works over the remote backend. `trace_enable`/`trace_disable` are
plain RPCs; `trace_read` is a variable-length read where the reply's `ret` is
the captured byte count and the payload carries the bytes (0 = none available).
`st-trace --remote HOST:PORT` then behaves like the local tool.

Unlike flash and debug, trace is **best-effort**. It is a continuous stream and
the ST-LINK has a finite trace FIFO, so every `trace_read` poll is a network
round trip. On a LAN or SSH tunnel (with `TCP_NODELAY`, which the backend sets)
polling keeps up with typical SWO rates; on a high-latency link the FIFO can
overflow and trace bytes are dropped silently. Use trace over remote on
low-latency links only.

## Bind Address And Security

The protocol has no authentication or encryption. Any client that can reach the
port has full debug/flash control of the probe. `st-server` defaults to the
loopback bind `127.0.0.1:4500`.

Use an SSH tunnel to keep the server bound to loopback while adding
authentication and encryption.

```sh
# on the host with the probe:
st-server
# on the client:
ssh -N -L 4500:127.0.0.1:4500 user@probe-host &
st-flash --remote 127.0.0.1:4500 write fw.bin 0x08000000
```

Only bind to `0.0.0.0` (or `--bind=:PORT`) on a trusted, isolated network.

## Shutdown And Recovery

`st-server` handles `SIGINT` and `SIGTERM` by leaving the accept loop and
closing the ST-LINK handle through the normal `stlink_close()` path. This keeps
normal shutdowns clean.

Forced termination, crashes, or a killed process during a USB transfer can
still leave the ST-LINK firmware or USB endpoint unresponsive. If local tools
such as `st-info --probe` also hang, reset the USB device with `usbreset` or
physically replug the probe.
