/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPowerBusCommon.H $                                         */
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
#ifndef MRWPOWERBUSCOMMON_H
#define MRWPOWERBUSCOMMON_H

/**
 * This file contains interfaces and classes that the PowerBus parsing code provides.
 */

#include <mrwParserCommon.H>


/**
 * A PowerBus has a single source and a single endpoint as seen in the
 * <powerbus> section of the <card> XML.
 */
class PowerBus : public BusSegment
{
public:

    PowerBus(XMLElement& i_element, Plug* i_plug) :
        BusSegment(i_element, i_plug),
        iv_rxSwap(false), iv_txSwap(false),
        iv_downstreamNPSwap(0), iv_upstreamNPSwap(0), iv_width(0) {};


    bool rxSwap() const { return iv_rxSwap; }
    bool txSwap() const { return iv_txSwap; }

    void rxSwap(const std::string & i_swap)
    {
        if ((i_swap.size() == 3) && (tolower(i_swap[0]) == 'y') &&
            (tolower(i_swap[1]) == 'e') && (tolower(i_swap[2]) == 's'))
            iv_rxSwap = true;
        else
            iv_rxSwap = false;
    }

    void txSwap(const std::string & i_swap)
    {
        if ((i_swap.size() == 3) && (tolower(i_swap[0]) == 'y') &&
            (tolower(i_swap[1]) == 'e') && (tolower(i_swap[2]) == 's'))
            iv_txSwap = true;
        else
            iv_txSwap = false;
    }

    int upstreamSwap() { return iv_upstreamNPSwap; }
    int downstreamSwap() { return iv_downstreamNPSwap; }

    void upstreamSwap(const std::string & i_swap)
    {
        //a hex number, like 0x11111111, enforced by mrw.xsd
        iv_upstreamNPSwap = strtoll(i_swap.c_str(), NULL, 16);
    }

    void downstreamSwap(const std::string & i_swap)
    {
        //a hex number, like 0x11111111, enforced by mrw.xsd
        iv_downstreamNPSwap = strtoll(i_swap.c_str(), NULL, 16);
    }


    void txSwap(bool i_swap) { iv_txSwap = i_swap; }

    int width() const { return iv_width; }
    void width(int w) { iv_width = w; }

private:

    bool iv_rxSwap;
    bool iv_txSwap;
    int iv_downstreamNPSwap;
    int iv_upstreamNPSwap;
    int iv_width;
};


/**
 * Contains information on a chip endpoint of a PowerBus connection
 */
class PowerBusSystemEndpoint : public SystemEndpoint
{
public:

    PowerBusSystemEndpoint(Endpoint & i_source, Plug* i_plug) :
       SystemEndpoint(i_source, i_plug)
    {}


    XMLElement getUnit()
    {
        std::string partId = mrwGetPartId(plug()->card(), source().id());
        XMLElement u = mrwGetUnit(partId, "powerbus-unit", source().unit());
        return u;
    }

};


/**
 * The Source of a PowerBus connection.
 * (Yes, bus is bidirectional, but picking a source for convenience.)
 */
class PowerBusSource : public PowerBusSystemEndpoint
{
public:
    PowerBusSource(Endpoint & i_source, Plug* i_plug) :
        PowerBusSystemEndpoint(i_source, i_plug) {}


};

/**
 * The destination of a PowerBus connection.
 */
class PowerBusDestination : public PowerBusSystemEndpoint
{
public:
    PowerBusDestination(Endpoint & i_source, Plug* i_plug) :
        PowerBusSystemEndpoint(i_source, i_plug) {}

};


/**
 * Represents a PowerBus bus as it goes from its source chip through the
 * system to the destination chip.
 */
class PowerSystemBus
{
public:
    PowerSystemBus(PowerBusSource* i_source,
                   PowerBusDestination* i_dest,
                   std::vector<std::string> & i_pathSegments,
                   bool i_rxSwap, bool i_txSwap,
                   int i_upstreamSwap, int i_downstreamSwap, 
                   int i_busWidth, Cable* i_cable) :
        iv_source(i_source), iv_destination(i_dest), iv_segments(i_pathSegments),
        iv_rxSwap(i_rxSwap), iv_txSwap(i_txSwap),
        iv_upstreamLaneSwap(i_upstreamSwap), iv_downstreamLaneSwap(i_downstreamSwap),
        iv_busWidth(i_busWidth), iv_cable(i_cable)
    {

        if (i_busWidth == 0)
        {
            mrwLogger l;
            l() << "Bus Width not set on powerbus between " << iv_source->getChipPath() <<
                   " and " << iv_destination->getChipPath();
            l.error(true);
        }
    }


    PowerBusSource* source() { return iv_source; }
    PowerBusDestination* destination() { return iv_destination; }

    //Returns the path segments
    std::vector<std::string> & segments() { return iv_segments; }

    bool rxSwap() { return iv_rxSwap; }
    bool txSwap() { return iv_txSwap; }
    int busWidth() { return iv_busWidth; }

    std::string getNodeConfig()
    {
        std::string config;

        if (iv_cable)
        {
           config = iv_cable->element().getChildValue("include-for-node-config");
        }

        if (config.empty()) config = "all";

        return config;
    }

    enum Type
    {
        X_BUS,
        A_BUS
    };

    Type getType()
    {
        Type type;
        XMLElement unit = source()->getUnit();
        std::string t = unit.getChildValue("type");

        if (t == "X")
            type = X_BUS;
        else //schema will enforce either X or A only
            type = A_BUS;

        return type;
    }

    int getUpstreamNPLaneSwap() { return iv_upstreamLaneSwap; }
    int getDownstreamNPLaneSwap() { return iv_downstreamLaneSwap; }

    enum bus_endpoint_type_t
    {
        PB_SOURCE,
        PB_DEST
    };

    std::string getCableConnPath(bus_endpoint_type_t i_type);

    Cable* cable() { return iv_cable; }

    std::string getCableName()
    {
        std::string name;

        if (iv_cable) {
            name = iv_cable->element().getChildValue("name");
        }

        return name;
    }

private:

    PowerBusSource* iv_source;
    PowerBusDestination* iv_destination;
    std::vector<std::string> iv_segments;
    bool iv_rxSwap;
    bool iv_txSwap;
    int iv_upstreamLaneSwap;
    int iv_downstreamLaneSwap;
    int iv_busWidth;
    Cable* iv_cable;

};


/**
 * Creates a list of all of the PowerBus busses in the system which
 * connect the processors.
 */
void mrwPowerBusMakeBusses(std::vector<PowerSystemBus*> & o_busses);



#endif
