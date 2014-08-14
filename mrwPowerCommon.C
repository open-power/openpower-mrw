/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPowerCommon.C $                                            */
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
#include <algorithm>
#include <mrwPowerCommon.H>

using namespace std;


//Globals
vector<Wire*> g_wires;

//Globals defined by mrwParserCommon
extern vector<Plug*> g_plugs;

int g_missingPartInstances = 0;


/**
 * Returns the port, if the source has one specified.
 * Otherwise returns 0
 */
int PowerSource::getPort()
{
    if (iv_port == -1)
    {
        vector<XMLElement> unitGroups;
        vector<XMLElement>::iterator ug;
        vector<XMLElement> units;
        vector<XMLElement>::iterator u;

        //will have to look through all the units on this part
        iv_part.findPath("units").getChildren(unitGroups);

        for (ug=unitGroups.begin();ug!=unitGroups.end();++ug)
        {
            units.clear();
            ug->getChildren(units);

            for (u=units.begin();u!=units.end();++u)
            {
                if (u->getChildValue("id") == iv_source.unit())
                {
                    string port = u->getChildValue("port");
                    if (!port.empty())
                        iv_port = atoi(port.c_str());
                    else
                        iv_port = 0;



                    return iv_port;
                }
            }
        }
    }

    return iv_port;
}



/**
 * Find the wires that have the instance/unit/plug as a source or endpoint
 */
void getWires(const string & i_instanceId, const string & i_unitId,
              Plug* i_plug, vector<busAndType_t<Wire> > & o_wires)
{
    vector<Wire*>::iterator w;
    o_wires.clear();

    for (w=g_wires.begin();w!=g_wires.end();++w)
    {
        if (i_plug == (*w)->plug())
        {
            if ((i_instanceId == (*w)->source().id()) && (i_unitId == (*w)->source().unit()))
            {
                o_wires.push_back(busAndType_t<Wire>(*w, SOURCE));
            }
            else if ((i_instanceId == (*w)->endpoint().id()) && (i_unitId == (*w)->endpoint().unit()))
            {
                o_wires.push_back(busAndType_t<Wire>(*w, ENDPOINT));
            }
        }
    }

}




/**
 * For a power passthrough device, like a pcie hotplug controller, the power-input-units have a
 * port on them that passes power through to a power-output-unit with the matching port.  This
 * function will return the power-output-unit that has the same port as the power-input-unit
 * passed in, or will return an empty object if this isn't that type of device.
 */
XMLElement getPassThroughPowerOutputUnit(Endpoint & i_endpoint, Plug* i_plug)
{
    XMLElement output;
    XMLElement unit = mrwGetUnit(i_endpoint.id(), i_endpoint.unit(), "power-input-unit", i_plug);

    if (!unit.empty())
    {
        string port = unit.getChildValue("port");

        if (!port.empty())
        {
            string partId = mrwGetPartId(i_plug->card(), i_endpoint.id());
            XMLElement part = mrwGetPart(partId);

            //try to get the power-output-unit with the same port
            output = part.findPath("units/power-output-units").find("power-output-unit", "port", port);

            //If it has an input, it better have an output
            if (output.empty())
            {
                mrwLogger logger;
                logger() << "Couldn't find power-output-unit for port " << port << " for part " << partId;
                logger.error(true);
            }
        }
    }

    return output;
}


/**
 * Says if this endpoint has a power-output unit that we'll use as a PowerSource
 */
bool isOutputUnit(Endpoint & i_endpoint, Plug* i_plug)
{
    string partId = mrwGetPartId(i_plug->card(), i_endpoint.id());


    if (partId.empty())
    {
        g_missingPartInstances++;
        mrwLogger l;
        l() << "Missing part ID for " << i_plug->path() << "/" << i_endpoint.id();
        l.error(false); //eventually should be an error
        return false;
    }

    if (partId != "POWER_TERMINAL") //PTs don't count
    {

        XMLElement part = mrwGetPart(partId);

        if (!part.empty())
        {
            XMLElement unit = part.findPath("units/power-output-units").find("power-output-unit", "id", i_endpoint.unit());
            if (!unit.empty())
                return true;
        }

    }

    return false;
}


