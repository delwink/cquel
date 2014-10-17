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

Library        | Description                                  | Purpose
-------------- | -------------------------------------------- | ---------------------
MariaDB Client | MariaDB client tools                         | Database Connectivity
ICU            | International Components for Unicode library | Unicode Support

Dependency Build Instructions: Debian and Trisquel
--------------------------------------------------

Build requirements:

    # apt-get install build-essential pkg-config
    # apt-get install libtool autotools-dev autoconf automake
    # apt-get install mariadb-client libicu-dev

Dependency Build Instructions: Parabola
---------------------------------------

Build requirements:

    # pacman -S base-devel
    # pacman -S mariadb-clients icu
