/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPCIEParser.C $                                             */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwPCIEParser.C 1$

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <xmlutil.H>
#include <mrwPCIECommon.H>


/**
 * This program will walk the topology XML and build a separate XML file that just
 * contains information on the PCIE connections.
 */

using namespace std;


//globals
string g_targetFile;
FILE* g_fp = NULL;




void usage()
{
    cout << "Usage: mrwPCIEParser  --in <full system xml file> --out <output xml file> --targets <ecmd targets xml file> \n";
}


class PrintPCIEBus
{
public:
    PrintPCIEBus(const vector<PCIESystemBus*> & i_allBusses) :
        iv_all(i_allBusses) {}

    void operator()(PCIESystemBus* i_bus)
    {
        printPCIEBus(i_bus);
    }

    void printPCIEBus(PCIESystemBus* i_bus);
    void printSystemPath(PCIESystemBus* i_bus);
    int getLaneMask(PCIESystemBus* i_bus);
    int getSwitchWidth(PCIESystemBus* i_bus);

private:
    const vector<PCIESystemBus*> & iv_all;
};


/**
 * Returns the width of the bus that has i_bus's switch as an endpoint
 */
int PrintPCIEBus::getSwitchWidth(PCIESystemBus* i_bus)
{
    int width = 0;

    if (i_bus->switchChip())
    {
        vector<PCIESystemBus*>::const_iterator b;

        for (b=iv_all.begin();b!=iv_all.end();++b)
        {
            if ((i_bus->root()->getUnitPath() == (*b)->root()->getUnitPath()) &&
                (i_bus->switchChip()->getChipPath() == (*b)->destination()->getChipPath()))
            {
                width = atoi((*b)->attributes().width.c_str());
                break;
            }

        }
    }

    return width;
}



/**
 * Returns the PCIE configured lane mask:
 *  0xFFFF - lanes 0-15 configured
 *  0xFF00 - lanes 0-7  configured
 *  0x00FF - lanes 8-15 configured
 */
int PrintPCIEBus::getLaneMask(PCIESystemBus* i_bus)
{
    int mask = 0;
    string startingLane = i_bus->root()->unit().getChildValue("starting-lane");
    int width, lane, start;

    //if the bus has a switch, then need to get the width from the object that
    //has that as an endpoint
    if (i_bus->switchChip())
        width = getSwitchWidth(i_bus);
    else
        width = atoi(i_bus->attributes().width.c_str());


    if (width == 0)
    {
        mrwLogger l;
        l() << "found invalid pcie width " << width << "for PCIE root " << i_bus->root()->getChipPath();
        l.error(true);
    }

    if (startingLane.empty())
        lane = 0;
    else
        lane = atoi(startingLane.c_str());

    if ((lane + width) > 16)
    {
        mrwLogger l;
        l() << "Found invalid starting lane " << lane << " and/or PCIE width " << width
            << "for PCIE root " << i_bus->root()->getChipPath();
        l.error(true);
    }
    else
    {
        start = (0x8000 >> lane);

        for (int i=0;i<width;i++)
        {
            mask |= (start >> i);
        }
    }

    return mask;
}


struct pcieAlternate
{
    string root;
    string loc;

    pcieAlternate(const string & r, const string & l) :
        root(r), loc(l) {}
};


/**
 * Looks for a matching parent plug and destination location code for
 * a PCIE destination
 */
class FindAlternate
{
public:

    FindAlternate(PCIESystemBus* i_bus) : iv_bus(i_bus) {}

    bool operator()(const pcieAlternate & i_entry)
    {
        return ((iv_bus->root()->getUnitPath() == i_entry.root) &&
                (iv_bus->destination()->plug()->location() == i_entry.loc));
    }

private:
    PCIESystemBus* iv_bus;
};


/**
 * Prints the cards and chips in the path of this PCIE bus.
 */
void PrintPCIEBus::printSystemPath(PCIESystemBus* i_bus)
{
    pciePathEntries_t::iterator e;

    fprintf(g_fp, "%s<system-path>\n", mrwIndent(2));

    for (e=i_bus->getPath().begin();e!=i_bus->getPath().end();++e)
    {
        if (!e->part.empty())
        {
            fprintf(g_fp, "%s<chip><instance-path>%s</instance-path></chip>\n",
                    mrwIndent(3), e->part.c_str());
        }
        else
        {
            fprintf(g_fp, "%s<card><instance-path>%s</instance-path><location-code>%s</location-code></card>\n",
                    mrwIndent(3), e->plug->path().c_str(), e->plug->location().c_str());
        }

    }

    fprintf(g_fp, "%s</system-path>\n", mrwIndent(2));
}



/**
 * Prints a pcie bus entry in the PCIE xml
 */
