/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPCIECommon.H $                                             */
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
#ifndef MRWPCIECOMMON_H
#define MRWPCIECOMMON_H

/**
 * This file contains interfaces and classes that the PCIE parsing code provides.
 */

#include <algorithm>
#include <mrwParserCommon.H>


/**
 * The attributes of a PCIE bus
 */
struct pcieAttributes_t
{
    std::string card_size;
    std::string gen;
    std::string slot_index;
    std::string capi;
    std::string hot_plug;
    std::string is_slot;
    std::string width;
    std::string lsi;
    std::string card;
    std::string swapBits;
    std::string reversalBits;
    std::string dsmp;
    std::string hxSelect; //hx-lane-set-select
    std::string default_pcie_cooling_type;
    std::string default_power_consumption;


    bool empty()
    {
        return (slot_index.empty() && capi.empty() &&
                lsi.empty() && card_size.empty() && gen.empty() && hot_plug.empty() &&
                is_slot.empty() && width.empty());
    }

};


/**
 * A PCIE bus segment, as seen in the card xml
 */
class PCIEBus : public BusSegment
{

public:
    PCIEBus(XMLElement& i_element, Plug* i_plug) :
        BusSegment(i_element, i_plug) {};

    pcieAttributes_t & attributes() { return iv_attributes; }

    void attributes(pcieAttributes_t & i_attributes)
    {
        iv_attributes = i_attributes;
    }

private:

    pcieAttributes_t iv_attributes;

};



/**
 * An object that represents the root of a PCIE system bus
 */
class PCIERoot : public SystemEndpoint
{
public:

    PCIERoot(Endpoint & i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug)
    {
        iv_unit = mrwGetUnit(iv_partId, "pcie-root-unit", i_source.unit());
        iv_iop = atoi(iv_unit.getChildValue("iop").c_str());
    }

    XMLElement & unit() { return iv_unit; }

    int iop() { return iv_iop; }


private:

    int iv_iop;
    XMLElement iv_unit;

};




/**
 * An object that represents the endpoint of a PCIE system bus.
 */
class PCIESystemEndpoint : public SystemEndpoint
{
public:
    PCIESystemEndpoint(Endpoint & i_endpoint, Plug* i_plug) :
        SystemEndpoint(i_endpoint, i_plug)
    { }
};


/**
 * An object that represents a PCIE switch (a midpoint chip in the path)
 */
class PCIESwitch
{
public:

    PCIESwitch(const std::string & i_instanceId,
               const std::string & i_unitId,
               Plug* i_plug,
               XMLElement & i_upstreamUnit,
               const std::string & i_width,
               const std::string & i_station,
               const std:: string & i_port) :
        iv_instanceId(i_instanceId),
        iv_unitId(i_unitId),
        iv_plug(i_plug),
        iv_upstreamUnit(i_upstreamUnit),
        iv_width(i_width),
        iv_station(i_station),
        iv_port(i_port)
    {
    }

    std::string id() { return iv_instanceId; }
    std::string unitId() { return iv_unitId; }
    Plug* plug() { return iv_plug; }
    XMLElement & upstreamUnit() { return iv_upstreamUnit; }

    std::string width() { return iv_width; }
    std::string station() { return iv_station; }
    std::string port() { return iv_port; }

    std::string partId() { return mrwGetPartId(iv_plug->card(), iv_instanceId); }

    std::string getChipPath()
    {
        std::string path = plug()->path() + "/";

        path += mrwGetPartId(plug()->card(), id()) + "-";
        path += mrwGetPartPos(plug()->card(), id());

        return path;
    }

private:

    std::string iv_instanceId;
    std::string iv_unitId;
    Plug* iv_plug;
    XMLElement iv_upstreamUnit;
    std::string iv_width;
    std::string iv_station;
    std::string iv_port;


};

/**
 * A segment of a PCIESystemBus. If a part, then
 * the part element will have the instance-path in it.
 * Otherwise, it's a card and just the plug will be filled in.
 */
struct pciePathEntry_t
{
    Plug* plug;
    std::string part;

    pciePathEntry_t(Plug* i_plug, const std::string & i_part) :
        plug(i_plug), part(i_part) {}
    pciePathEntry_t(Plug* i_plug) : plug(i_plug) {}
};

typedef std::vector<pciePathEntry_t> pciePathEntries_t;



/**
 * An object that represents the end to end system pcie bus connection.
 */
class PCIESystemBus
{
public:
    PCIESystemBus(PCIERoot* i_root, PCIESystemEndpoint* i_dest,
                  pcieAttributes_t & i_attributes,
                  std::vector<pciePathEntry_t> & i_paths) :
        iv_root(i_root), iv_switch(NULL), iv_destination(i_dest),
       iv_attributes(i_attributes), iv_paths(i_paths)
    {
        checkAttributes();
    }

    PCIESystemBus(PCIERoot* i_root, PCIESwitch* i_switch,
                  PCIESystemEndpoint* i_dest,
                  pcieAttributes_t & i_attributes,
                  std::vector<pciePathEntry_t> & i_paths) :
         iv_root(i_root), iv_switch(i_switch), iv_destination(i_dest),
         iv_attributes(i_attributes), iv_paths(i_paths)
    {
        checkAttributes();
    }

    PCIERoot* root() { return iv_root; }
    PCIESwitch* switchChip() { return iv_switch; }
    PCIESystemEndpoint* destination() { return iv_destination; }
    pcieAttributes_t & attributes() { return iv_attributes; }

    void checkAttributes()
    {
        if (iv_attributes.empty())
        {
            mrwLogger logger;
            logger() << "The PCIE bus between " << iv_root->getChipPath() << "/" << iv_root->source().unit()
                     << " and " << iv_destination->getChipPath() << "/" << iv_destination->source().unit()
                     << " doesn't have any attributes set";
            logger.error(true);
        }
    }

    enum linkNum_t
    {
        ZERO,
        ONE
    };

    /**
     * Returns the lane mask for when a x16 link is bifurcated
     * into 2 x8 links.
     */
    std::string getBifurcatedLaneMask(linkNum_t i_link)
    {
        if (i_link == ZERO)
            return "0xFF00";
        else
            return "0x00FF";
    }

    /**
     * Returns the lane swap bits for the 2 x8 links stemming
     * from a bifurcated x16 link.
     */
    std::string getBifurcatedLaneSwap(linkNum_t i_link);


    /**
     * Returns the lane swap bits for the 2 x8 links stemming
     * from a bifurcated x16 link.
     */
    std::string getBifurcatedLaneReversal(linkNum_t i_link);

    std::vector<pciePathEntry_t> & getPath() { return iv_paths; }

private:

    PCIERoot* iv_root;
    PCIESwitch* iv_switch;
    PCIESystemEndpoint* iv_destination;
    pcieAttributes_t iv_attributes;
    pciePathEntries_t iv_paths;
};




/**
 * Creates a list of all of the PCIE busses in the system.
 */
void mrwPCIEMakeBusses(std::vector<PCIESystemBus*> & o_busses);


#endif

