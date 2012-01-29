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
  auth.cpp \
  ../3rd/tinyxml/tinystr.cpp \
  ../3rd/tinyxml/tinyxml.cpp \
  ../3rd/tinyxml/tinyxmlerror.cpp \
  ../3rd/tinyxml/tinyxmlparser.cpp

INCLUDE_OPTIONS = -I../3rd/boost -I../3rd/tinyxml

LINK_OPTIONS = -L../3rd/boost/stage/lib -lboost_system -lboost_thread -lboost_filesystem -lpthread
#-lboost_log
#

CPPFLAGS = 

LINK_OBJS =

###############################################################################
CC = g++
CPPFLAGS += -DNDEBUG -O2
INCLUDE_OPTIONS +=
LINK_OPTIONS +=

#GCOV_CPPFLAGS = -ftest-coverage -fprofile-arcs
#GCOV_LINK_OPTION = -lgcov

OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))

BINDIR = ../bin
OBJDIR = ../obj
TARGET = $(BINDIR)/$(BIN)

all: $(TARGET)

$(TARGET) : $(OBJS)
	echo "Linking $@";
	@if [ ! -d $(BINDIR) ]; \
	then \
		mkdir -p $(BINDIR); \
	fi
	$(CC) $(OBJS) $(LINK_OPTIONS) $(GCOV_LINK_OPTION) $(LINK_OBJS) -o $@; \

$(OBJDIR)/%.o : %.cpp
	@if [ ! -d $(OBJDIR) ]; \
	then \
		mkdir -p $(OBJDIR); \
	fi
	$(CC) -c $(CPPFLAGS) $(GCOV_CPPFLAGS) $(INCLUDE_OPTIONS) $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