void PrintPCIEBus::printPCIEBus(PCIESystemBus* i_bus)
{
    PCIERoot* root = i_bus->root();
    PCIESystemEndpoint* dest = i_bus->destination();
    PCIESwitch* sw = i_bus->switchChip();
    pcieAttributes_t & a = i_bus->attributes();
    static int s_count = 0;
    static vector<pcieAlternate> s_alternates;

    //if the endpoint has alternates, we only need to do 1 of them
    //so save the loc (and root, because across multi-nodes the locs could be the same)
    if (!dest->plug()->alternates().empty())
    {
        if (find_if(s_alternates.begin(), s_alternates.end(),
                    FindAlternate(i_bus)) != s_alternates.end())
        {
            if (mrwLogger::getDebugMode())
            {
                string m = "Skipping printing alternate endpoint " + dest->getChipPath();
                mrwLogger::debug(m);
            }

            return;
        }

        s_alternates.push_back(pcieAlternate(root->getUnitPath(), dest->plug()->location()));
    }


    string chipPath = root->getChipPath();
    ecmdTarget_t target = mrwGetTarget(chipPath, g_targetFile);

    string hxSelect = (a.hxSelect.empty()) ? "0" : a.hxSelect;

    ////////////////////////////////////////////////
    //Source

    fprintf(g_fp, "%s<pcie-bus>\n", mrwIndent(1));
    fprintf(g_fp, "%s<source>\n", mrwIndent(2));
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n", mrwIndent(3),
            target.name.c_str(), target.node, target.position);
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), target.location.c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), chipPath.c_str());
    fprintf(g_fp, "%s<unit-instance-path>%s</unit-instance-path>\n", mrwIndent(3), root->getUnitPath().c_str());
    fprintf(g_fp, "%s<iop>%d</iop>\n", mrwIndent(3), root->iop());
    fprintf(g_fp, "%s<lane-mask>0x%.2X</lane-mask>\n", mrwIndent(3), getLaneMask(i_bus));
    fprintf(g_fp, "%s<hx-lane-set-select>%s</hx-lane-set-select>\n", mrwIndent(3), hxSelect.c_str());

    if (target.name == "pu")
    {
        fprintf(g_fp, "%s<dsmp-capable>%s</dsmp-capable>\n", mrwIndent(3), a.dsmp.c_str());
        fprintf(g_fp, "%s<lane-swap-bits>0b%s</lane-swap-bits>\n", mrwIndent(3), a.swapBits.c_str());
        fprintf(g_fp, "%s<lane-reversal-bits>0b%s</lane-reversal-bits>\n", mrwIndent(3), a.reversalBits.c_str());


        //if a x16 link, also need to display the swap/reversal bits for the
        //bifucated x8 links. but only if the endpoint isn't a switch.
        if ((a.width == "16") && (dest->partType() != "pcie-switch"))
        {
            fprintf(g_fp, "%s<bifurcation-settings>\n", mrwIndent(3));

            //the first half
            fprintf(g_fp, "%s<bifurcation-setting>\n", mrwIndent(4));
            fprintf(g_fp, "%s<lane-mask>%s</lane-mask>\n", mrwIndent(5), i_bus->getBifurcatedLaneMask(PCIESystemBus::ZERO).c_str());
            fprintf(g_fp, "%s<lane-swap-bits>0b%s</lane-swap-bits>\n", mrwIndent(5), i_bus->getBifurcatedLaneSwap(PCIESystemBus::ZERO).c_str());
            fprintf(g_fp, "%s<lane-reversal-bits>0b%s</lane-reversal-bits>\n", mrwIndent(5), i_bus->getBifurcatedLaneReversal(PCIESystemBus::ZERO).c_str());
            fprintf(g_fp, "%s</bifurcation-setting>\n", mrwIndent(4));

            //the second half
            fprintf(g_fp, "%s<bifurcation-setting>\n", mrwIndent(4));
            fprintf(g_fp, "%s<lane-mask>%s</lane-mask>\n", mrwIndent(5), i_bus->getBifurcatedLaneMask(PCIESystemBus::ONE).c_str());
            fprintf(g_fp, "%s<lane-swap-bits>0b%s</lane-swap-bits>\n", mrwIndent(5), i_bus->getBifurcatedLaneSwap(PCIESystemBus::ONE).c_str());
            fprintf(g_fp, "%s<lane-reversal-bits>0b%s</lane-reversal-bits>\n", mrwIndent(5), i_bus->getBifurcatedLaneReversal(PCIESystemBus::ONE).c_str());
            fprintf(g_fp, "%s</bifurcation-setting>\n", mrwIndent(4));

            fprintf(g_fp, "%s</bifurcation-settings>\n", mrwIndent(3));
        }

    }

    fprintf(g_fp, "%s</source>\n", mrwIndent(2));



    ////////////////////////////////////////////////
    //Switch

    if (sw)
    {
        chipPath = sw->getChipPath();

        fprintf(g_fp, "%s<switch>\n", mrwIndent(2));
        fprintf(g_fp, "%s<part-id>%s</part-id>\n", mrwIndent(3), sw->partId().c_str());
        fprintf(g_fp, "%s<width>%s</width>\n", mrwIndent(3), sw->width().c_str());
        fprintf(g_fp, "%s<station>%s</station>\n", mrwIndent(3), sw->station().c_str());
        fprintf(g_fp, "%s<port>%s</port>\n", mrwIndent(3), sw->port().c_str());
        fprintf(g_fp, "%s<upstream-station>%s</upstream-station>\n", mrwIndent(3), sw->upstreamUnit().getChildValue("station").c_str());
        fprintf(g_fp, "%s<upstream-port>%s</upstream-port>\n", mrwIndent(3), sw->upstreamUnit().getChildValue("port").c_str());
        fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), chipPath.c_str());
        fprintf(g_fp, "%s</switch>\n", mrwIndent(2));
    }


    ////////////////////////////////////////////////
    //Endpoint

    string partLoc = mrwGetPartLoc(dest->plug(), dest->source().id());
    chipPath = dest->getChipPath();


    fprintf(g_fp, "%s<endpoint>\n", mrwIndent(2));
    fprintf(g_fp, "%s<card-id>%s</card-id>\n", mrwIndent(3), dest->plug()->id().c_str());
    fprintf(g_fp, "%s<part-id>%s</part-id>\n", mrwIndent(3), dest->partId().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), partLoc.c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), chipPath.c_str());
    fprintf(g_fp, "%s<width>%s</width>\n", mrwIndent(3), a.width.c_str());
    fprintf(g_fp, "%s<card-size>%s</card-size>\n", mrwIndent(3), a.card_size.c_str());
    fprintf(g_fp, "%s<gen>%s</gen>\n", mrwIndent(3), a.gen.c_str());
    fprintf(g_fp, "%s<slot-index>%s</slot-index>\n", mrwIndent(3), a.slot_index.c_str());
    fprintf(g_fp, "%s<capi>%s</capi>\n", mrwIndent(3), a.capi.c_str());
    fprintf(g_fp, "%s<hot-plug>%s</hot-plug>\n", mrwIndent(3), a.hot_plug.c_str());
    fprintf(g_fp, "%s<is-slot>%s</is-slot>\n", mrwIndent(3), a.is_slot.c_str());
    if (a.is_slot == "Yes")
    {
	    fprintf(g_fp, "%s<default-pcie-cooling-type>%s</default-pcie-cooling-type>\n",
			mrwIndent(3), a.default_pcie_cooling_type.c_str());
	    fprintf(g_fp, "%s<default-power-consumption>%s</default-power-consumption>\n",
			mrwIndent(3), a.default_power_consumption.c_str());
    }
    fprintf(g_fp, "%s<lsi>%s</lsi>\n", mrwIndent(3), a.lsi.c_str());


    fprintf(g_fp, "%s</endpoint>\n", mrwIndent(2));

    //////////////////////////////////////////////
    //Print the system path

    printSystemPath(i_bus);


    ///////////////////////////////////////////////
    //Need an index that the xslt can use

    fprintf(g_fp, "%s<entry-num>%d</entry-num>\n", mrwIndent(2), s_count++);

    fprintf(g_fp, "%s</pcie-bus>\n", mrwIndent(1));
}