/**
 * Says if this endpoint is a power-terminal
 */
bool isPowerTerminal(Endpoint & i_endpoint, Plug* i_plug)
{
    string partId = mrwGetPartId(i_plug->card(), i_endpoint.id());

    //also check for an empty partId - a serverwiz problem with part-instances missing for PTs
    return (partId.empty() || (partId == "POWER_TERMINAL"));
}



/**
 * Walks a power wire through a system and creates a PowerConnection for it.
 *
 */
void mrwPowerWalkPowerWire(Wire* i_sourceWire, endpointType i_type,
                           PowerSource *& io_source,
                           vector<PowerConnection*> & o_connections)
{
    vector<busAndType_t<Wire> > wires;
    vector<busAndType_t<Wire> >::iterator w;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceWire->source() : &i_sourceWire->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceWire->endpoint() : &i_sourceWire->source();
    XMLElement outputUnit;
    bool isPt;


    if (!source->unit().empty())
    {

        if (io_source != NULL)
        {
            mrwLogger l;
            l() << " PowerSource " << mrwUnitPath(io_source->plug(), io_source->source()) <<
                            " already found, but found another source "
                << mrwUnitPath(i_sourceWire->plug(), *source);
            l.error(true);
            return;
        }

        io_source = new PowerSource(*source, i_sourceWire->plug());
    }


    if (!endpoint->unit().empty())
    {
        //Need to know if it's a power terminal we can bypass.
        //Note: Power terminals have one unit - VOUT - that everything connects to.
        isPt = isPowerTerminal(*endpoint, i_sourceWire->plug());

        if (!isPt)
        {
            //Check if this device is just a passthrough we can bypass.
            //Actually, lets not bypass these, we'll treat it as a separate path.
            //outputUnit = getPassThroughPowerOutputUnit(*endpoint, i_sourceWire->plug());
        }
        else if (mrwLogger::getDebugMode())
        {
            string m = "Found power terminal " + mrwUnitPath(i_sourceWire->plug(), *endpoint);
            mrwLogger::debug(m);
        }


        if (!outputUnit.empty() || (isPt))
        {
            vector<busAndType_t<Wire> > newWires;
            vector<busAndType_t<Wire> >::iterator nw;
            PowerSink* sink = NULL;

            //it's a passthrough, get the wires with this as an endpoint
            //and keep walking those out to the end device
            string u = (!outputUnit.empty()) ? outputUnit.getChildValue("id") : endpoint->unit();

            //get the wires the have this plug/instance/unit as an endpoint
            getWires(endpoint->id(), u, i_sourceWire->plug(), newWires);

            if (mrwLogger::getDebugMode())
            {
                ostringstream m;
                m << "Found " << newWires.size() << " wires off of the power-terminal";
                mrwLogger::debug(m.str());
            }

            //walk out each of these wires
            for (nw=newWires.begin();nw!=newWires.end();++nw)
            {
                wires.clear();
                sink = NULL;

                if (nw->type == SOURCE)
                {
                    if (nw->bus->endpoint().id() != source->id()) //not this wire
                    {
                        if (nw->bus->endpoint().unit().empty())
                        {
                            mrwGetNextBusses<Wire>(i_sourceWire->plug(), nw->bus->endpoint(), wires);
                        }
                        else
                        {
                            //found an endpoint, don't need to get next busses
                            sink = new PowerSink(nw->bus->endpoint(), nw->bus->plug());
                        }
                    }
                }
                else
                {
                    if (nw->bus->source().id() != source->id()) //not this wire
                    {
                        if (nw->bus->source().unit().empty())
                        {
                            mrwGetNextBusses<Wire>(i_sourceWire->plug(), nw->bus->source(), wires);
                        }
                        else
                        {
                            //found an endpoint, don't need to get next busses
                            sink = new PowerSink(nw->bus->source(), nw->bus->plug());
                        }
                    }
                }

                if (sink)
                {
                    PowerConnection* c = new PowerConnection(io_source, sink);
                    o_connections.push_back(c);

                    if (mrwLogger::getDebugMode())
                    {
                        string m = "Creating a new PowerSink " + mrwUnitPath(sink->plug(), sink->source());
                        mrwLogger::debug(m);
                    }
                }
                else
                {
                    //need to keep walking
                    for (w=wires.begin();w!=wires.end();++w)
                    {
                        //walk the next part of the domain
                        mrwPowerWalkPowerWire(w->bus, w->type,
                                              io_source,
                                              o_connections);
                    }
                }

            }

        }
        else
        {
            //We reached the sink
            PowerSink* sink = new PowerSink(*endpoint, i_sourceWire->plug());
            PowerConnection* c = new PowerConnection(io_source, sink);
            o_connections.push_back(c);

            if (mrwLogger::getDebugMode())
            {
                string m = "Creating a new PowerSink " + mrwUnitPath(sink->plug(), sink->source());
                mrwLogger::debug(m);
            }
        }

    }
    else
    {
        //Walk out the wire to the end devices.
        //i_sourceWire may be connected to multiple wires on the other side
        //so need to get back all of them.

        mrwGetNextBusses<Wire>(i_sourceWire->plug(), *endpoint, wires);


        for (w=wires.begin();w!=wires.end();++w)
        {
            //walk the next part of the domain
            mrwPowerWalkPowerWire(w->bus, w->type,
                                  io_source,
                                  o_connections);
        }

    }


}



