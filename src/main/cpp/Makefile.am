
libdir = $(prefix)/lib
lib_LTLIBRARIES = libmodule_stream_pipe.la

libmodule_stream_pipe_la_SOURCES  = stream_pipe.cpp
libmodule_stream_pipe_la_CXXFLAGS = -Wall -I$(srcdir)/src -I$(builddir) @ROBOTKERNEL_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ @YAML_CPP_CFLAGS@
libmodule_stream_pipe_la_LDFLAGS  = -Wall -release $(PACKAGE_VERSION)

install-exec-hook:
	mkdir -p $(prefix)/share/ln
	-cp -rf $(top_srcdir)/share/ln/* $(prefix)/share/ln/
