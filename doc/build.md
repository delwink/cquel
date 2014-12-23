BUILD NOTES
===========

cquel is designed for *nix operating systems, and it is build using the GNU
build system.

To Build
--------

The following commands must be executed from the root of the repository
(the one containing `doc/`).

    ./autogen.sh
    ./configure
    make

Dependencies
------------

Library         | Description                                  | Purpose
--------------- | -------------------------------------------- | ---------------------
MariaDB Client  | MariaDB client tools                         | Database Connectivity
MariaDB Library | MariaDB database development files           | Compiler flags

Dependency Build Instructions: Debian and Trisquel
--------------------------------------------------

Build requirements:

    # apt-get install build-essential pkg-config
    # apt-get install libtool autotools-dev autoconf automake
    # # need some mysql things while Debian is broken
    # apt-get install libmariadb2 libmysqlclient-dev

Dependency Build Instructions: Parabola
---------------------------------------

Build requirements:

    # pacman -S base-devel
    # pacman -S mariadb-clients
