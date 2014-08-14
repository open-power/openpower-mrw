/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPCIECommon.C $                                             */
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
#include <mrwPCIECommon.H>


using namespace std;


//Globals
vector<PCIEBus*> g_pcieBusses;

//Globals defined by mrwParserCommon
extern vector<Plug*> g_plugs;


/*
 *  The 'lane swap bits' field is a 3 bit field with the following definition:
 *      XXX
 *      |||---- 1 = lanes 8:15 are swapped to 15:8
 *      ||----- 1 = lanes 0:7 are swapped to 7:0
 *      |------ 1 = lanes 0:15 are swapped to 15:0
 *
 * The 'lane reversal bits' field is the inverse of the lane swap field for the
 * associated bit.
 *
 *   link-type    swap    reverse
 *   -----------------------------
 *   x16          000     100
 *   x16          100     000
 *   x8 0:7       000     01-    "-" means "don't touch"
 *   x8 0:7       010     00-
 *   x8 8:15      000     0-1
 *   x8 8:15      001     0-0
 *
 */

string PCIESystemBus::getBifurcatedLaneSwap(linkNum_t i_link)
{
    string swap;
    string x16Swap = iv_attributes.swapBits;

    //only valid on an x16 link
    if (iv_attributes.width != "16")
    {
        mrwLogger l;
        l() << "Invalid to call getBifurcatedLaneSwap on an x8 link.  Root is " << iv_root->getUnitPath();
        l.error(true);
    }

    if (x16Swap == "000")
    {
        //nothing swapped for either x8 link
        swap = "000";
    }
    else if (x16Swap == "100")
    {
        //since all x16 lanes were swapped, now both x8 links are
        swap = "011";
    }
    else
    {
        //I don't think we support anything else
        mrwLogger l;
        l() << " Invalid lane swap value " << x16Swap << " for x16 link that starts at " << iv_root->getUnitPath();
        l.error(true);
    }

    return swap;
}


string PCIESystemBus::getBifurcatedLaneReversal(linkNum_t i_link)
{
    string reverse;
    string x16Reverse = iv_attributes.reversalBits;

    //only valid on an x16 link
    if (iv_attributes.width != "16")
    {
        mrwLogger l;
        l() << "Invalid to call getBifurcatedLaneReversal on an x8 link.  Root is " << iv_root->getUnitPath();
        l.error(true);
    }

    if (x16Reverse == "000")
    {
        //the x16 wasn't reversed, so neither is either x8 link
        reverse = "000";
    }
    else if (x16Reverse == "100")
    {
        //The x16 link was reversed, so so are both x8 links
        //I think I can just return 011, and not 01- or 00-
        reverse = "011";
    }
    else
    {
        //I don't think we support anything else
        mrwLogger l;
        l() << " Invalid lane reversal value " << x16Reverse << " for x16 link that starts at " << iv_root->getUnitPath();
        l.error(true);
    }


    return reverse;
}



/**
 * Creates all the PowerBus objects that are on the card of the Plug passed in
 */
