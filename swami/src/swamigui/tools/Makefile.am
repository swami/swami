## Process this file with automake to produce Makefile.in
## Conditional compilation of cdump

EXTRA_DIST = cdump.c

# Only compile if SPLASH is enabled
if SPLASH
noinst_PROGRAMS = cdump
cdump_SOURCES = cdump.c
endif

MAINTAINERCLEANFILES = Makefile.in
