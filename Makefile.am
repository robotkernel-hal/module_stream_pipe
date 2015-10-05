ACLOCAL_AMFLAGS = -I m4 -I scripts/m4

libdir = $(prefix)/lib
lib_LTLIBRARIES = libmodule_stream_pipe.la

libmodule_stream_pipe_la_SOURCES  = stream_pipe.cpp
libmodule_stream_pipe_la_CXXFLAGS = -Wall -I$(top_srcdir)/src -I$(top_builddir) @ROBOTKERNEL_CFLAGS@
libmodule_stream_pipe_la_LDFLAGS  = -Wall -release $(PACKAGE_VERSION)

install-exec-hook:
	mkdir -p $(prefix)/share/ln
	-cp -rf $(srcdir)/share/ln/* $(prefix)/share/ln/