void makePCIEBusses(Plug* i_plug)
{
    vector<XMLElement> busGroups;
    vector<XMLElement> busses;
    vector<XMLElement>::iterator busGroup;
    vector<XMLElement>::iterator b;
    pcieAttributes_t a;

    if (i_plug->type() == Plug::CARD)
    {
        i_plug->card().findPath("busses").getChildren(busGroups);

        for (busGroup=busGroups.begin();busGroup!=busGroups.end();++busGroup)
        {
            if (busGroup->getName() == "pcies")
            {
                busGroup->getChildren(busses, "pcie");
                for (b=busses.begin();b!=busses.end();++b)
                {

                    PCIEBus* bus = new PCIEBus(*b, i_plug);
                    i_plug->busses().push_back(bus);
                    g_pcieBusses.push_back(bus);

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

                    /////////////////////

                    a.card_size = b->getChildValue("pci-card-size");
                    a.slot_index = b->getChildValue("pci-slot-index");
                    a.capi = b->getChildValue("pcie-capi");
                    a.lsi = b->getChildValue("pci-lsi");
                    a.gen = b->getChildValue("pci-gen");
                    a.hot_plug = b->getChildValue("pcie-hot-plug");
                    a.is_slot = b->getChildValue("pci-is-slot");
                    a.width = b->getChildValue("pci-width");
                    a.swapBits = b->getChildValue("pcie-lane-swap-bits");
                    a.reversalBits = b->getChildValue("pcie-lane-reversal-bits");
                    a.dsmp = b->getChildValue("pci-dsmp-slot");
                    a.card = i_plug->id();
                    a.hxSelect = b->getChildValue("hx-lane-set-select");
                    a.default_pcie_cooling_type = b->getChildValue("default-pcie-cooling-type");
                    XMLElement dpc = b->getChild("default-power-consumption");
                    if (dpc.getAttributeValue("power-units") == "deciWatts")
                    {
                    	// Convert to "Watts"
                    	char buf[100];
                    	float deci_watts;
                    	double watts;
                    	sscanf(dpc.getValue().c_str(), "%f", &deci_watts);
                    	watts = deci_watts / 10.0;
                    	sprintf(buf, "%.1f", watts);
                    	a.default_power_consumption = buf;
                    }
                    else
                    {
                    	// Assume "Watts" because it passed schema validation
                    	a.default_power_consumption = dpc.getValue();
                    }
                    bus->attributes(a);
                }
            }
        }
    }

    vector<Plug*>::iterator plug;
    for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
    {
        makePCIEBusses(*plug);
    }
}


/**
 * High level function caller of makeBusses() on the plug level.
 */
void makePCIEBusses()
{
    vector<Plug*>::iterator plug;

    for (plug=g_plugs.begin();plug!=g_plugs.end();++plug)
    {
        makePCIEBusses(*plug);
    }
}



/**
 * Says if the instance and units passed in make up a pcie root unit
 */
bool pcieRootUnit(const string & i_instanceId, const string & i_unitName, Plug* i_plug)
{
    string partId = mrwGetPartId(i_plug->card(), i_instanceId);
    XMLElement unit = mrwGetUnit(partId, "pcie-root-unit", i_unitName);

    if (!unit.empty())
    {
        return true;
    }

    return false;
}



/**
 * Checks if the unit on the source of the bus passed in is a PCIE
 * root unit on the CPU (not a switch/repeater)
 */
bool cpuPcieRootUnit(Endpoint & i_source, Plug* i_plug)
{
    if (!i_source.unit().empty())
    {
        string partType = mrwGetPartType(i_plug->card(), i_source.id());

        if (partType == "cpu")
        {
            if (pcieRootUnit(i_source.id(), i_source.unit(), i_plug))
            {
                return true;
            }

        }
    }

    return false;
}



/**
 * Finds the PCIEBus objects that have the id, unit, and plug passed in as either a source or endpoint,
 */
void getPCIEBusses(const string & i_instanceId, const string & i_unitId,
                   Plug* i_plug, vector<busAndType_t<PCIEBus> > & o_busses)
{
    vector<PCIEBus*>::iterator b;

    o_busses.clear();

    for (b=g_pcieBusses.begin();b!=g_pcieBusses.end();++b)
    {
        if ((*b)->plug() == i_plug)
        {
            if (((*b)->source().id() == i_instanceId) && ((*b)->source().unit() == i_unitId))
            {
                o_busses.push_back(busAndType_t<PCIEBus>(*b, SOURCE));
            }
            else if (((*b)->endpoint().id() == i_instanceId) && ((*b)->endpoint().unit() == i_unitId))
            {
                o_busses.push_back(busAndType_t<PCIEBus>(*b, ENDPOINT));
            }
        }
    }

}




/**
 * Recursive function to walk a PCIE bus out from a switch to the end device and create
 * new  PCIESystemBus with i_root, i_switch, and the destinations it finds off of this switch.
 */
