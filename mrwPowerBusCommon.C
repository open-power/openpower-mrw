/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPowerBusCommon.C $                                         */
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
#include <iostream>
#include <sstream>
#include <algorithm>
#include <mrwPowerBusCommon.H>


using namespace std;


//Globals
vector<PowerBus*> g_busses;


//Globals defined by mrwParserCommon
extern vector<Plug*> g_plugs;

/**
 * Returns the instance path of the connector of the SMP cable on this bus endpoint's 
 * side.
 */
string PowerSystemBus::getCableConnPath(PowerSystemBus::bus_endpoint_type_t i_type)
{
    string path;


    //There's a hard way to do this, and an easy way.  Knowing that so far all
    //cables have been used to cross assemblies, the easy way is to just check for 
    //which end of the cable has an assembly that matches either the PB source's 
    //or destination's. The hard way is to keep track of the cable connector
    //instance in the bus path while walking the bus. We'll do the easy way.

    if (iv_cable)
    {
        string cableConnId, cableAssembly, endpointAssembly;
        PowerBusSystemEndpoint* endpoint;
        Plug* cablePlug = NULL;

        if (PB_SOURCE == i_type) endpoint = iv_source;
        else endpoint = iv_destination;
        
        endpointAssembly = endpoint->plug()->assembly().id + '-' + endpoint->plug()->assembly().position;

        //First check if the source side of the cable has endpoint's assembly
        cablePlug = iv_cable->sourcePlug();
        cableAssembly = cablePlug->assembly().id + '-' + cablePlug->assembly().position;

        if (endpointAssembly == cableAssembly)
        {
            cableConnId = iv_cable->element().getChildValue("source-connector-instance");
        }
        else
        {
            //Now try the target side
            cablePlug = iv_cable->targetPlug();
            cableAssembly = cablePlug->assembly().id + '-' + cablePlug->assembly().position;

            if (endpointAssembly == cableAssembly)
            {
                cableConnId = iv_cable->element().getChildValue("target-connector-instance");
            }
            else
            {
                mrwLogger l;
                l() << "Couldn't find a cable endpoint with same assembly as " 
                    << endpoint->getChipPath() << " for cable " << iv_cable->element().getChildValue("name");
                l.error();
            }
        }


        //Now build the connector instance path
        if (!cableConnId.empty())
        {

            //get the connector-instance, so we can build the instance path
            XMLElement inst = cablePlug->card().findPath("connector-instances").find("connector-instance", "id", cableConnId);
            if (!inst.empty())
            {
                string cid = inst.getChildValue("connector-id");
                string pos = inst.getChildValue("position");
                path = cablePlug->path() + "/" + cid + "-" + pos;
            }
            else
            {
                mrwLogger l;
                l() << "Couldn't find connector instance element for ID " << cableConnId << " on " << cablePlug->path();
                l.error();
            }
        }
    }


    return path;
}




/**
 * Creates all the PowerBus objects that are on the card of the Plug passed in
 */
void makePowerBusses(Plug* i_plug)
{
    vector<XMLElement> busGroups;
    vector<XMLElement> busses;
    vector<XMLElement>::iterator busGroup;
    vector<XMLElement>::iterator b;
    XMLElement swap, width;

    if (i_plug->type() == Plug::CARD)
    {
        i_plug->card().findPath("busses").getChildren(busGroups);

        for (busGroup=busGroups.begin();busGroup!=busGroups.end();++busGroup)
        {
            if (busGroup->getName() == "powerbusses")
            {
                busGroup->getChildren(busses, "powerbus");
                for (b=busses.begin();b!=busses.end();++b)
                {
                    PowerBus* bus = new PowerBus(*b, i_plug);
                    i_plug->busses().push_back(bus);
                    g_busses.push_back(bus);

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

                    swap = b->findPath("rx-msb-lsb-swap");
                    if (!swap.empty())
                        bus->rxSwap(swap.getValue());

                    swap = b->findPath("tx-msb-lsb-swap");
                    if (!swap.empty())
                        bus->txSwap(swap.getValue());

                    swap = b->findPath("downstream_n_p_lane_swap_mask");
                    if (!swap.empty())
                        bus->downstreamSwap(swap.getValue());

                    swap = b->findPath("upstream_n_p_lane_swap_mask");
                    if (!swap.empty())
                        bus->upstreamSwap(swap.getValue());

                    width = b->findPath("bus-width");
                    if (!width.empty())
                        bus->width(atoi(width.getValue().c_str()));



                }
            }
        }
    }

    vector<Plug*>::iterator plug;
    for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
    {
        makePowerBusses(*plug);
    }

}


