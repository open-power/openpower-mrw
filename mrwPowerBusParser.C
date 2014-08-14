/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwPowerBusParser.C $                                         */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwPowerBusParser.C 5$

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <xmlutil.H>
#include <mrwParserCommon.H>
#include <mrwPowerBusCommon.H>

/**
 * This program will walk the topology XML and build a separate XML file that just
 * contains information on the power bus connections.
 */


using namespace std;


//Globals
string g_targetFile;
FILE* g_fp = NULL;


struct powerBusUnit_t
{
    string missingUnit;
    PowerBusSystemEndpoint* processor;
    vector<string> units;
};


void usage()
{
    cout << "Usage: mrwPowerBusParser  --in <full system xml file> --out <output xml file> --targets <ecmd targets xml file> \n";
}



/**
 * Wrapper for mrwGetTarget that just strips off the unit first.
 */
void getPowerBusTarget(const string & i_busPath, ecmdTarget_t & o_target)
{
    string path = i_busPath;

    //remove the unit from the end so we can find it in the targets file
    int pos = path.find_last_of('/');
    path = path.substr(0, pos);

    o_target = mrwGetTarget(path, g_targetFile);
}


/**
 * Prints an XML entry for a power bus connection in the power bus file.
 */
void printPowerBus(PowerSystemBus* i_bus)
{
    PowerBusSource* source = i_bus->source();
    PowerBusDestination* dest = i_bus->destination();
    vector<string> & segments = i_bus->segments();
    vector<string>::const_iterator segment;
    string cableConnPath;

    ecmdTarget_t sourceTarget = mrwGetTarget(source->getChipPath(), g_targetFile);
    ecmdTarget_t destTarget   = mrwGetTarget(dest->getChipPath(), g_targetFile);

    const char* rxSwap = (i_bus->rxSwap()) ? "true" : "false";
    const char* txSwap = (i_bus->txSwap()) ? "true" : "false";

    fprintf(g_fp, "%s<power-bus>\n", mrwIndent(1));

    fprintf(g_fp, "%s<description>n%d:p%d:%s to n%d:p%d:%s</description>\n",
            mrwIndent(2), sourceTarget.node, sourceTarget.position, source->source().unit().c_str(),
            destTarget.node, destTarget.position, dest->source().unit().c_str());

	string source_unit_type = (source->source().unit()).substr(0,1);
	if ((source_unit_type == "a") || (source_unit_type == "A"))
	{
	    fprintf(g_fp, "%s<bus-type>abus</bus-type>\n", mrwIndent(2));
	}
	else if ((source_unit_type == "x") || (source_unit_type == "X"))
	{
	    fprintf(g_fp, "%s<bus-type>xbus</bus-type>\n", mrwIndent(2));
	}
	else
	{
		mrwLogger l;
		l() << "Unable to parse power bus type from unit name \"" << source->source().unit() << "\"";
		l.error(true);
	}
    fprintf(g_fp, "%s<bus-width>%d</bus-width>\n", mrwIndent(2), i_bus->busWidth());

    fprintf(g_fp, "%s<rx-msb-lsb-swap>%s</rx-msb-lsb-swap>\n", mrwIndent(2), rxSwap);
    fprintf(g_fp, "%s<tx-msb-lsb-swap>%s</tx-msb-lsb-swap>\n", mrwIndent(2), txSwap);
    fprintf(g_fp, "%s<upstream-n-p-lane-swap-mask>0x%.8X</upstream-n-p-lane-swap-mask>\n", mrwIndent(2), i_bus->getUpstreamNPLaneSwap());
    fprintf(g_fp, "%s<downstream-n-p-lane-swap-mask>0x%.8X</downstream-n-p-lane-swap-mask>\n", mrwIndent(2), i_bus->getDownstreamNPLaneSwap());
    fprintf(g_fp, "%s<include-for-node-config>%s</include-for-node-config>\n", mrwIndent(2), i_bus->getNodeConfig().c_str());
    fprintf(g_fp, "%s<cable-name>%s</cable-name>\n", mrwIndent(2), i_bus->getCableName().c_str());

    fprintf(g_fp, "%s<endpoint>\n", mrwIndent(2));

    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
            mrwIndent(3), sourceTarget.name.c_str(), sourceTarget.node, sourceTarget.position);

    fprintf(g_fp, "%s<port>%s</port>\n", mrwIndent(3), source->source().unit().c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), source->getUnitPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), sourceTarget.location.c_str());
    fprintf(g_fp, "%s<endpoint-type>source</endpoint-type>\n", mrwIndent(3));

    cableConnPath = i_bus->getCableConnPath(PowerSystemBus::PB_SOURCE);
    fprintf(g_fp, "%s<cable-connector-instance-path>%s</cable-connector-instance-path>\n",
            mrwIndent(3), cableConnPath.c_str());

    fprintf(g_fp, "%s</endpoint>\n", mrwIndent(2));
    fprintf(g_fp, "%s<endpoint>\n", mrwIndent(2));

    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
            mrwIndent(3), destTarget.name.c_str(), destTarget.node, destTarget.position);

    fprintf(g_fp, "%s<port>%s</port>\n", mrwIndent(3), dest->source().unit().c_str());

    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), dest->getUnitPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), destTarget.location.c_str());
    fprintf(g_fp, "%s<endpoint-type>destination</endpoint-type>\n", mrwIndent(3));

    cableConnPath = i_bus->getCableConnPath(PowerSystemBus::PB_DEST);
    fprintf(g_fp, "%s<cable-connector-instance-path>%s</cable-connector-instance-path>\n",
            mrwIndent(3), cableConnPath.c_str());

    fprintf(g_fp, "%s</endpoint>\n", mrwIndent(2));



    fprintf(g_fp, "%s<bus-path>\n", mrwIndent(2));
    for (segment=segments.begin();segment!=segments.end();++segment)
    {
        fprintf(g_fp, "%s<path-segment>%s</path-segment>\n", mrwIndent(3), (*segment).c_str());
    }

    fprintf(g_fp, "%s</bus-path>\n", mrwIndent(2));

    fprintf(g_fp, "%s</power-bus>\n", mrwIndent(1));
}