void walkBusFromSwitch(PCIERoot* i_root, PCIESwitch* i_switch,
                       PCIEBus* i_sourceBus, endpointType i_type,
                       pcieAttributes_t & i_attributes,
                       pciePathEntries_t & i_paths,
                       vector<PCIESystemBus*> & o_busses)
{
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    vector<busAndType_t<PCIEBus> > busses;
    vector<busAndType_t<PCIEBus> >::iterator b;
    pcieAttributes_t attributes = i_sourceBus->attributes();
    pciePathEntries_t paths = i_paths;


    //Get the bus attributes, should just be set once along the path
    if (!attributes.empty())
    {
        if (!i_attributes.empty())
        {
            mrwLogger logger;
            logger() << "The PCIE bus  that starts at switch "
                     << i_switch->getChipPath() << "/" << i_switch->unitId()
                     << " has attributes defined on multiple cards: "
                     << i_attributes.card << " and " << attributes.card << endl;
            logger.error(true);
        }
    }
    else
        attributes = i_attributes;

    paths.push_back(pciePathEntry_t(i_sourceBus->plug()));

    //we found the end
    if (!endpoint->unit().empty())
    {
        //Create the system bus with the root and switch passed in, and this end device

        PCIESystemEndpoint* dest = new PCIESystemEndpoint(*endpoint, i_sourceBus->plug());
        paths.push_back(pciePathEntry_t(dest->plug(), dest->getChipPath()));

        PCIESystemBus* systemBus = new PCIESystemBus(i_root, i_switch, dest, attributes, paths);
        o_busses.push_back(systemBus);
    }
    else
    {
        //Find the next set of PCIEBusses that are connected to this one
        mrwGetNextBusses<PCIEBus>(i_sourceBus->plug(), *endpoint, busses);

        for (b=busses.begin();b!=busses.end();++b)
        {
            //walk the next part of the bus
            walkBusFromSwitch(i_root, i_switch, b->bus, b->type, attributes, paths, o_busses);
        }
    }

}



/**
 * If i_systemBus->destination is a switch, will walk it out to the end devices
 * and create new PCIESystemBus that have the root, switch, and end device
 */
void walkSwitch(PCIESystemBus* i_systemBus, vector<PCIESystemBus*> & o_busses)
{
    PCIESystemEndpoint* switchChip = i_systemBus->destination();
    vector<XMLElement> units;
    vector<XMLElement>::iterator unit;
    vector<busAndType_t<PCIEBus> > busses;
    vector<busAndType_t<PCIEBus> >::iterator b;
    XMLElement upstreamUnit;


    //Get any root units on the endpoint, on switches
    //they're downstream bridge units
    mrwGetUnits(mrwGetPartId(switchChip->plug()->card(), switchChip->source().id()),
                "pcie-downstream-bridge", units);


    //walk out each downstream unit to the end devices
    for (unit=units.begin();unit!=units.end();++unit)
    {

        //Get the busses that have a source at this switch pci-root unit
        //could be multiple if multiple cards in the same slot
        getPCIEBusses(switchChip->source().id(), unit->getChildValue("id"), switchChip->plug(), busses);

        if (!busses.empty())
        {
            if (upstreamUnit.empty())
            {
                upstreamUnit = mrwGetUnit(switchChip->source().id(),
                                          switchChip->source().unit(),
                                          "pcie-upstream-bridge",
                                          switchChip->plug());
            }



            PCIESwitch* sw = new PCIESwitch(switchChip->source().id(),
                                            unit->getChildValue("id"),
                                            switchChip->plug(),
                                            upstreamUnit,
                                            unit->getChildValue("width"),
                                            unit->getChildValue("station"),
                                            unit->getChildValue("port"));

            if (mrwLogger::getDebugMode())
            {
                string m = "Found PCIE Switch " + sw->plug()->path() + "/" + sw->id() + "/" + sw->unitId();
                mrwLogger::debug(m);
            }


            //walk those busses out from the switch to the end device
            for (b=busses.begin();b!=busses.end();++b)
            {
                pcieAttributes_t a;
                walkBusFromSwitch(i_systemBus->root(), sw,
                                  b->bus, b->type, a,
                                  i_systemBus->getPath(), o_busses);
            }
        }

    }


}

/**
 * Walks a GPIO bus from source unit to end unit.  Creates a GPIOSystemBus object for every full bus path it
 * finds and adds them to o_gpioBusses.  Recursive because to handle the case where multiple cards can plug
 * into the same slot.
 */
