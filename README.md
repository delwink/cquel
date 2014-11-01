cquel
=====

A wrapper for the MySQL API with dynamic data structures.

Why?
----

When developing [libpatts][1], we noticed that the functionality going into the
query API could be put to good use in future projects as well, so we decided to
make a wrapper for it with the name cquel.

When is it useful?
------------------

cquel is designed to be useful when trying to do complicated tasks with data
from a database that would be easier to do in the programming language than in
SQL.

For running simple queries, you are probably better off just using the MySQL
API. There will be fewer opportunities to forget about allocated memory, and the
setup is much simpler.

cquel is designed for a situation where a table of data is pulled in from the
database and the user wants to be able to arbitrarily modify the data in that
table and then commit it to the database. The data are held in memory for
editing, and then the entire structure is parsed in order to construct an update
query to the database.

Installation
------------

After following the steps in [doc/build.md][2], you can install the shared
library with

    # make install

Alternatively, all the compiled libraries are placed in `.libs` on success,
including the static one.

API
---

You can read a basic tutorial in [doc/api.md][5], you can read the official
API outline on [Delwink's website][6], or you can generate it yourself with

    $ doxygen Doxyfile

if you have doxygen installed.

License
-------

cquel is [free software][3]\: you are free to copy, modify, share, sell, and
redistribute under the terms of version 3 of the GNU Affero General Public
License. See [COPYING][4] for more details.

[1]: http://git.delwink.com/git/summary/libpatts.git
[2]: doc/build.md
[3]: http://gnu.org/philosophy/free-sw.html
[4]: COPYING
[5]: doc/api.md
[6]: http://delwink.com/software/apidocs/cquel
