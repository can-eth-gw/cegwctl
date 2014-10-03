 #############################################################################
 # Makefile - Control Area Network - Ethernet - Gateway (Utilities)
 #############################################################################

 #############################################################################
 # (C) Copyright 2013 Fabian Raab, Stefan Smarzly
 #
 # This file is part of CAN-Eth-GW.
 #
 # CAN-Eth-GW is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # CAN-Eth-GW is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with CAN-Eth-GW.  If not, see <http://www.gnu.org/licenses/>.
 #############################################################################

BUILDDIR = bin
SRCDIR = src
BINDIR = bin
INCLUDEDIR = include
PWD  := $(shell pwd)
TARGET = $(BINDIR)/cegwctl
LIBS = `pkg-config --libs libnl-3.0 libnl-genl-3.0` -lm
CC = gcc
CFLAGS := -g -Wall `pkg-config --cflags libnl-3.0 libnl-genl-3.0` -std=gnu99
CFLAGS += -I$(PWD)/$(INCLUDEDIR)
VPATH = $(SRCDIR)


.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(wildcard $(SRCDIR)/*.c))
HEADERS = $(wildcard $(INCLUDEDIR)/*.h)

$(BUILDDIR)/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f $(BUILDDIR)/*.o
	-rm -f $(TARGET)


install:
	-cp "$(TARGET)" "/usr/local/bin/$(shell basename $(TARGET))"
	-chmod +rx "/usr/local/bin/cegwctl"
	-cp "./man/cegwctl.8" "/usr/share/man/man8/cegwctl.8"
	-chmod +rx "/usr/share/man/man8/cegwctl.8"

