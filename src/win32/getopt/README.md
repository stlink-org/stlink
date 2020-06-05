getopt_port
===========

[Kim Gräsman](http://grundlig.wordpress.com)  
[@kimgr](http://twitter.com/kimgr)

An original implementation of `getopt` and `getopt_long` with limited GNU extensions. Provided under the BSD license, to allow non-GPL projects to use `getopt`-style command-line parsing.

So far only built with Visual C++, but has no inherently non-portable constructs.

Intended to be embedded into your code tree -- `getopt.h` and `getopt.c` are self-contained and should work in any context.

Comes with a reasonable unit test suite.

See also:

 * [Full Win32 getopt port](http://www.codeproject.com/Articles/157001/Full-getopt-Port-for-Unicode-and-Multibyte-Microso) -- LGPL licensed.
 * [XGetOpt](http://www.codeproject.com/Articles/1940/XGetopt-A-Unix-compatible-getopt-for-MFC-and-Win32) -- No `getopt_long` support.
 * [Free Getopt](https://sourceforge.net/projects/freegetopt/) -- No `getopt_long` support.

For license terms, see LICENSE.txt.