/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwMemCommon.C $                                              */
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
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <mrwParserCommon.H>
#include <mrwMemCommon.H>

using namespace std;


//globals
vector<DDRBus*> g_ddrBusses;
vector<DMIBus*> g_dmiBusses;
vector<MemSystemBus*> g_systemBusList;


//mrwParserCommon defines these globals
extern vector<Plug*> g_plugs;



/**
 *  Returns the instance path for the DIMM plug passed in.
 */
string mrwMemGetDimmInstancePath(Plug* i_dimmPlug)
{
    //take the DIMM's card ID field in the instance path, and convert it to a card type instead
    //so it will say 'dimm', and not 'ddr3_16GB_dimm' for example

    string path = i_dimmPlug->path();
    int p = path.find_last_of("/");

    string base = path.substr(0, p+1);

    string cardAndPos = path.substr(p+1, path.length() - p);
    string pos = cardAndPos.substr(cardAndPos.find_first_of("-"), cardAndPos.length() - cardAndPos.find_first_of("-"));

    string cardType = i_dimmPlug->card().getChildValue("card-type");

    path = base + cardType + pos;

    return path;
}

/**
 * Returns the MBA instance path.
 *
 * @param i_ddrEndpoint - the MBA endpoint of a DDRBus.
 * @param i_plug - the plug the bus is on
 */
string mrwMemGetMbaInstancePath(Endpoint & i_ddrEndpoint, Plug* i_plug)
{
    string unitId    = i_ddrEndpoint.unit();
    string chip      = i_ddrEndpoint.id();
    string partId    = mrwGetPartId(i_plug->card(), chip);
    string pos       = mrwGetPartPos(i_plug->card(), chip);

    XMLElement unit  = mrwGetUnit(partId, "ddr-master-unit", unitId);

    string chipPath  = i_plug->path() + "/" + partId + "-" + pos;
    string mbaPath   = chipPath + "/" + unit.getChildValue("chiplet-id");

    return mbaPath;
}


/**
 * Returns the MBA port.
 *
 * @param i_ddrEndpoint - the MBA endpoint of a DDRBus.
 * @param i_plug - the plug the bus is on
 */
string mrwMemGetMbaPort(Endpoint & i_ddrEndpoint, Plug* i_plug)
{
    string unitId    = i_ddrEndpoint.unit();
    string chip      = i_ddrEndpoint.id();
    string partId    = mrwGetPartId(i_plug->card(), chip);
    string pos       = mrwGetPartPos(i_plug->card(), chip);

    XMLElement unit  = mrwGetUnit(partId, "ddr-master-unit", unitId);

    return unit.getChildValue("port");
}


/**
 * Returns the MBA slot.
 *
 * @param i_ddrEndpoint - the MBA endpoint of a DDRBus.
 * @param i_plug - the plug the bus is on
 */
string mrwMemGetMbaSlot(Endpoint & i_ddrEndpoint, Plug* i_plug)
{
    string unitId    = i_ddrEndpoint.unit();
    string chip      = i_ddrEndpoint.id();
    string partId    = mrwGetPartId(i_plug->card(), chip);
    string pos       = mrwGetPartPos(i_plug->card(), chip);

    XMLElement unit  = mrwGetUnit(partId, "ddr-master-unit", unitId);

    return unit.getChildValue("slot");
}


/**
 * Returns the MCS instance path
 *
 * @param i_dmiSource - the MCS source of a DMIBus
 * @param i_plug - the plug the bus is on
 */
string mrwMemGetMcsInstancePath(Endpoint & i_dmiSource, Plug* i_plug)
{
    string unitId    = i_dmiSource.unit();
    string chip      = i_dmiSource.id();
    string partId    = mrwGetPartId(i_plug->card(), chip);
    string pos       = mrwGetPartPos(i_plug->card(), chip);

    XMLElement dmiUnit = mrwGetUnit(partId, "dmi-master-unit", unitId);

    string chipPath  = i_plug->path() + "/" + partId + "-" + pos;
    string mcsPath   = chipPath + "/" + dmiUnit.getChildValue("chiplet-id");

    return mcsPath;
}