bool sortPCIEBus(PCIESystemBus* i_left, PCIESystemBus* i_right)
{
    ecmdTarget_t t1 = mrwGetTarget(i_left->root()->getChipPath(),  g_targetFile);
    ecmdTarget_t t2 = mrwGetTarget(i_right->root()->getChipPath(), g_targetFile);

    if (t1.node < t2.node)
    {
        return true;
    }
    else if (t1.node == t2.node)
    {
        if (t1.position < t2.position)
        {
            return true;
        }
        else if (t1.position == t2.position)
        {
            return i_left->destination()->getChipPath() < i_right->destination()->getChipPath();
        }
    }

    return false;
}




void printPCIEBusses(vector<PCIESystemBus*> & i_busses)
{
    sort(i_busses.begin(), i_busses.end(), sortPCIEBus);

    for_each(i_busses.begin(), i_busses.end(), PrintPCIEBus(i_busses));
}


int main(int argc, char* argv[])
{
    string inputFile, outputFile;

    if (mrwParseArgs(argc, argv, inputFile, outputFile, g_targetFile))
    {
        usage();
        return 1;
    }

    if (SystemXML::load(inputFile))
            return 1;

    vector<PCIESystemBus*> busses;

    mrwPCIEMakeBusses(busses);

    g_fp = mrwPrintHeader(outputFile, "pcie-busses", "mrwpcie");
    if (!g_fp)
        return 1;

    printPCIEBusses(busses);

    mrwPrintFooter("pcie-busses", g_fp);

    return mrwGetReturnCode();
}

