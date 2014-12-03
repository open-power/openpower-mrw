/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwDMIParser.C $                                              */
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
#include <mrwDMICommon.H>


/**
 * Prints out XML for the processor-> centaur DMI connections
 */

using namespace std;


//globals
FILE* g_fp = NULL;
string g_targetFile;

void usage()
{
    cout << "Usage: mrwDMIParser --in <full system xml file> --out <output xml file> --targets <ecmd targets xml file>\n";
}


void printDMIBus(DMISystemBus* i_bus)
{
    fprintf(g_fp, "%s<dmi-bus>\n", mrwIndent(1));

    ////////////////////////////////////////////
    //MCS

    ecmdTarget_t target = mrwGetTarget(i_bus->master()->getMcsPath(), g_targetFile);

    fprintf(g_fp, "%s<mcs>\n", mrwIndent(2));
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), i_bus->master()->getMcsPath().c_str());
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position><chipUnit>%d</chipUnit></target>\n",
            mrwIndent(3), target.name.c_str(), target.node, target.position, target.chipUnit);
    fprintf(g_fp, "%s</mcs>\n", mrwIndent(2));


    ////////////////////////////////////////////
    //Centaur

    target = mrwGetTarget(i_bus->slave()->getChipPath(), g_targetFile);

    fprintf(g_fp, "%s<centaur>\n", mrwIndent(2));
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), i_bus->slave()->getChipPath().c_str());
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
            mrwIndent(3), target.name.c_str(), target.node, target.position);
    fprintf(g_fp, "%s</centaur>\n", mrwIndent(2));

    ///////////////////////////////////////////
    //Swaps
    const char* rxSwap = (i_bus->rxSwap()) ? "true" : "false";
    const char* txSwap = (i_bus->txSwap()) ? "true" : "false";


    fprintf(g_fp, "%s<upstream-n-p-lane-swap-mask>0x%.8X</upstream-n-p-lane-swap-mask>\n", mrwIndent(2), i_bus->getUpstreamNPLaneSwap());
    fprintf(g_fp, "%s<downstream-n-p-lane-swap-mask>0x%.8X</downstream-n-p-lane-swap-mask>\n", mrwIndent(2), i_bus->getDownstreamNPLaneSwap());
    fprintf(g_fp, "%s<rx-msb-lsb-swap>%s</rx-msb-lsb-swap>\n", mrwIndent(2), rxSwap);
    fprintf(g_fp, "%s<tx-msb-lsb-swap>%s</tx-msb-lsb-swap>\n", mrwIndent(2), txSwap);
    fprintf(g_fp, "%s<mcs-refclock-enable-mapping>%d</mcs-refclock-enable-mapping>\n", mrwIndent(2), i_bus->refclockMap());


    fprintf(g_fp, "%s</dmi-bus>\n", mrwIndent(1));

}


bool sortDMIBus(DMISystemBus* i_left, DMISystemBus* i_right)
{
    ecmdTarget_t t1 = mrwGetTarget(i_left->master()->getMcsPath(), g_targetFile);
    ecmdTarget_t t2 = mrwGetTarget(i_right->master()->getMcsPath(), g_targetFile);

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
            if (t1.chipUnit < t2.chipUnit)
                return true;
        }
    }

    return false;
}



void printDMIBusses(vector<DMISystemBus*> & i_dmiBusses)
{
    sort(i_dmiBusses.begin(), i_dmiBusses.end(), sortDMIBus);

    for_each(i_dmiBusses.begin(), i_dmiBusses.end(), printDMIBus);
};


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

    vector<DMISystemBus*> dmis;

    mrwDMIMakeBusses(dmis);

    g_fp = mrwPrintHeader(outputFile, "dmi-busses", "mrwdmi");
    if (!g_fp)
        return 1;

    printDMIBusses(dmis);

    mrwPrintFooter("dmi-busses", g_fp);

    return mrwGetReturnCode();
}