/**
 * Returns the DDR busses that have a source at the part ID(a centaur instance) passed in.
 * Used to find the DDR busses that start where a DMI bus leaves off.
 */
void getDDRBusses(const string & i_sourcePartId, Plug* i_plug, vector<DDRBus*> & o_ddrBusses)
{
    vector<DDRBus*>::iterator b;

    o_ddrBusses.clear();

    //Get the DDR busses that have the source ID equal to i_sourcePartId

    for (b=g_ddrBusses.begin();b!=g_ddrBusses.end();++b)
    {
        if ((i_sourcePartId == (*b)->source().id()) && (i_plug == (*b)->plug()))
        {
            o_ddrBusses.push_back(*b);
        }
        else if ((i_sourcePartId == (*b)->endpoint().id()) && (i_plug == (*b)->plug()))
        {
            o_ddrBusses.push_back(*b);
        }
    }
}


/**
 * Walks the source DDR bus passed in out to the ending unit, and then
 * creates the DDRSystemBus object that contains information about the whole path
 */
void walkDDRBus(DDRBus* i_sourceBus, endpointType i_type, MemMcs * i_mcs)
{
    DDRBus* bus = i_sourceBus;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    endpointType type = (i_type == SOURCE) ? ENDPOINT : SOURCE;
    bool done = false;


    //only need to walk busses if there's more than 1 segment
    if (endpoint->unit().empty())
    {
        do
        {
            bus = mrwGetNextBus<DDRBus>(bus, type);

            if (!bus) break;

            if (type == SOURCE)
                done = !bus->source().unit().empty();
            else
                done = !bus->endpoint().unit().empty();



        } while (!done);
    }


    if (!bus)
    {
        ostringstream msg;
        msg << "Couldn't complete a DDR bus that started at " <<
                i_sourceBus->plug()->path() << "/" << i_sourceBus->source().id() << "/" << i_sourceBus->source().unit();
        mrwError(msg.str());

        return;
    }


    //the midpoint of the system bus - the source of the DDR/end of the DMI.
    //The ID of i_sourceBus->source() is the centaur instance
    //The unit of i_sourceBus->source is the DDR unit
    MemMba* mba = new MemMba(i_sourceBus->plug(), *source);

    //the destination of the system bus, the DIMM
    MemDram* dram = new MemDram(bus->plug(), (type == SOURCE) ? bus->source() : bus->endpoint());

    if (mrwLogger::getDebugMode())
    {
        string m = "Found DDR endpoint " + mrwUnitPath(dram->plug(), dram->dramEndpoint());
        mrwLogger::debug(m);
    }

    //the whole system bus
    MemSystemBus* systemBus = new MemSystemBus(i_mcs, mba, dram);

    g_systemBusList.push_back(systemBus);

}



/**
 * Walks the DDR busses out from the partId passed in, where this partId is for the
 * centaur that is the source of the DDR busses
 */
void walkDDRBusses(const string & i_partId, Plug* i_plug, MemMcs * i_mcs)
{
    vector<DDRBus*> ddrBusses;
    vector<DDRBus*>::iterator bus;

    //Get the DDR busses that start at this partID.
    getDDRBusses(i_partId, i_plug, ddrBusses);


    for (bus=ddrBusses.begin();bus!=ddrBusses.end();++bus)
    {
        if (!(*bus)->source().unit().empty())
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out DDR source " + mrwUnitPath((*bus)->plug(), (*bus)->source());
                mrwLogger::debug(m);
            }

            walkDDRBus(*bus, SOURCE, i_mcs);
        }
        else if (!(*bus)->endpoint().unit().empty())
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out DDR source " + mrwUnitPath((*bus)->plug(), (*bus)->endpoint());
                mrwLogger::debug(m);
            }

            walkDDRBus(*bus, ENDPOINT, i_mcs);
        }

    }

}




/**
 * Walks the DMIBus passed in out past the Centaurs and over the DDR busses to the DIMMs
 */
