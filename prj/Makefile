#
# $Id: Makefile,v 1.31 2014/09/08 11:14:54 gwesp Exp $
#
# TODO: 
# - tests and applications must be subdirectories of src, then the three build
#   rules can be collapsed into one.
# - Get rid of the symlinks, use += on SOURCES instead (or similar).
#
# Usage:
# Release build:
# make 
# make tests 
# make clean
#
# Debug build:
# make debug=true
# make tests debug=true
# make clean debug=true
#

RANLIB = ranlib
CREATELIB = ar cr $(CPP_LIB_LIBRARY) $(OBJECTS)

CXXFLAGS =

include def.platform
include def.compiler
include def.includes
include def.configuration

BASE_DIR        = ..
SRC_DIR         = ${BASE_DIR}/src
TEST_DIR        = ${BASE_DIR}/testing
APPS_DIR        = ${BASE_DIR}/applications
INCLUDE_DIR     = ${BASE_DIR}/include
INCLUDE_CPP_LIB = ${INCLUDE_DIR}/cpp-lib
OBJ_DIR         = ${BASE_DIR}/obj/${CONFIGURATION}
OBJ_SUBDIRS     = ${OBJ_DIR}/sys                 \
                  ${OBJ_DIR}/posix               \
                  ${OBJ_DIR}/posix1b             \
                  ${OBJ_DIR}/windows
BIN_DIR         = ${BASE_DIR}/bin/${CONFIGURATION}
CPP_LIB_LIBRARY = $(OBJ_DIR)/libcpp-lib.$(LIB_EXTENSION)

all: directories $(CPP_LIB_LIBRARY)

INCLUDES =                                 	\
	-I$(INCLUDE_DIR) 			\
	$(THIRD_PARTY_INCLUDES)

OBJECTS =					        \
	$(OBJ_DIR)/sys/file.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/sys/logger.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/sys/server.$(OBJ_EXTENSION)          \
	$(OBJ_DIR)/sys/syslogger.$(OBJ_EXTENSION)       \
	$(OBJ_DIR)/sys/util.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/audio.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/blowfish.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/cgi.$(OBJ_EXTENSION)	                \
	$(OBJ_DIR)/command_line.$(OBJ_EXTENSION)	\
	$(OBJ_DIR)/dispatch.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/geodb.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/geometry.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/gnss.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/http.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/igc.$(OBJ_EXTENSION)	                \
	$(OBJ_DIR)/interpolation.$(OBJ_EXTENSION)	\
	$(OBJ_DIR)/map.$(OBJ_EXTENSION)	                \
	$(OBJ_DIR)/math-util.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/nmea.$(OBJ_EXTENSION)	       	\
	$(OBJ_DIR)/network.$(OBJ_EXTENSION)	       	\
	$(OBJ_DIR)/ogn.$(OBJ_EXTENSION)	          	\
	$(OBJ_DIR)/registry.$(OBJ_EXTENSION)	        \
	$(OBJ_DIR)/util.$(OBJ_EXTENSION)		\
	$(OBJ_DIR)/random.$(OBJ_EXTENSION)		\
	$(OBJ_DIR)/varlist.$(OBJ_EXTENSION)

# Windoze specific parts.
WINDOWS_OBJECTS =                                                       \
	$(OBJ_DIR)/windows/iso8601.$(OBJ_EXTENSION)			\
	$(OBJ_DIR)/windows/serial.$(OBJ_EXTENSION)			\
	$(OBJ_DIR)/windows/realtime.$(OBJ_EXTENSION)			\
	$(OBJ_DIR)/windows/wrappers.$(OBJ_EXTENSION)			\
	$(OBJ_DIR)/windows/wrappers_special.$(OBJ_EXTENSION)

# POSIX specific parts.
POSIX_OBJECTS =								\
	$(OBJ_DIR)/posix/wrappers.$(OBJ_EXTENSION)			\
	$(OBJ_DIR)/posix/realtime.$(OBJ_EXTENSION)

# POSIX.1B (realtime) specific parts.
POSIX1B_OBJECTS =							\
	$(OBJ_DIR)/posix/wrappers.$(OBJ_EXTENSION)			\
	$(OBJ_DIR)/posix1b/realtime.$(OBJ_EXTENSION)

APPS  =				                                \
	$(BIN_DIR)/igctool

