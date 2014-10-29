// IBM_PROLOG_BEGIN_TAG 
// This is an automatically generated prolog. 
//  
// fips830 src/mrw/xml/consumers/common/mrwProcSpiParser.C 1.1 
//  
// IBM CONFIDENTIAL 
//  
// OBJECT CODE ONLY SOURCE MATERIALS 
//  
// COPYRIGHT International Business Machines Corp. 2012 
// All Rights Reserved 
//  
// The source code for this program is not published or otherwise 
// divested of its trade secrets, irrespective of what has been 
// deposited with the U.S. Copyright Office. 
//  
// IBM_PROLOG_END_TAG 
#include <iostream>
#include <algorithm>
#include <mrwProcSpiCommon.H>

using namespace std;

/**
 * Prints out XML for the processor->APSS SPI connections
 */


//globals
FILE* g_fp = NULL;
string g_targetFile;



void usage()
{
    cout << "Usage: mrwProcSpiParser --in <full system xml file> --out <output xml file> --targets <ecmd targets xml file>\n";
}


void printProcSpiBus(SpiSystemBus* i_bus)
{
    ecmdTarget_t target = mrwGetTarget(i_bus->master()->getChipPath(), g_targetFile);


    fprintf(g_fp, "%s<processor-spi-bus>\n", mrwIndent(1));

    fprintf(g_fp, "%s<processor>\n", mrwIndent(2));
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), i_bus->master()->getChipPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), i_bus->master()->plug()->location().c_str());
    fprintf(g_fp, "%s<target><name>%s</name><node>%d</node><position>%d</position></target>\n",
                       mrwIndent(3), target.name.c_str(), target.node, target.position);
    fprintf(g_fp, "%s</processor>\n", mrwIndent(2));

    fprintf(g_fp, "%s<endpoint>\n", mrwIndent(2));
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(3), i_bus->slave()->getChipPath().c_str());
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(3), i_bus->slave()->plug()->location().c_str());
    fprintf(g_fp, "%s</endpoint>\n", mrwIndent(2));

    fprintf(g_fp, "%s</processor-spi-bus>\n", mrwIndent(1));
}


void printProcSpiBusses(const vector<SpiSystemBus*> & i_systemBusses)
{
    for_each(i_systemBusses.begin(), i_systemBusses.end(), printProcSpiBus);
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

    vector<SpiSystemBus*> systemBusses;

    mrwProcSpiMakeBusses(systemBusses);

    g_fp = mrwPrintHeader(outputFile, "processor-spi-busses", "mrwspi");
    if (!g_fp)
        return 1;

    printProcSpiBusses(systemBusses);

    mrwPrintFooter("processor-spi-busses", g_fp);

    return mrwGetReturnCode();
}