/**
 * Walk out the source wires and create the PowerConnection objects.
 */
void makePowerConnections(vector<PowerConnection*> & o_connections)
{
    vector<Wire*>::iterator wire;
    int count = 0;

    for (wire=g_wires.begin();wire!=g_wires.end();++wire)
    {
        if ((!(*wire)->source().unit().empty()) &&  isOutputUnit((*wire)->source(), (*wire)->plug()))
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out power wire that starts at " + mrwUnitPath((*wire)->plug(), (*wire)->source());
                mrwLogger::debug(m);
                count++;
            }
            PowerSource* s = NULL;
            mrwPowerWalkPowerWire(*wire, SOURCE, s, o_connections);
        }

        else if ((!(*wire)->endpoint().unit().empty()) && isOutputUnit((*wire)->endpoint(), (*wire)->plug()))
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out power wire that starts at " + mrwUnitPath((*wire)->plug(), (*wire)->endpoint());
                mrwLogger::debug(m);
                count++;
            }

            PowerSource* s = NULL;
            mrwPowerWalkPowerWire(*wire, ENDPOINT, s, o_connections);
        }
    }

    if (mrwLogger::getDebugMode())
    {
        ostringstream m;
        m << "Walked out " << count << " total wires";
        mrwLogger::debug(m.str());
    }
}



/**
 * Creates all of the PowerConnection objects in the system
 */
void mrwPowerMakeConnections(vector<PowerConnection*> & o_connections)
{
    mrwMakePlugs();

    mrwMakeBusses<Wire>(g_plugs, "power", g_wires);

    if (mrwLogger::getDebugMode())
    {
        ostringstream m;
        m << "Found " << g_wires.size() << " total power wire segments";
        mrwLogger::debug(m.str());
    }

    makePowerConnections(o_connections);

    if (mrwLogger::getDebugMode())
    {
        ostringstream m;
        m << "Found " << o_connections.size() << " total power connections";
        mrwLogger::debug(m.str());
    }

    //after serverwiz fixes, shouldn't have to worry about this anymore
    if (g_missingPartInstances != 0)
        cerr << "Found " << g_missingPartInstances << " missing part instances\n";

}


