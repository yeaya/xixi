SRCS = cache.cpp \
  currtime.cpp \
  io_service_pool.cpp \
  log.cpp \
  main.cpp \
  server.cpp \
  settings.cpp \
  stats.cpp \
  lookup3.cpp \
  util.cpp \
  peer.cpp \
  peer_cache.cpp \
  peer_pdu.cpp \
  auth.cpp

INCLUDE_OPTIONS = -I../3rd/boost

BIN = xixi

LINK_OPTIONS = -L../3rd/boost/stage/lib -lboost_system -lboost_thread -lpthread
#-lboost_log
#

CPPFLAGS = -DENDIAN_LITTLE

LINK_OBJS =

include ./Makefile.common