void walkPCIEBus(PCIEBus* i_sourceBus,
                endpointType i_type,
                PCIERoot *& i_root,
                pcieAttributes_t & i_attributes,
                pciePathEntries_t & i_paths,
                vector<PCIESystemBus*> & o_busses)
{
    vector<busAndType_t<PCIEBus> > busses;
    vector<busAndType_t<PCIEBus> >::iterator b;
    PCIERoot* root = NULL;
    Endpoint* source = (i_type == SOURCE) ? &i_sourceBus->source() : &i_sourceBus->endpoint();
    Endpoint* endpoint = (i_type == SOURCE) ? &i_sourceBus->endpoint() : &i_sourceBus->source();
    pcieAttributes_t attributes = i_sourceBus->attributes();
    pciePathEntries_t paths = i_paths;


    //If the source has a unit, then create the source of the system bus
    if (!source->unit().empty())
    {
        root = new PCIERoot(*source, i_sourceBus->plug());
        paths.push_back(pciePathEntry_t(root->plug(), root->getChipPath()));
    }

    //Get the bus attributes, should only be set once along the bus
    if (!attributes.empty())
    {
        if (!i_attributes.empty())
        {
            mrwLogger logger;
            logger() << "The PCIE bus  that starts at "
                     << i_sourceBus->plug()->path() << "/" << source->id() << "/" << source->unit()
                     << " has attributes defined on multiple cards: "
                     << i_attributes.card << " and " << attributes.card << endl;
            logger.error(true);
        }
    }
    else
        attributes = i_attributes;

    paths.push_back(pciePathEntry_t(i_sourceBus->plug()));

    //we found the end
    if (!endpoint->unit().empty())
    {
        PCIESystemEndpoint* dest = new PCIESystemEndpoint(*endpoint, i_sourceBus->plug());

        paths.push_back(pciePathEntry_t(dest->plug(), dest->getChipPath()));

        PCIESystemBus* systemBus = new PCIESystemBus(i_root ? i_root : root, dest, attributes, paths);
        o_busses.push_back(systemBus);

        if (mrwLogger::getDebugMode())
        {
            string m = "Creating PCIE Endpoint " + mrwUnitPath(dest->plug(), dest->source());
            mrwLogger::debug(m);
        }

        //It could be a switch, so try to keep walking the busses
        //out past the switch to the real end devices.
        walkSwitch(systemBus, o_busses);
    }
    else
    {
        //i_sourceBus may be connected to multiple busses on the other side
        //so need to get back all of them.  In this case when we do reach
        //the end devices they will all have the same master.

        mrwGetNextBusses<PCIEBus>(i_sourceBus->plug(), *endpoint, busses);

        for (b=busses.begin();b!=busses.end();++b)
        {
            //walk the next part of the bus
            walkPCIEBus(b->bus, b->type, i_root ? i_root : root, attributes, paths, o_busses);
        }
    }

}


/**
 * Finds the busses with root units, and walks those busses out
 * to find the whole path.
 */
void processPCIEBusses(vector<PCIESystemBus*> & o_busses)
{
    vector<PCIEBus*>::iterator bus;

    for (bus=g_pcieBusses.begin();bus!=g_pcieBusses.end();++bus)
    {
        if (cpuPcieRootUnit((*bus)->source(), (*bus)->plug()))
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out PCIE Root " + mrwUnitPath((*bus)->plug(), (*bus)->source());
                mrwLogger::debug(m);
            }

            PCIERoot* root = NULL;
            pcieAttributes_t attributes;
            pciePathEntries_t paths;
            walkPCIEBus(*bus, SOURCE, root, attributes, paths, o_busses);
        }

        else if (cpuPcieRootUnit((*bus)->endpoint(), (*bus)->plug()))
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Walking out PCIE Root " + mrwUnitPath((*bus)->plug(), (*bus)->endpoint());
                mrwLogger::debug(m);
            }

            PCIERoot* root = NULL;
            pcieAttributes_t attributes;
            pciePathEntries_t paths;
            walkPCIEBus(*bus, ENDPOINT, root, attributes, paths, o_busses);
        }
    }
}


/**
 * Creates a list of all of the PCIE busses in the system.
 */
void mrwPCIEMakeBusses(vector<PCIESystemBus*> & o_busses)
{
    g_pcieBusses.clear();

    mrwMakePlugs();

    makePCIEBusses();

    processPCIEBusses(o_busses);
}


