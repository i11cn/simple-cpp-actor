AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = actor

actor_SOURCES = main.cpp test_thread_pool.cpp test_actor.cpp test_actor_system.cpp test_perf.cpp test_timer.cpp test_logger.cpp
actor_LDADD = ../src/libactor.la -lgtest_main -lgtest -lpthread

DEFAULT_INCLUDES = -I.
AM_CPPFLAGS = -I../include
AM_CXXFLAGS = -static

