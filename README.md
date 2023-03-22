## NAME
```
    rkmc - remote keyboard and mouse client
    rkms - remote keyboard and mouse server
```

## SYNOPSIS
```
    rkmc HOST:PORT
    rkms PORT
```

## DESCRIPTION

The `rkmc` and `rkms` act as a bridge from the host system's keyboard
and mouse to a controlled system.

Start `rkms` on the system to be controlled. Select a port to be hosted on.
e.g.
```
    % rkms 1111
```

Start `rkmc` on the system with the mouse and keyboard. The host should be
the IP or hostname of the system that `rkms` was started on and the same
port number should be used.
```
    % rkmc othersystem:1111
```


## EXIT STATUS
```
0   No error
1   A fatal error occurred
```

## AUTHORS
Copyright Jerry Williams <williams.jerry.jr@gmail.com>
https://github.com/jwillia3/rkm
