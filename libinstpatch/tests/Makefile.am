## Process this file with automake to produce Makefile.in

TESTS = sample_test sample_list_test
check_PROGRAMS = sample_test sample_list_test

sample_test_SOURCES = sample_test.c
sample_test_LDADD = $(top_srcdir)/libinstpatch/libinstpatch-1.0.la

sample_list_test_SOURCES = sample_list_test.c
sample_list_test_LDADD = $(top_srcdir)/libinstpatch/libinstpatch-1.0.la

INCLUDES = -I$(top_srcdir) @GOBJECT_CFLAGS@