TESTS =				                                \
	$(BIN_DIR)/audio-test   	               		\
	$(BIN_DIR)/blowfish-test	               		\
	$(BIN_DIR)/command_line-test	                        \
	$(BIN_DIR)/dispatch-test     	                        \
	$(BIN_DIR)/crypt_stream-test	                        \
	$(BIN_DIR)/gnss-test        	                        \
	$(BIN_DIR)/interpolation-test		        	\
	$(BIN_DIR)/optimization-test		        	\
	$(BIN_DIR)/table-test		       	 		\
	$(BIN_DIR)/map-test	               			\
	$(BIN_DIR)/math-test	               			\
	$(BIN_DIR)/matrix-wrapper-test	       			\
	$(BIN_DIR)/registry-test	               		\
	$(BIN_DIR)/rtree-test	                       		\
	$(BIN_DIR)/spatial-index-test	               		\
	$(BIN_DIR)/quaternion-test	               		\
	$(BIN_DIR)/random-test	               			\
	$(BIN_DIR)/top-n-test		               		\
	$(BIN_DIR)/util-test		               		\
	$(BIN_DIR)/varlist-test	        			\
	$(BIN_DIR)/udp-test	                        	\
	$(BIN_DIR)/utc-test	                        	\
	$(BIN_DIR)/tcp-test					\
	$(BIN_DIR)/ogn-minimal                                  \
	$(BIN_DIR)/ogn-test                                     \
	$(BIN_DIR)/rosenbrock-search		        	\
	$(BIN_DIR)/sched-thread-test  		        	\
	$(BIN_DIR)/syslogger-test     		        	\
	$(BIN_DIR)/file-test					\
	$(BIN_DIR)/logger-test					\
	#$(BIN_DIR)/iowait					\
	#$(BIN_DIR)/netutil     				\
	#$(BIN_DIR)/file-test					\
	#$(BIN_DIR)/sched-test
	
WINDOWS_TESTS =							\
	$(BIN_DIR)/serial-test

PNG_TESTS =                                                     \
	$(BIN_DIR)/png-test

POSIX_TESTS =

POSIX1B_TESTS =


# DON't use spaces inside the parentheses.  It won't work.
ifeq ($(WINDOWS_STUFF),yes)
OBJECTS += $(WINDOWS_OBJECTS)
TESTS   += $(WINDOWS_TESTS)
endif

ifeq ($(PNG_STUFF),yes)
TESTS   += $(PNG_TESTS)
LDFLAGS += $(shell libpng-config --ldflags)
endif

ifeq ($(POSIX_STUFF),yes)
ifeq ($(POSIX1B_STUFF),yes)
OBJECTS += $(POSIX1B_OBJECTS)
TESTS   += $(POSIX1B_TESTS)
else
OBJECTS += $(POSIX_OBJECTS)
TESTS   += $(POSIX_TESTS)
endif
endif


ifdef DONT_KNOW_SYMLINKS
LINK = cp -r
else
LINK = ln -sf
endif


symlinks:
ifdef DONT_KNOW_SYMLINKS
	( cd $(INCLUDE_CPP_LIB) ; rm -fR platform    )
	( cd $(INCLUDE_CPP_LIB) ; rm -fR platform_rt )
else
	( cd $(INCLUDE_CPP_LIB) ; rm -f platform    )
	( cd $(INCLUDE_CPP_LIB) ; rm -f platform_rt )
endif
ifeq ($(POSIX_STUFF),yes)
ifeq ($(POSIX1B_STUFF),yes)
	( cd $(INCLUDE_CPP_LIB) ; $(LINK) posix1b platform_rt )
else
	( cd $(INCLUDE_CPP_LIB) ; $(LINK) posix   platform_rt )
endif
	( cd $(INCLUDE_CPP_LIB) ; $(LINK) posix   platform    )
endif
ifeq ($(WINDOWS_STUFF),yes)
	( cd $(INCLUDE_CPP_LIB) ; $(LINK) windows platform_rt )
	( cd $(INCLUDE_CPP_LIB) ; $(LINK) windows platform    )
endif


SOURCES = $(subst .o,.cpp,$(OBJECTS))

directories: $(OBJ_DIR) $(OBJ_SUBDIRS) $(BIN_DIR) 

$(CPP_LIB_LIBRARY): symlinks $(OBJECTS)
	$(CREATELIB)
	$(RANLIB) $(CPP_LIB_LIBRARY)

# Build tests
tests: 		directories $(TESTS)

# Build applications
apps:           directories $(APPS)

include def.recipes

$(BIN_DIR)/%: $(OBJ_DIR)/%.$(OBJ_EXTENSION) $(CPP_LIB_LIBRARY)
	$(COMPILE_BINARY_RECIPE)

$(OBJ_DIR)/%.$(OBJ_EXTENSION) : $(SRC_DIR)/%.cpp
	$(COMPILE_OBJECT_RECIPE)
$(OBJ_DIR)/%.$(OBJ_EXTENSION) : $(TEST_DIR)/%.cpp
	$(COMPILE_OBJECT_RECIPE)
$(OBJ_DIR)/%.$(OBJ_EXTENSION) : $(APPS_DIR)/%.cpp
	$(COMPILE_OBJECT_RECIPE)


$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_SUBDIRS):
	mkdir -p $(OBJ_SUBDIRS)


clean:
	rm -f *.o core *.out *.obj *.exe *.pdb
	rm -f $(SRC_DIR)/*.o
	rm -f $(OBJECTS) libcpp-lib.a
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	rm -f $(TEST_DIR)/surface.out.*
	rm -f $(TEST_DIR)/*.crypt
	rm -f $(TEST_DIR)/*.stackdump
	rm -rf $(INCLUDE_CPP_LIB)/platform
	rm -rf $(INCLUDE_CPP_LIB)/platform_rt

distclean: clean
	rm -f def.compiler
	rm -f def.platform
	rm -f .depend

# Only works with GNU.
#.depend: depend
#depend:	
#	g++ -MM $(INCLUDES) $(SOURCES) $(TEST_DIR)/*.cpp > .depend
#
#include .depend
