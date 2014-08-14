# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: makefile $
#
# OpenPOWER HostBoot Project
#
# Contributors Listed Below - COPYRIGHT 2014
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.
#
# IBM_PROLOG_END_TAG

PROGRAMS += mrwTargetParser
PROGRAMS += mrwPCIEParser
PROGRAMS += mrwPowerBusParser
PROGRAMS += mrwMergeElements
PROGRAMS += mrwCecChips
PROGRAMS += mrwFSIParser
PROGRAMS += mrwMemParser
PROGRAMS += mrwI2CParser
PROGRAMS += mrwPower
PROGRAMS += mrwChipIDs
PROGRAMS += mrwDMIParser
PROGRAMS += mrwMruIdParser
PROGRAMS += mrwLocationCodeParser

SOURCE += xmlutil.C
SOURCE += mrwTargetParser.C
SOURCE += mrwParserCommon.C
SOURCE += mrwMemCommon.C
SOURCE += mrwPCIEParser.C
SOURCE += mrwPCIECommon.C
SOURCE += mrwPowerBusParser.C
SOURCE += mrwPowerBusCommon.C
SOURCE += mrwFSICommon.C
SOURCE += mrwI2CCommon.C
SOURCE += mrwPowerCommon.C
SOURCE += mrwLocationCodeParser.C
SOURCE += mrwDMICommon.C
SOURCE += mrwMergeElements.C
SOURCE += mrwChipIDs.C
SOURCE += mrwCecChips.C
SOURCE += mrwFSIParser.C
SOURCE += mrwMemParser.C
SOURCE += mrwI2CParser.C
SOURCE += mrwDMIParser.C
SOURCE += mrwPower.C
SOURCE += mrwMruIdParser.C


SOURCE_OBJS  = $(basename $(SOURCE))
SOURCE_OBJS := $(addsuffix .o, $(SOURCE_OBJS))


all : ${SOURCE_OBJS} ${PROGRAMS}

clean :
	rm -rf ${SOURCE_OBJS}
	rm -rf ${PROGRAMS}

install:
	mkdir -p ${INSTALL_DIRECTORY}
	/usr/bin/install ${PROGRAMS} ${INSTALL_DIRECTORY}

CFLAGS       = -I. -I/usr/include/

LDFLAGS := -L/lib64
CFLAGS := -g -I. -I/usr/include/libxml2/ -DMRW_OPENPOWER
CC := g++
LD := g++



# *****************************************************************************
# Compile code for the common C++ objects if their respective
# code has been changed.  Or, compile everything if a header
# file has changed.
# *****************************************************************************
$(SOURCE_OBJS): %.o : %.C
	$(CC) -c $(CFLAGS) $< -o $@

# *****************************************************************************
# Create the programs
# *****************************************************************************
mrwMergeElements: mrwMergeElements.o xmlutil.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwTargetParser: mrwTargetParser.o xmlutil.o mrwParserCommon.o mrwMemCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwPowerBusParser: mrwPowerBusParser.o xmlutil.o mrwParserCommon.o mrwPowerBusCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwPCIEParser: mrwPCIEParser.o xmlutil.o mrwParserCommon.o mrwPCIECommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwI2CParser: mrwI2CParser.o xmlutil.o mrwParserCommon.o mrwI2CCommon.o mrwMemCommon.o mrwFSICommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwMemParser: mrwMemParser.o xmlutil.o mrwParserCommon.o mrwMemCommon.o mrwFSICommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwFSIParser: mrwFSIParser.o xmlutil.o mrwParserCommon.o mrwFSICommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwChipIDs: mrwChipIDs.o xmlutil.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwSystemPolicy: mrwSystemPolicy.o xmlutil.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwCecChips: mrwCecChips.o xmlutil.o mrwParserCommon.o mrwFSICommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwPower: mrwPower.o xmlutil.o mrwPowerCommon.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwDMIParser: mrwDMIParser.o mrwDMICommon.o xmlutil.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwMruIdParser: mrwMruIdParser.o xmlutil.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2

mrwLocationCodeParser: mrwLocationCodeParser.o xmlutil.o mrwParserCommon.o
	$(LD) $(LDFLAGS) -o $@  $^  -lstdc++ -lgcc -lxml2
