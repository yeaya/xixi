BIN = xixibase

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
  peer_http.cpp \
  peer_pdu.cpp \
  auth.cpp

INCLUDE_OPTIONS = -I../3rd/boost

LINK_OPTIONS = -L../3rd/boost/stage/lib -lboost_system -lboost_thread -lpthread
#-lboost_log
#

CPPFLAGS = 
#-DENDIAN_LITTLE

LINK_OBJS =

###############################################################################
CC = g++
CPPFLAGS += -DNDEBUG -O2
INCLUDE_OPTIONS +=
LINK_OPTIONS +=

#GCOV_CPPFLAGS = -ftest-coverage -fprofile-arcs
#GCOV_LINK_OPTION = -lgcov

OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))

BINDIR = ..
OBJDIR = $(BINDIR)/obj
TARGET = $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET) : $(OBJS)
	echo "Linking $@"; \
	$(CC) $(OBJS) $(LINK_OPTIONS) $(GCOV_LINK_OPTION) $(LINK_OBJS) -o $@; \

$(OBJDIR)/%.o : %.cpp
	@if [ ! -d $(OBJDIR) ]; \
	then \
		mkdir -p $(OBJDIR); \
	fi
	$(CC) -c $(CPPFLAGS) $(GCOV_CPPFLAGS) $(INCLUDE_OPTIONS) $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
