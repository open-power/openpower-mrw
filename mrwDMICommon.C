/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwDMICommon.C $                                              */
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
#include <mrwMemCommon.H>
#include <mrwDMICommon.H>

using namespace std;


extern vector<Plug*> g_plugs;
vector<DMIBus*> g_dmiBusses;


/**
 * Creates all the DMIBus objects that are on the card of the Plug passed in
 */
void makeDMIBusses(Plug* i_plug)
{
    vector<XMLElement> busGroups;
    vector<XMLElement> busses;
    vector<XMLElement>::iterator busGroup;
    vector<XMLElement>::iterator b;
    XMLElement swap;

    if (i_plug->type() == Plug::CARD)
    {
        i_plug->card().findPath("busses").getChildren(busGroups);

        for (busGroup=busGroups.begin();busGroup!=busGroups.end();++busGroup)
        {
            if (busGroup->getName() == "dmis")
            {
                busGroup->getChildren(busses, "dmi");
                for (b=busses.begin();b!=busses.end();++b)
                {
                    DMIBus* bus = new DMIBus(*b, i_plug);
                    i_plug->busses().push_back(bus);
                    g_dmiBusses.push_back(bus);

                    if (b->findPath("source").getChildValue("connector-instance-id") != "")
                        bus->source().id(b->findPath("source").getChildValue("connector-instance-id"));
                    else
                        bus->source().id(b->findPath("source").getChildValue("part-instance-id"));

                    if (b->findPath("source").getChildValue("unit-name") != "")
                        bus->source().unit(b->findPath("source").getChildValue("unit-name"));

                    if (b->findPath("source").getChildValue("pin-name") != "")
                        bus->source().pin(b->findPath("source").getChildValue("pin-name"));

                    ////////////////////

                    if (b->findPath("endpoint").getChildValue("connector-instance-id") != "")
                        bus->endpoint().id(b->findPath("endpoint").getChildValue("connector-instance-id"));
                    else
                        bus->endpoint().id(b->findPath("endpoint").getChildValue("part-instance-id"));

                    if (b->findPath("endpoint").getChildValue("unit-name") != "")
                        bus->endpoint().unit(b->findPath("endpoint").getChildValue("unit-name"));

                    if (b->findPath("endpoint").getChildValue("pin-name") != "")
                        bus->endpoint().pin(b->findPath("endpoint").getChildValue("pin-name"));

                    swap = b->findPath("downstream_n_p_lane_swap_mask");
                    if (!swap.empty())
                        bus->downstreamSwap(swap.getValue());

                    swap = b->findPath("upstream_n_p_lane_swap_mask");
                    if (!swap.empty())
                        bus->upstreamSwap(swap.getValue());

                    swap = b->findPath("rx-msb-lsb-swap");
                    if (!swap.empty())
                        bus->rxSwap(swap.getValue());

                    swap = b->findPath("tx-msb-lsb-swap");
                    if (!swap.empty())
                        bus->txSwap(swap.getValue());

                    swap = b->findPath("mcs-refclock-enable-mapping");
                    if (!swap.empty())
                        bus->refclockMap(swap.getValue());

                }
            }
        }
    }

    vector<Plug*>::iterator plug;
    for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
    {
        makeDMIBusses(*plug);
    }

}

/**
 * High level function caller of makeDMIBusses() on the plug level.
 */
void makeDMIBusses()
{
    vector<Plug*>::iterator plug;

    for (plug=g_plugs.begin();plug!=g_plugs.end();++plug)
    {
        makeDMIBusses(*plug);
    }
}


/**
 * Walks a DMI Bus out from the source MCS to the endpoint centaur and creates a
 * DMISystemBus object for the path
 */
void walkDMIBus(DMIBus* i_sourceBus, endpointType i_type, vector<DMISystemBus*> & o_busses)
{
    DMIBus* bus = i_sourceBus;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    endpointType type = (i_type == SOURCE) ? ENDPOINT : SOURCE;
    bool done = false;
    int upSwap, downSwap, rxSwap, txSwap, refclockMap;
    rxSwap = txSwap = 0;


    upSwap = i_sourceBus->upstreamSwap();
    downSwap = i_sourceBus->downstreamSwap();
    if (i_sourceBus->rxSwap()) rxSwap++;
    if (i_sourceBus->txSwap()) txSwap++;
    refclockMap = i_sourceBus->refclockMap();


    //only need to walk busses if there's more than 1 segment
    if (endpoint->unit().empty())
    {
        do
        {
            bus = mrwGetNextBus<DMIBus>(bus, type);

            if (!bus) break;


            //Keep a running XOR of the swapped lanes
            upSwap ^= bus->upstreamSwap();
            downSwap ^= bus->downstreamSwap();

            //Keep a running count of the rx/tx swaps along the
            //bus to know if the final result is swapped
            if (bus->rxSwap()) rxSwap++;
            if (bus->txSwap()) txSwap++;

            //Keep track of the refclock mapping
            if (bus->refclockMap() != 0)
            {
                if (refclockMap != 0)
                {
                    if (refclockMap != bus->refclockMap())
                    {
                        mrwLogger l;
                        l() << "Found another nonzero mcs-refclock-mapping value " << bus->refclockMap()
                            << " after " << refclockMap << " already found on DMI bus that starts at "
                            << mrwUnitPath(i_sourceBus->plug(), *source);
                        l.error();
                    }
                }
                else
                    refclockMap = bus->refclockMap();
            }


            if (type == SOURCE)
                done = !bus->source().unit().empty();
            else
                done = !bus->endpoint().unit().empty();

        } while (!done);
    }


    if (!bus)
    {
        ostringstream msg;
        msg << "Couldn't complete a DMI bus that started at " <<
                i_sourceBus->plug()->path() << "/" << i_sourceBus->source().id() << "/" << i_sourceBus->source().unit();
        mrwError(msg.str());

        return;
    }


    DMIMaster* master = new DMIMaster(*source, i_sourceBus->plug());
    DMISlave* slave;

    if (type == SOURCE)
        slave = new DMISlave(bus->source(), bus->plug());
    else
        slave = new DMISlave(bus->endpoint(), bus->plug());

    if (mrwLogger::getDebugMode())
    {
        string m = "Creating DMI slave: " + mrwUnitPath(slave->plug(), slave->source());
        mrwLogger::debug(m);
    }

    DMISystemBus* systemBus = new DMISystemBus(master, slave, upSwap, downSwap,
                                               (rxSwap % 2) != 0, (txSwap % 2) != 0, refclockMap);


    o_busses.push_back(systemBus);

}



/**
 * Finds every DMIBus segment that starts at a CPU and walks it out
 * to the destination.
 */
void processDMIBusses(vector<DMISystemBus*> & o_busses)
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

            walkDMIBus(dmi, SOURCE, o_busses);
        }
        else if (mrwGetPartType(dmi->plug(), dmi->endpoint().id()) == "cpu")
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out DMI source " + mrwUnitPath(dmi->plug(), dmi->endpoint());
                mrwLogger::debug(m);
            }

            walkDMIBus(dmi, ENDPOINT, o_busses);
        }
    }

}


/**
 * Creats the DMISystemBus objects for the system, from the source MCSs to the
 * endpoint Centaurs
 */
void mrwDMIMakeBusses(vector<DMISystemBus*> & o_busses)
{
    mrwMakePlugs();

    makeDMIBusses();

    processDMIBusses(o_busses);
}