/**
 * Prints one endpoint of a powerbus connection, for when a powerbus port is unused but
 * HWSV still needs to know about it
 */
void printPowerBusUnit(powerBusUnit_t & i_unit)
{
    PowerBusSystemEndpoint* source = i_unit.processor;
    ecmdTarget_t sourceTarget = mrwGetTarget(source->getChipPath(), g_targetFile);
    string unitPath = source->getChipPath() + "/" + i_unit.missingUnit;

    fprintf(g_fp, "%s<power-bus skiphtml=\"true\">\n", mrwIndent(1));

    fprintf(g_fp, "%s<description>n%d:p%d:%s unit, not connected</description>\n",
            mrwIndent(2), sourceTarget.node, sourceTarget.position, i_unit.missingUnit.c_str());

    fprintf(g_fp, "%s<bus-width></bus-width>\n", mrwIndent(2));

    fprintf(g_fp, "%s<rx-msb-lsb-swap></rx-msb-lsb-swap>\n", mrwIndent(2));
    fprintf(g_fp, "%s<tx-msb-lsb-swap></tx-msb-lsb-swap>\n", mrwIndent(2));
    fprintf(g_fp, "%s<upstream-n-p-lane-swap-mask></upstream-n-p-lane-swap-mask>\n", mrwIndent(2));
    fprintf(g_fp, "%s<downstream-n-p-lane-swap-mask></downstream-n-p-lane-swap-mask>\n", mrwIndent(2));
    fprintf(g_fp, "%s<include-for-node-config>all</include-for-node-config>\n", mrwIndent(2));
    fprintf(g_fp, "%s<cable-name></cable-name>\n", mrwIndent(2));

    fprintf(g_fp, "%s<endpoint>\n", mrwIndent(2));

    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
            mrwIndent(3), sourceTarget.name.c_str(), sourceTarget.node, sourceTarget.position);

    fprintf(g_fp, "%s<port>%s</port>\n", mrwIndent(3), i_unit.missingUnit.c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), unitPath.c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), sourceTarget.location.c_str());
    fprintf(g_fp, "%s<cable-connector-instance-path></cable-connector-instance-path>\n", mrwIndent(3));

    fprintf(g_fp, "%s</endpoint>\n", mrwIndent(2));

    fprintf(g_fp, "%s<endpoint>\n", mrwIndent(2));
    fprintf(g_fp, "%s</endpoint>\n", mrwIndent(2));

    fprintf(g_fp, "%s<bus-path>\n", mrwIndent(2));
    fprintf(g_fp, "%s</bus-path>\n", mrwIndent(2));

    fprintf(g_fp, "%s</power-bus>\n", mrwIndent(1));

}


void getUnitsOfType(PowerBusSource* i_source, const string & i_type,
                    vector<string> & o_units)
{
    vector<XMLElement> units;
    vector<XMLElement>::iterator u;

    i_source->part().findPath("units/powerbus-units").getChildren(units);

    for (u=units.begin();u!=units.end();++u)
    {
        if (u->getChildValue("type") == i_type)
        {
            o_units.push_back(u->getChildValue("id"));
        }
    }
}


