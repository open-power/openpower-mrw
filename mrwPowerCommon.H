/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPowerCommon.H $                                            */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014                             */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#ifndef MRWPOWERCOMMON_H
#define MRWPOWERCOMMON_H


#include <mrwParserCommon.H>

/**
 * Includes utilities for walking the <power> wires throughout the system.
 */



/**
 *  Represents a power wire connection
 */
class Wire : public BusSegment
{
public:
    Wire(XMLElement& i_element, Plug* i_plug) : BusSegment(i_element, i_plug) {}

};



/**
 * A PowerSource (supply, VRD, etc)
 */
class PowerSource : public SystemEndpoint
{
public:
    PowerSource(Endpoint & i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug), iv_port(-1) {}

    int getPort();

private:
    int iv_port;
};


/**
 * A PowerSink
 */
class PowerSink : public SystemEndpoint
{
public:
    PowerSink(Endpoint & i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug) {}

};


/**
 * A connection between a source and sink
 */
class PowerConnection
{
public:

    PowerConnection(PowerSource* i_source, PowerSink* i_sink) :
        iv_source(i_source), iv_sink(i_sink)
    {
    }

    PowerSource* source() { return iv_source; }
    PowerSink* sink() { return iv_sink; }

private:

    PowerSource* iv_source;
    PowerSink* iv_sink;

};


/**
 * Walks a power wire through a system and creates a PowerConnection for it.
 *
 */
void mrwPowerWalkPowerWire(Wire* i_sourceWire, endpointType i_type,
                           PowerSource *& io_source,
                           std::vector<PowerConnection*> & o_connections);

/**
 * Creates all of the PowerConnection objects in the system
 */
void mrwPowerMakeConnections(std::vector<PowerConnection*> & o_connections);


#endif

