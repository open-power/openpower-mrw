/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwMemCommon.H $                                              */
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
//  LAST_VERSION_FROM_CC: 
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwMemCommon.H 1$
#ifndef MRWMEMCOMMON_H
#define MRWMEMCOMMON_H

#include <mrwParserCommon.H>


/**
 * This file contains interfaces and classes that the memory parsing code provides.
 * The memory parsing code finds the paths from processors out to the DIMMs.
 */



/**
 * A DDR bus segment, as seen in the card xml:
 * <ddr>
 *     <id>...</id>
 *     <source>....</source>
 *     <endpoint>...</endpoint>
 * </ddr>
 */
class DDRBus : public BusSegment
{
public:
    DDRBus(XMLElement& i_element, Plug* i_plug) : BusSegment(i_element, i_plug) {};
};



/**
 * A DMI bus segment, as seen in the card XML:
 * <dmi>
 *     <id>...</id>
 *     <source>....</source>
 *     <endpoint>...</endpoint>
 * </dmi>
 */
class DMIBus : public BusSegment
{

public:
    DMIBus(XMLElement& i_element, Plug* i_plug) :
        BusSegment(i_element, i_plug),
        iv_downstreamNPSwap(0), iv_upstreamNPSwap(0), iv_refclockMap(0),
        iv_rxSwap(false), iv_txSwap(false) {}

    int upstreamSwap() { return iv_upstreamNPSwap; }
    int downstreamSwap() { return iv_downstreamNPSwap; }
    int refclockMap() { return iv_refclockMap; }

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

    void refclockMap(const std::string & i_map)
    {
        iv_refclockMap = atoi(i_map.c_str());
    }


    bool rxSwap() const { return iv_rxSwap; }
    bool txSwap() const { return iv_txSwap;}

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


private:
    int iv_downstreamNPSwap;
    int iv_upstreamNPSwap;
    int iv_refclockMap;
    bool iv_rxSwap;
    bool iv_txSwap;

};


/**
 * Represents the bus endpoint that has the MCS unit.
 */
class MemMcs
{
public:
    MemMcs(Plug* i_plug, Endpoint & i_mcs, Endpoint & i_cent) :
            iv_plug(i_plug), iv_mcsEndpoint(i_mcs), iv_centEndpoint(i_cent)
    {}

        Plug* plug() { return iv_plug; }
        Endpoint& mcsEndpoint() { return iv_mcsEndpoint; }
        Endpoint& centEndpoint() { return iv_centEndpoint; }

        std::string path()
        {
            if (iv_path.empty())
            {
                std::string unitId    = iv_mcsEndpoint.unit();
                std::string chip      = iv_mcsEndpoint.id();
                std::string partId    = mrwGetPartId(iv_plug->card(), chip);
                std::string pos       = mrwGetPartPos(iv_plug->card(), chip);

                XMLElement dmiUnit = mrwGetUnit(partId, "dmi-master-unit", unitId);
                std::string chipPath  = iv_plug->path() + "/" + partId + "-" + pos;

                iv_path = chipPath + "/" + dmiUnit.getChildValue("chiplet-id");
            }

            return iv_path;
        }

private:
    Plug* iv_plug;
    Endpoint iv_mcsEndpoint;
    Endpoint iv_centEndpoint;
    std::string iv_path;

};


/**
 * Represents the bus endpoint that has the MBA unit.
 */
class MemMba
{
public:
    MemMba(Plug* i_plug, Endpoint & i_mba) :
            iv_plug(i_plug), iv_mbaEndpoint(i_mba) {};

        Plug* plug() { return iv_plug; }
        Endpoint& mbaEndpoint() { return iv_mbaEndpoint; }

        std::string path()
        {
            if (iv_path.empty())
            {
                std::string unitId    = iv_mbaEndpoint.unit();
                std::string chip      = iv_mbaEndpoint.id();
                std::string partId    = mrwGetPartId(iv_plug->card(), chip);
                std::string pos       = mrwGetPartPos(iv_plug->card(), chip);
                XMLElement unit  = mrwGetUnit(partId, "ddr-master-unit", unitId);
                std::string chipPath  = iv_plug->path() + "/" + partId + "-" + pos;

                iv_path = chipPath + "/" + unit.getChildValue("chiplet-id");
            }

            return iv_path;
        }

        std::string port()
        {
            if (iv_port.empty())
            {
                std::string unitId    = iv_mbaEndpoint.unit();
                std::string chip      = iv_mbaEndpoint.id();
                std::string partId    = mrwGetPartId(iv_plug->card(), chip);
                std::string pos       = mrwGetPartPos(iv_plug->card(), chip);

                XMLElement unit  = mrwGetUnit(partId, "ddr-master-unit", unitId);

                iv_port = unit.getChildValue("port");
            }

            return iv_port;
        }

