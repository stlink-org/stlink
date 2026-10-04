% st-server(1) Open source version of the STMicroelectronics STLINK Tools | stlink
%
% Jun 2026

# NAME

st-server - Serve a local ST-LINK over TCP for remote stlink tools

# SYNOPSIS

*st-server* \[*OPTIONS*\]

# DESCRIPTION

*st-server* opens one locally attached ST-LINK probe and exposes the stlink
backend operations over TCP. A remote *st-flash*(1), *st-info*(1), or
*st-util*(1) client can then use *--remote* to drive the probe from another
machine.

The server owns the USB connection to the ST-LINK. The client runs the normal
stlink target logic such as target connection, chip detection, erase, program,
verify, and GDB server behavior.

By default *st-server* listens only on the loopback address
*127.0.0.1:4500*. Use *--bind* to listen on another IPv4 address. Binding to
*0.0.0.0* or using *--bind=:PORT* exposes the probe to the network; only do
this on trusted networks or behind another access-control layer such as SSH.

# OPTIONS

\--bind *ADDR*\[:*PORT*\]
:   IPv4 address and optional port to listen on. The default is
    *127.0.0.1:4500*. Use *0.0.0.0* to listen on all IPv4 interfaces. The
    shorthand *:4500* is accepted and means *0.0.0.0:4500*.

\--port *PORT*
:   TCP port to listen on. This is kept for compatibility and can be combined
    with *--bind* when the bind address does not include a port.

\--serial *iSerial*
:   Serial number of the local ST-LINK device to serve. To serve multiple
    probes concurrently, run one *st-server* process per probe on different
    ports.

\--freq *kHz*
:   SWD/JTAG frequency in kHz used by the server when opening the local probe.
    Client commands may still set their own frequency during the remote connect
    sequence.

\--verbose\[=*LEVEL*\]
:   Enable verbose logging. If *LEVEL* is omitted, debug logging is enabled.

\--help
:   Print usage information.

# EXAMPLES

Serve the first local ST-LINK on the default loopback address:

    $ st-server

Serve a specific probe on all IPv4 interfaces:

    $ st-server --serial 57FF72067265575742132067 --bind=:4500

Use an SSH tunnel to keep *st-server* bound to loopback. On the host with the
probe, run:

    $ st-server

On the laptop, forward the port and use the tunnel:

    $ ssh -N -L 4500:127.0.0.1:4500 pi@raspberrypi &
    $ st-flash --remote 127.0.0.1:4500 write firmware.bin 0x08000000
    $ st-info  --probe --remote 127.0.0.1:4500
    $ st-util  --remote 127.0.0.1:4500

Serve two probes concurrently (one *st-server* per probe, on different ports):

    $ st-server --serial aaa --port 4500
    $ st-server --serial bbb --port 4501

# SECURITY

The protocol has no authentication or encryption: a client that can reach the
port has full debug and flash control of the probe. The default loopback bind
(*127.0.0.1*) avoids exposing the probe to the network. Use an SSH tunnel for
remote access, or use *--bind=0.0.0.0* only on a trusted, isolated network.

# RECOVERY

If *st-server* is killed during a flash operation, normal *SIGINT* and
*SIGTERM* shutdown paths close the ST-LINK handle cleanly. A forced kill,
process crash, or USB firmware fault can still leave the ST-LINK unresponsive.
If local tools such as *st-info --probe* also hang, reset the USB device with a
tool such as *usbreset* or physically replug the probe.

# SEE ALSO

st-flash(1), st-info(1), st-util(1), st-trace(1)

# COPYRIGHT

This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
