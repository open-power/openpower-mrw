/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwChipIDs.C $                                                */
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
#include <algorithm>
#include <xmlutil.H>
#include <mrwPowerBusCommon.H>


/**
 * Prints the logical chip IDs  Basically looks in the proc-chips-ids.xml file and then
 * tacks on the instance-path and location-code from the targets file
 */


using namespace std;


//Globals
FILE*  g_fp;



/**
 * Structure to contain logical chip IDs for a processor
 */
struct logicalChipId_t
{
    int node; //logical fabric node
    int pos;  //logical fabric position on the node
    ecmdTarget_t target;
};


void usage()
{
    cout << "Usage: mrwLogicalIDs  --in <system-proc-chip-ids.xml file> --out <output xml file> --targets <ecmd targets xml file> \n";
}




void printChipId(const logicalChipId_t & i_id)
{

    fprintf(g_fp, "%s<chip-id>\n", mrwIndent(1));

    fprintf(g_fp, "%s<node>%d</node>\n", mrwIndent(2), i_id.node);
    fprintf(g_fp, "%s<position>%d</position>\n", mrwIndent(2), i_id.pos);
    fprintf(g_fp, "%s<target><node>%d</node><position>%d</position></target>\n", mrwIndent(2),
            i_id.target.node, i_id.target.position);
    fprintf(g_fp, "%s<location-code>%s</location-code>\n", mrwIndent(2), i_id.target.location.c_str());
    fprintf(g_fp, "%s<instance-path>%s</instance-path>\n", mrwIndent(2), i_id.target.instancePath.c_str());

    fprintf(g_fp, "%s</chip-id>\n", mrwIndent(1));
}



void printChipIds(vector<logicalChipId_t> & i_ids)
{
    for_each(i_ids.begin(), i_ids.end(), printChipId);
}


//Finds an ecmdTarget_t based on an XMLElement that has node and position
//child values
class FindTarget
{
public:
    FindTarget(const XMLElement & i_targetElement) :
        iv_element(i_targetElement) {}

    bool operator()(const ecmdTarget_t & i_target)
    {
        return atoi(iv_element.getChildValue("node").c_str()) == i_target.node &&
               atoi(iv_element.getChildValue("position").c_str()) == i_target.position;
    }
private:
    const XMLElement & iv_element;

};


/**
 * Finds all of the Ids in the IDs file, and then finds the corresponding
 * targets for eacfh ID entry.
 */
void getLogicalIds(const string & i_chipIdFile,
                   const vector<ecmdTarget_t> & i_targets,
                   vector<logicalChipId_t> & o_ids)
{
    XMLTree tree;
    XMLElement root, targetElement;
    vector<ecmdTarget_t>::const_iterator t;
    vector<XMLElement> ids;
    vector<XMLElement>::iterator i;

    if (tree.load(i_chipIdFile))
    {
        mrwLogger l;
        l() << "Could not load chip id file " << i_chipIdFile;
        l.error();
        return;
    }

    tree.getRoot(root);
    root.getChildren(ids);

    for (i=ids.begin();i!=ids.end();++i)
    {
        logicalChipId_t id;
        id.node = atoi(i->getChild("chip-id").getChildValue("node").c_str());
        id.pos =  atoi(i->getChild("chip-id").getChildValue("position").c_str());

        targetElement = i->getChild("target");

        t = find_if(i_targets.begin(), i_targets.end(), FindTarget(targetElement));

        if (t == i_targets.end())
        {
            mrwLogger l;
            l() << "Unable to find processor target for n" <<
                   targetElement.getChildValue("node") << "p" <<
                   targetElement.getChildValue("position");
            l.error();
        }
        else
        {
            id.target = *t;
        }

        o_ids.push_back(id);
    }

}



int main(int argc, char* argv[])
{
    string inputFile, outputFile, targetFile;
    vector<ecmdTarget_t> processors;
    vector<logicalChipId_t> ids;

    if (mrwParseArgs(argc, argv, inputFile, outputFile, targetFile))
        return 1;

    mrwGetTargets("pu", targetFile, processors);

    getLogicalIds(inputFile, processors, ids);

    g_fp = mrwPrintHeader(outputFile, "chip-ids", "mrwid");
    if (!g_fp)
        return 1;

    printChipIds(ids);

    mrwPrintFooter("chip-ids", g_fp);

    return mrwGetReturnCode();
}