/**
 * Class to find a PowerBusSystemEndpoint in a vector of powerBusUnit_ts
 */
class FindProc
{
public:
    FindProc(PowerBusSystemEndpoint* i_endpoint) :
        iv_endpoint(i_endpoint) {}

    bool operator()(const powerBusUnit_t & i_pbu)
    {
        return (iv_endpoint->getChipPath() == i_pbu.processor->getChipPath());
    }

private:
    PowerBusSystemEndpoint* iv_endpoint;
};


/**
 * Finds which processors have powerbus units that aren't connected to another processor,
 * and puts them into o_units along with the missing unit.
 */
void findMissingPBUnits(vector<PowerSystemBus*> & i_busses,
                        const string & i_type, vector<powerBusUnit_t> & o_units)
{
    vector<PowerSystemBus*>::iterator bus;
    vector<string> unitIds;
    vector<powerBusUnit_t> allUnits;
    vector<powerBusUnit_t>::iterator pbu;
    string m;

	m = ">findMissingPBUnits(ibusses, \"" + i_type + "\",...)";
	mrwLogger::debug(m);

    for (bus=i_busses.begin();bus!=i_busses.end();++bus)
    {
        //Get all possible unit IDs of this type from the part
        if (unitIds.empty())
            getUnitsOfType((*bus)->source(), i_type, unitIds);

        //Find everything we do have, both source and destination,
        //and save in allUnits, but ignore types we don't care about

        if ((*bus)->source()->getUnit().getChildValue("type") == i_type)
        {
            pbu = find_if(allUnits.begin(), allUnits.end(), FindProc((*bus)->source()));
            if (pbu == allUnits.end())
            {
                powerBusUnit_t p;
                p.processor = (*bus)->source();
                p.units.push_back((*bus)->source()->source().unit());
                allUnits.push_back(p);
            }
            else
            {
                //already have the processor, so save the unit if its a new one
                if (find(pbu->units.begin(), pbu->units.end(),
                         (*bus)->source()->source().unit()) == pbu->units.end())
                {
                    pbu->units.push_back((*bus)->source()->source().unit());
                }

            }
        }


        if ((*bus)->destination()->getUnit().getChildValue("type") == i_type)
        {
            pbu = find_if(allUnits.begin(), allUnits.end(), FindProc((*bus)->destination()));
            if (pbu == allUnits.end())
            {
                powerBusUnit_t p;
                p.processor = (*bus)->destination();
                p.units.push_back((*bus)->destination()->source().unit());
                allUnits.push_back(p);
            }
            else
            {
                //already have the processor, so save the unit if its a new one
                if (find(pbu->units.begin(), pbu->units.end(),
                         (*bus)->destination()->source().unit()) == pbu->units.end())
                {
                    pbu->units.push_back((*bus)->destination()->source().unit());
                }

            }
        }
    }


    //Now, find what's missing
    vector<string>::iterator u;

    for (pbu=allUnits.begin();pbu!=allUnits.end();++pbu)
    {
        for (u=unitIds.begin();u!=unitIds.end();++u)
        {
            m = "Checking for unconnected powerbus unit " + *u + " for " + pbu->processor->getChipPath();
            mrwLogger::debug(m);
            if (find(pbu->units.begin(), pbu->units.end(), *u) == pbu->units.end())
            {
                if (mrwLogger::getDebugMode()) {
                    m = "   Found unconnected powerbus unit " + *u + " for " + pbu->processor->getChipPath();
                    mrwLogger::debug(m);
                }

                powerBusUnit_t missingUnit;
                missingUnit.missingUnit = *u;
                missingUnit.processor = pbu->processor;
                o_units.push_back(missingUnit);
            }
        }
    }

	m = "<findMissingPBUnits(ibusses, \"" + i_type + "\",...)";
	mrwLogger::debug(m);
}



void printPowerBusses(vector<PowerSystemBus*> & i_busses)
{
    for_each(i_busses.begin(), i_busses.end(), printPowerBus);

    vector<powerBusUnit_t> units;

    //HWSV/Hostboot also needs to know about the powerbus units that aren't connected
    //like when a system just uses A0 and A1 but not the A2 bus.
    findMissingPBUnits(i_busses, "A", units);
    findMissingPBUnits(i_busses, "X", units);

    for_each(units.begin(), units.end(), printPowerBusUnit);
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

    vector<PowerSystemBus*> busses;

    mrwPowerBusMakeBusses(busses);


    g_fp = mrwPrintHeader(outputFile, "power-busses", "mrwpb");
    if (!g_fp)
        return 1;

    printPowerBusses(busses);

    mrwPrintFooter("power-busses", g_fp);

    return mrwGetReturnCode();
}


