AUTOMAKE_OPTIONS = foreign
lib_LTLIBRARIES = libactor.la
libactor_la_SOURCES = logger.cpp actor_address.cpp actor_system.cpp actor.cpp
DEFAULT_INCLUDES = -I.
AM_CPPFLAGS = -I../include -DSTRERROR_R_CHAR_P
AM_CXXFLAGS = 
LIB_VERSION ?= 1:0:0
libactor_la_LIBADD = 
libactor_la_LDFLAGS = -version-info $(LIB_VERSION) -lpthread

