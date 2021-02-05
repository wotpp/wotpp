# wot++

BUILD_DIR=build
TARGET=wpp
LIBS=$(LDLIBS)
INC=-Imodules/ -Isrc/

CXX?=clang++

SRC=src/main.cpp
STD=c++17
CXXWARN=-Wall -Wextra -Wcast-align -Wcast-qual -Wformat=2 -Wredundant-decls -Wshadow -Wundef -Wwrite-strings
# CXXFLAGS+=-fno-exceptions -fno-rtti

symbols?=yes
sanitize?=no
profile?=yes
release?=no

ifeq ($(release),no)
	CXXFLAGS+=-O1

else ifeq ($(release),yes)
	CXXFLAGS+=-Os -march=native -flto -DNDEBUG

else
$(error debug should be either yes or no)
endif


ifeq ($(sanitize),no)

else ifeq ($(sanitize),yes)
	CXXFLAGS+=-fsanitize=undefined,address

else
$(error sanitize should be either yes or no)
endif


ifeq ($(symbols),no)
	CXXFLAGS+=-s

else ifeq ($(symbols),yes)
	CXXFLAGS+=-g
else
$(error symbols should be either yes or no)
endif


ifeq ($(profile),no)

else ifeq ($(profile),yes)
	CXXFLAGS+=-finstrument-functions

else
$(error profile should be either yes or no)
endif


ifeq ($(CXX),clang++)
	CXXWARN+=-ferror-limit=2
endif


.POSIX:

all: options wotpp

config:
	@mkdir -p $(BUILD_DIR)/

options:
	@echo "cc       = $(CXX)"
	@echo "symbols  = $(symbols)"
	@echo "sanitize = $(sanitize)"
	@echo "release  = $(release)"
	@echo "profile  = $(profile)"
	@echo "flags    = -std=$(STD) $(CXXWARN) $(CXXFLAGS)"

wotpp: config
	@$(CXX) -std=$(STD) $(CXXWARN) $(CXXFLAGS) $(LDFLAGS) $(CPPFLAGS) $(INC) $(LIBS) -o $(BUILD_DIR)/$(TARGET) $(SRC)

clean:
	@rm -rf $(BUILD_DIR)/

.PHONY: all options clean

