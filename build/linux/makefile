
SRC_ROOT	= ../..
RTA_DIR		= $(SRC_ROOT)/modules/rta
OBJ_DIR		= obj
LIB_DIR		= lib

pipo-headers = \
	$(SRC_ROOT)/sdk/include/PiPo.h \
	$(SRC_ROOT)/sdk/include/PiPoParallel.h \
	$(SRC_ROOT)/sdk/include/PiPoSequence.h \
	$(SRC_ROOT)/sdk/host/PiPoHost.h \
	$(SRC_ROOT)/sdk/collection/PiPoCollection.h

pipo-sources = \
        $(SRC_ROOT)/modules/collection/PiPoCollection.cpp \
        $(RTA_DIR)/misc/rta_correlation.c \
        $(RTA_DIR)/misc/rta_delta.c \
        $(RTA_DIR)/misc/rta_mean_variance.c \
        $(RTA_DIR)/misc/rta_yin.c \
        $(RTA_DIR)/common/rta_bpf.c \
        $(RTA_DIR)/common/rta_int.c \
        $(RTA_DIR)/misc/rta_bands.c \
        $(RTA_DIR)/misc/rta_biquad.c \
        $(RTA_DIR)/misc/rta_dct.c \
        $(RTA_DIR)/misc/rta_fft.c \
        $(RTA_DIR)/misc/rta_mel.c \
        $(RTA_DIR)/misc/rta_moments.c \
        $(RTA_DIR)/misc/rta_onepole.c \
        $(RTA_DIR)/misc/rta_selection.c \
        $(RTA_DIR)/psy/psy.c

pipo-sourcesx = $(notdir $(pipo-sources:%.cpp=%.c))
pipo-objects = $(pipo-sourcesx:%.c=$(OBJ_DIR)/%.o)
pipo-sourcedirs = $(sort $(dir $(pipo-sources)))

VPATH = $(pipo-sourcedirs) $(OBJ_DIR)

INCLUDE += -I. \
	-I$(RTA_DIR) \
	-I$(RTA_DIR)/common \
	-I$(RTA_DIR)/misc \
	-I$(RTA_DIR)/psy \
	-I$(SRC_ROOT)/sdk/include \
	-I$(SRC_ROOT)/sdk/host \
	-I$(SRC_ROOT)/modules \
	-I$(SRC_ROOT)/sdk/collection

# compiler flags common to c and c++
FLAGS = -g -O3 $(INCLUDE)  -fpermissive
CFLAGS   += $(FLAGS) -std=c99
CXXFLAGS += $(FLAGS)

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
