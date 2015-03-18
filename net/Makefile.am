AUTOMAKE_OPTIONS = foreign
lib_LTLIBRARIES = libactor_net.la

libactor_net_la_SOURCES = tcp.cpp epoll.cpp

DEFAULT_INCLUDES = -I.
AM_CPPFLAGS = -I../include
AM_CXXFLAGS =

LIB_VERSION ?= 1:0:0
libactor_net_la_LIBADD = ../src/libactor.la
libactor_net_la_LDFLAGS = -version-info $(LIB_VERSION)
