## NAME
```
    rkm - remote keyboard and mouse
```

## SYNOPSIS
```
    rkm [-sv] [-h host] [-p port]
```

## DESCRIPTION

The `rkm` utility acts as a bridge from the host system's mouse
to a controlled system.

By default, `rkm` will act as a server unless the `-s` option
is is used or `server = true` appears in a configuration file.

When acting as a client, a window is opened and the mouse and keyboard
are grabbed. The devices are released if focus is lost from the
window (e.g. through Alt+Tab) or when by closing the program by
hitting the *Menu* key.

When acting as a server, it will attempt to bind the port given
by the `-p port` option or `port = number` in a configuration file.


Options given on the command line take precedence.

Options:
```
  -h host   Host to connect to
  -p port   Port to serve from or connect to
            0 to select random port
  -v        Display verbose information
```

## FILES
All config files named `rkm.conf` are read from the following directories
in order. Values in later configuration files override prior. Options
given on the command line override options given in config files.

```
    /etc
    /usr/local/etc
    $XDG_CONFIG_DIRS
    $HOME
    $XDG_CONFIG_HOME
```

The file is a list of key-value pairs separated by an equal sign.
Space is allowed around the equal sign, before the key, and after the value.
Comments are introduced with `#`.

```
break   The key that exits the program.
        By default it is the Menu key.
        This value is given in X11 "Keysym" format (listed in `X11/keysymdef.h`)
        E.g. the Right Alt key would be `Alt_R`
host    Host to connect to (either an IP or a hostname)
port    The port to listen (server) on or connect to (client)
server  Start as server (`true`, `yes`, or `1`)
        or client (`false`, `no`, or `0`)
```

### Example
```
# Connect to a server started on PHOEBE:1111
port = 1111
host = PHOEBE
server = false
break = Menu
```


## EXIT STATUS
```
0   No error
1   A fatal error occurred
```

## AUTHORS
Copyright Jerry Williams <williams.jerry.jr@gmail.com>
https://github.com/jwillia3/rkm
