/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwDMICommon.H $                                              */
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
#ifndef MRWDMICOMMON_H
#define MRWDMICOMMON_H

#include <mrwParserCommon.H>

/**
 * Contains objects for walking the DMI busses.  Similar to mrwMemCommon,
 * but doesn't have anything for the Centaur<->DRAM side of things.
 */


/**
 * A DMI master - i.e. an MCS on a processor
 */
class DMIMaster : public SystemEndpoint
{
public:

    DMIMaster(Endpoint& i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug) {}


    std::string getMcsPath()
    {
        if (iv_mcsPath.empty())
        {
            std::string unitId = iv_source.unit();
            std::string partId = mrwGetPartId(iv_plug->card(), iv_source.id());
            std::string pos    = mrwGetPartPos(iv_plug->card(), iv_source.id());

            XMLElement dmiUnit = mrwGetUnit(partId, "dmi-master-unit", unitId);
            std::string chipPath  = iv_plug->path() + "/" + partId + "-" + pos;

            iv_mcsPath = chipPath + "/" + dmiUnit.getChildValue("chiplet-id");
        }

        return iv_mcsPath;
    }

private:
    std::string iv_mcsPath;

};


/**
 * A DMI slave - i.e. the Centaur
 */
class DMISlave : public SystemEndpoint
{
public:
    DMISlave(Endpoint& i_source, Plug* i_plug) :
        SystemEndpoint(i_source, i_plug) {}

};


/**
 * Represents a DMI bus between an MCS on the processor
 * and a Centaur.
 */
class DMISystemBus
{
public:
    DMISystemBus(DMIMaster* i_master, DMISlave* i_slave,
                 int i_upstreamSwap, int i_downstreamSwap,
                 bool i_rxSwap, bool i_txSwap, int i_map) :
        iv_master(i_master), iv_slave(i_slave),
        iv_upstreamLaneSwap(i_upstreamSwap),
        iv_downstreamLaneSwap(i_downstreamSwap),
        iv_rxSwap(i_rxSwap), iv_txSwap(i_txSwap),
        iv_refclockMap(i_map) {}

    DMIMaster* master() { return iv_master; }
    DMISlave* slave() { return iv_slave; }

    int getUpstreamNPLaneSwap() { return iv_upstreamLaneSwap; }
    int getDownstreamNPLaneSwap() { return iv_downstreamLaneSwap; }
    bool rxSwap() { return iv_rxSwap; }
    bool txSwap() { return iv_txSwap; }
    int refclockMap() { return iv_refclockMap; }

private:
    DMIMaster* iv_master; //MCS
    DMISlave* iv_slave;   //Centaur

    int iv_upstreamLaneSwap;
    int iv_downstreamLaneSwap;
    bool iv_rxSwap;
    bool iv_txSwap;
    int iv_refclockMap;
};


/**
 * Creats the DMISystemBus objects for the system, from the source MCSs to the
 * endpoint Centaurs
 */
void mrwDMIMakeBusses(std::vector<DMISystemBus*> & o_busses);



#endif
