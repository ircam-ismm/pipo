
SRC_ROOT	= ../..
RTA_ROOT	= $(SRC_ROOT)/modules/rta
RTA_DIR		= $(RTA_ROOT)/src
OBJ_DIR		= obj
LIB_DIR		= lib

pipo-headers = \
	$(SRC_ROOT)/sdk/src/PiPo.h \
	$(SRC_ROOT)/sdk/src/PiPoParallel.h \
	$(SRC_ROOT)/sdk/src/PiPoSequence.h \
	$(SRC_ROOT)/sdk/src/host/PiPoHost.h \
	$(SRC_ROOT)/sdk/src/host/PiPoCollection.h

pipo-sources = \
        $(SRC_ROOT)/modules/collection/PiPoCollection.cpp \
        $(RTA_DIR)/signal/rta_correlation.c \
        $(RTA_DIR)/signal/rta_delta.c \
        $(RTA_DIR)/signal/rta_yin.c \
        $(RTA_DIR)/statistics/rta_mean_variance.c \
        $(RTA_DIR)/util/rta_bpf.c \
        $(RTA_DIR)/util/rta_int.c \
        $(RTA_DIR)/signal/rta_bands.c \
        $(RTA_DIR)/signal/rta_biquad.c \
        $(RTA_DIR)/signal/rta_dct.c \
        $(RTA_DIR)/signal/rta_fft.c \
        $(RTA_DIR)/signal/rta_mel.c \
        $(RTA_DIR)/signal/rta_moments.c \
        $(RTA_DIR)/signal/rta_onepole.c \
        $(RTA_DIR)/signal/rta_psy.c \
        $(RTA_DIR)/statistics/rta_selection.c

pipo-sourcesx = $(notdir $(pipo-sources:%.cpp=%.c))
pipo-objects = $(pipo-sourcesx:%.c=$(OBJ_DIR)/%.o)
pipo-sourcedirs = $(sort $(dir $(pipo-sources)))

VPATH = $(pipo-sourcedirs) $(OBJ_DIR)

INCLUDE += -I. \
	-I$(RTA_DIR) \
	-I$(RTA_DIR)/signal \
	-I$(RTA_DIR)/statistics \
	-I$(RTA_DIR)/recognition \
	-I$(RTA_DIR)/util \
	-I$(RTA_ROOT)/bindings/lib \
	-I$(SRC_ROOT)/sdk/src \
	-I$(SRC_ROOT)/sdk/src/host \
	-I$(SRC_ROOT)/modules \
	-I$(SRC_ROOT)/modules/finitedifferences \
	-I$(SRC_ROOT)/modules/bayesfilter/src

# compiler flags common to c and c++
FLAGS = -g -O3 $(INCLUDE)
CFLAGS   += $(FLAGS) -std=c99
# with gnu c++ all everything is easy
#CXXFLAGS += $(FLAGS) -std=gnu++11 -fpermissive
# with clang, -fext-numeric-literals is needed for complex definition
CXXFLAGS += $(FLAGS) -std=c++11 -fpermissive -fext-numeric-literals

pipo-lib = $(LIB_DIR)/libpipo.a


all: $(pipo-lib) dist

$(pipo-lib): | $(LIB_DIR)
$(pipo-lib): $(pipo-objects)
	ar rcs $@ $^ 

$(pipo-objects): | $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@


DISTNAME = pipo-v$(PIPO_VERSION)-linux-armhf

dist:	$(pipo-lib) $(pipo-headers)
	mkdir -p distrib/$(DISTNAME)/pipo
	cp -p $(pipo-lib) distrib/$(DISTNAME)
	cp -p $(pipo-headers) distrib/$(DISTNAME)/pipo

INSTALL_DIR = /root/Bela

install: $(pipo-lib) $(pipo-headers)
	mkdir -p $(INSTALL_DIR)/include/pipo
	cp -p $(pipo-lib) $(INSTALL_DIR)/lib
	cp -p $(pipo-headers)  $(INSTALL_DIR)/include/pipo

clean:
	-rm $(pipo-objects) $(pipo-lib)

new:	clean all

print:
	@echo pipo-sourcedirs = $(pipo-sourcedirs)
	@echo pipo-sources = $(pipo-sources)
	@echo pipo-objects = $(pipo-objects)