/**
 * High level function caller of makePowerBusses() on the plug level.
 */
void makePowerBusses()
{
    vector<Plug*>::iterator plug;

    for (plug=g_plugs.begin();plug!=g_plugs.end();++plug)
    {
        makePowerBusses(*plug);
    }

}



/**
 * Checks if the source and plug are already in a PowerSystemBus source or destination
 */
bool isPowerBusProcessed(Endpoint& i_source,
                         Plug* i_plug,
                         const vector<PowerSystemBus*> i_systemBusses)
{
    vector<PowerSystemBus*>::const_iterator bus;


    for (bus=i_systemBusses.begin();bus!=i_systemBusses.end();++bus)
    {
        if ((i_source == (*bus)->source()->source()) && (i_plug == (*bus)->source()->plug()))
            return true;

        if ((i_source == (*bus)->destination()->source()) && (i_plug == (*bus)->destination()->plug()))
            return true;

    }

    return false;
}




/**
 * Takes a bus where the source is the source unit of a power bus connection, and
 * walks it out to find the destinations.
 */
void walkPowerBus(PowerBus* i_sourceBus,
                  endpointType i_type, //the end with the unit
                  vector<PowerSystemBus*> & o_systemBusses)
{
    PowerBus* bus = i_sourceBus;
    PowerBusDestination* dest = NULL;
    vector<string> segments;
    string connectorPath;
    Endpoint* source   = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    endpointType type = SOURCE;
    int width, rxSwap, txSwap, upSwap, downSwap;
    width = rxSwap = txSwap = upSwap = downSwap = 0;
    Cable* cable = NULL;


    //if we're on the other endpoint of a bus we already processed, we can skip it
    if (isPowerBusProcessed(*source, i_sourceBus->plug(), o_systemBusses))
    {
        if (mrwLogger::getDebugMode())
        {
            string m = "Already processed " + i_sourceBus->plug()->path() +
                       "/" + source->id() + "/" + source->unit();
            m += ", so skipping it this time";
            mrwLogger::debug(m);
        }

        return;
    }

    PowerBusSource* pbSource = new PowerBusSource(*source, i_sourceBus->plug());

    //the source chip is the beginning path segment
    segments.push_back(pbSource->getChipPath());

    //next in the segments is the card the source chip is on
    segments.push_back(i_sourceBus->plug()->path());

    //Get the swaps & width
    if (i_sourceBus->rxSwap()) rxSwap++;
    if (i_sourceBus->txSwap()) txSwap++;
    width  = i_sourceBus->width();
    upSwap = i_sourceBus->upstreamSwap();
    downSwap = i_sourceBus->downstreamSwap();


    //if just a 1 segment bus, then unit will be at the endpoint and we'll be done
    if (!endpoint->unit().empty())
    {
        dest = new PowerBusDestination(*endpoint, bus->plug());

        if (mrwLogger::getDebugMode())
        {
            string m = "Found endpoint " + mrwUnitPath(dest->plug(), dest->source());
            mrwLogger::debug(m);
        }
    }
    else
    {
        //if not a 1 bus powerbus, then have to walk through cards/plugs

        //the end we need to connect to the next bus
        type = (i_type == SOURCE) ? ENDPOINT : SOURCE;
        bool done = false;

        //add the connector to the bus path
        connectorPath = bus->plug()->path() + "/" + endpoint->id();
        segments.push_back(connectorPath);

        do
        {
            Cable* c = NULL;

            bus = mrwGetNextBus(bus, type, &c);

            //Note: This code currently just supports 1 cable in the path. 
            //That's never been a problem in a system yet.
            if (c)
            {
                if (cable != NULL)
                {
                    mrwLogger l;
                    l() << "More than one cable found in powerbus path for source " << pbSource->getChipPath();
                    l.error();
                }

                cable = c;
            }

            if (!bus) break;

            //Set/check the width on this segment
            if (width != 0)
            {
                if ((bus->width() != 0) && (width != bus->width()))
                {
                    mrwLogger l;
                    l() << "Different bus widths specified along powerbus segment that starts at " << pbSource->getUnitPath();
                    l.error(true);
                }
            }
            else
                width = bus->width();

            //Keep a running count of the swaps along the
            //bus to know if the final result is swapped
            if (bus->rxSwap()) rxSwap++;
            if (bus->txSwap()) txSwap++;

            //For the NP swaps, keeping a running XOR to tell
            //the endpoint's swapped lanes
            upSwap ^= bus->upstreamSwap();
            downSwap ^= bus->downstreamSwap();


            if (type == SOURCE)
            {

                if (!bus->source().unit().empty())
                {
                    done = true;
                }
                else
                {
                    //add the intermediate card and connector to the path
                    connectorPath = bus->plug()->path() + "/" + bus->endpoint().id();
                    segments.push_back(connectorPath);
                    segments.push_back(bus->plug()->path());
                    connectorPath = bus->plug()->path() + "/" + bus->source().id();
                    segments.push_back(connectorPath);
                }
            }
            else
            {
                if (!bus->endpoint().unit().empty())
                {
                    done = true;
                }
                else
                {
                    //add the intermediate card and connector to the path
                    connectorPath = bus->plug()->path() + "/" + bus->source().id();
                    segments.push_back(connectorPath);
                    segments.push_back(bus->plug()->path());
                    connectorPath = bus->plug()->path() + "/" + bus->endpoint().id();
                    segments.push_back(connectorPath);
                }
            }


        } while (!done);

        if (!bus)
        {
            string path = i_sourceBus->plug()->path() + "/" + source->id() + "/" + source->unit();
            ostringstream msg;
            msg << "No endpoint found for a powerbus path that starts at "
                << path << ".\n"  << "       It may not be wired.";
            mrwInfo(msg.str());
            return;
        }


        if (type == SOURCE)
            dest = new PowerBusDestination(bus->source(), bus->plug());
        else
            dest = new PowerBusDestination(bus->endpoint(), bus->plug());

        if (mrwLogger::getDebugMode())
        {
            string m = "Found endpoint " + mrwUnitPath(dest->plug(), dest->source());
            mrwLogger::debug(m);
        }

    }


    //save the destination's card to the system path segment list,
    //if not already there
    if (bus->plug()->path() != segments.back())
    {
        //save the connector, and then the card
        connectorPath = bus->plug()->path() + "/";
        connectorPath += (type == SOURCE) ? bus->endpoint().id() : bus->source().id();
        segments.push_back(connectorPath);
        segments.push_back(bus->plug()->path());
    }

    //Destination chip is the last path segment
    segments.push_back(dest->getChipPath());

    PowerSystemBus* systemBus = new PowerSystemBus(pbSource, dest, segments,
                                                   (rxSwap % 2) != 0, (txSwap % 2) != 0,
                                                   upSwap, downSwap,
                                                   width, cable);
    o_systemBusses.push_back(systemBus);

}





/**
 * Finds the busses with source units, and walks those busses out
 * to find the whole path.
 */
void processPowerBusses(vector<PowerSystemBus*> & o_busses)
{
    vector<PowerBus*>::iterator bus;

    for (bus=g_busses.begin();bus!=g_busses.end();++bus)
    {
        if (!(*bus)->source().unit().empty())
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out Power Bus " + mrwUnitPath((*bus)->plug(), (*bus)->source());
                mrwLogger::debug(m);
            }

            walkPowerBus(*bus, SOURCE, o_busses);
        }

        else if (!(*bus)->endpoint().unit().empty())
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out Power Bus " + mrwUnitPath((*bus)->plug(), (*bus)->endpoint());
                mrwLogger::debug(m);
            }

            walkPowerBus(*bus, ENDPOINT, o_busses);
        }

    }
}



/**
 * Creates a list of all of the PowerBus busses in the system which
 * connect the processors.
 */
void mrwPowerBusMakeBusses(vector<PowerSystemBus*> & o_busses)
{
    g_busses.clear();

    mrwMakePlugs();

    makePowerBusses();

    processPowerBusses(o_busses);

}

