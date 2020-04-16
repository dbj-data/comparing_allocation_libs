# Klib: a Generic Library in C

## Overview

Klib is a standalone and lightweight C library distributed under [MIT/X11
license][1]. Most components are independent of external libraries, except the
standard C library, and independent of each other. To use a component of this
library, you only need to copy a couple of files to your source code tree
without worrying about library dependencies.

Klib strives for efficiency and a small memory footprint. Some components, such
as khash.h, kbtree.h, ksort.h and kvec.h, are among the most efficient
implementations of similar algorithms or data structures in all programming
languages, in terms of both speed and memory use.

A documentation is available [here](http://attractivechaos.github.io/klib/)