void walkMemoryBus(DMIBus* i_bus, endpointType i_type)
{
    DMIBus* dmiBus = i_bus;
    bool unitEmpty = false;
    endpointType type;


    //First walk the DMI bus from the CPU unit out to the membuf unit,
    //then walk the DDR bus from the membuf unit to the DIMM unit

    if (i_type == SOURCE)
    {
        unitEmpty = dmiBus->endpoint().unit().empty();
        type = ENDPOINT;
    }
    else
    {
        unitEmpty = dmiBus->source().unit().empty();
        type = SOURCE;
    }

    if (unitEmpty)
    {

        do
        {
            dmiBus = mrwGetNextBus(dmiBus, type);

            if (!dmiBus)
                break;

            //Our ending unit could be on the source or endpoint of a bus
            if (!dmiBus->source().unit().empty() || !dmiBus->endpoint().unit().empty())
                unitEmpty = false;

        }  while (unitEmpty);
    }

    if (!dmiBus)
    {
        Endpoint* e = (i_type == SOURCE) ? &i_bus->source() : &i_bus->endpoint();

        ostringstream msg;
        msg << "No endpoint found for a DMI bus path that starts at " <<
                i_bus->plug()->path() << "/" << e->id() << "/" << e->unit();
        mrwInfo(msg.str());

        return;
    }


    //////////////////////////////////////////////////////////////////

    //Now that we we have the ending unit of the DMI bus,
    //we look for DDR busses going out from that same part and
    //walk them out to the DIMMs

    //Create the system bus source - the endpoint that has the DMI unit on the CPU
    MemMcs* mcs;

    //The MCS needs to know the endpoint/unit of the cent on the far end
    Endpoint centEndpoint = (type == SOURCE) ? dmiBus->source() : dmiBus->endpoint();

    if (i_type == SOURCE)
        mcs = new MemMcs(i_bus->plug(), i_bus->source(), centEndpoint);
    else
        mcs = new MemMcs(i_bus->plug(), i_bus->endpoint(), centEndpoint);

    if (mrwLogger::getDebugMode())
    {
        string m = "Found DMI endpoint " + mrwUnitPath(dmiBus->plug(), mcs->centEndpoint());
        mrwLogger::debug(m);
    }


    walkDDRBusses(centEndpoint.id(), dmiBus->plug(), mcs);

}




/**
 * Finds every DMIBus segment that starts at a CPU and walks it out
 * to the destination.
 */
void processMemoryBusses()
{
    vector<DMIBus*>::iterator bus;


    for (bus=g_dmiBusses.begin();bus!=g_dmiBusses.end();++bus)
    {
        //The system source of the DMI bus is on either a CPU bus's source or endpoint

        DMIBus* dmi = (DMIBus*) *bus;

        if (mrwGetPartType(dmi->plug(), dmi->source().id()) == "cpu")
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out DMI source " + mrwUnitPath(dmi->plug(), dmi->source());
                mrwLogger::debug(m);
            }

            walkMemoryBus(dmi, SOURCE);
        }
        else if (mrwGetPartType(dmi->plug(), dmi->endpoint().id()) == "cpu")
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out DMI source " + mrwUnitPath(dmi->plug(), dmi->endpoint());
                mrwLogger::debug(m);
            }

            walkMemoryBus(dmi, ENDPOINT);
        }
    }


}


/**
 * Creates a list of all of the DDRSystemBuses in the system.
 * These busses indicate the path of the memory from the MCS in a processor
 * out to the DDR units in the DIMMs.
 */
void mrwMemMakeBusses(vector<MemSystemBus*> & o_memBusses)
{
    g_ddrBusses.clear();
    g_dmiBusses.clear();
    g_systemBusList.clear();

    mrwMakePlugs();

    mrwMakeBusses<DMIBus>(g_plugs, "dmi", g_dmiBusses);
    mrwMakeBusses<DDRBus>(g_plugs, "ddr", g_ddrBusses);

    processMemoryBusses();

    o_memBusses = g_systemBusList;

}