        std::string slot()
        {
            if (iv_slot.empty())
            {
                std::string unitId    = iv_mbaEndpoint.unit();
                std::string chip      = iv_mbaEndpoint.id();
                std::string partId    = mrwGetPartId(iv_plug->card(), chip);
                std::string pos       = mrwGetPartPos(iv_plug->card(), chip);

                XMLElement unit  = mrwGetUnit(partId, "ddr-master-unit", unitId);

                iv_slot = unit.getChildValue("slot");
            }

            return iv_slot;
        }



private:
    Plug* iv_plug;
    Endpoint iv_mbaEndpoint;
    std::string iv_path;
    std::string iv_port;
    std::string iv_slot;

};

/**
 * Represents the bus endpoint that has the DRAM unit.
 */
class MemDram
{
public:
    MemDram(Plug* i_plug, Endpoint & i_dram) :
            iv_plug(i_plug), iv_dramEndpoint(i_dram) {};

        Plug* plug() { return iv_plug; }
        Endpoint& dramEndpoint() { return iv_dramEndpoint; }

        std::string dimmPath()
        {
            if (iv_dimmPath.empty())
            {
                //take the DIMM's card ID field in the instance path, and convert it to a card type instead
                //so it will say 'dimm', and not 'ddr3_16GB_dimm' for example

                std::string path = iv_plug->path();
                int p = path.find_last_of("/");

                std::string base = path.substr(0, p+1);

                std::string cardAndPos = path.substr(p+1, path.length() - p);
                std::string pos = cardAndPos.substr(cardAndPos.find_first_of("-"), cardAndPos.length() - cardAndPos.find_first_of("-"));

                std::string cardType = iv_plug->card().getChildValue("card-type");

                iv_dimmPath = base + cardType + pos;
            }

            return iv_dimmPath;
        }

        std::string path()
        {
            if (iv_path.empty())
            {
                iv_path = plug()->path() + "/";

                iv_path += mrwGetPartId(plug()->card(), dramEndpoint().id()) + "-";
                iv_path += mrwGetPartPos(plug()->card(), dramEndpoint().id());
            }

            return iv_path;
        }

private:
    Plug* iv_plug;
    Endpoint iv_dramEndpoint;
    std::string iv_dimmPath;
    std::string iv_path;

};




/**
 * A memory system bus, which combines all the connected bus segments between the cards.  Ties
 * together the DMI and DDR paths out to a DIMM.
 */
class MemSystemBus
{
public:
    MemSystemBus(MemMcs* i_mcs,
                 MemMba* i_mba,
                 MemDram* i_dram) :
      iv_mcs(i_mcs), iv_mba(i_mba), iv_dram(i_dram) {};

    MemMcs* mcs() { return iv_mcs; }
    MemMba* mba() { return iv_mba; }
    MemDram* dram() { return iv_dram; }

private:

    //Has an MCS endpoint that has the DMI unit on the Venice
    MemMcs* iv_mcs;

    //Has an MBA endpoint that has the DDR unit on the centaur
    MemMba* iv_mba;

    //Has an endpoint that has the DRAM unit on the DIMM
    MemDram* iv_dram;

};



/**
 * Creates a list of all of the DDRSystemBuses in the system.
 * These busses indicate the path of the memory from the MCS in a processor
 * out to the DDR units in the DIMMs.
 */
void mrwMemMakeBusses(std::vector<MemSystemBus*> & o_memBusses);


/**
 *  Returns the instance path for the DIMM plug passed in.
 */
std::string mrwMemGetDimmInstancePath(Plug* i_dimmPlug);


/**
 * Returns the MBA instance path.
 *
 * @param i_ddrEndpoint - the MBA endpoint of a DDRBus.
 * @param i_plug - the plug the bus is on
 */
std::string mrwMemGetMbaInstancePath(Endpoint & i_ddrEndpoint, Plug* i_plug);



/**
 * Returns the MBA port.
 *
 * @param i_ddrEndpoint - the MBA endpoint of a DDRBus.
 * @param i_plug - the plug the bus is on
 */
std::string mrwMemGetMbaPort(Endpoint & i_ddrEndpoint, Plug* i_plug);


/**
 * Returns the MBA slot.
 *
 * @param i_ddrEndpoint - the MBA endpoint of a DDRBus.
 * @param i_plug - the plug the bus is on
 */
std::string mrwMemGetMbaSlot(Endpoint & i_ddrEndpoint, Plug* i_plug);


/**
 * Returns the MCS instance path
 *
 * @param i_dmiSource - the MCS source of a DMIBus
 * @param i_plug - the plug the bus is on
 */
std::string mrwMemGetMcsInstancePath(Endpoint & i_dmiSource, Plug* i_plug);

#endif
