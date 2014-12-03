/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwLocationCodeParser.C $                                     */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwLocationCodeParser.C 3$

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <xmlutil.H>
#include <mrwParserCommon.H>

/**
 * Generates an XML file that just contains location codes based on the system XML.
 */

using namespace std;


struct LocationCodeEntry
{
    string loc;
    string path;
    string assembly_type;
    string description;
    string position;
};

//Globals
string g_inputFile;
string g_outputFile;
string g_assemblyLoc;
vector<LocationCodeEntry> g_locationCodes;



void usage()
{
    cout << "Usage: mrwLocationCodeParser  --in <full system xml file> --out <output xml file>\n";
}


int parseArgs(int argc, char* argv[])
{
    string arg;

    if (argc != 5)
    {
        cerr << "Invalid number of arguments\n";
        usage();
        return 1;
    }

    for (int i=1;i<argc;i++)
    {
        arg = argv[i];

        if (arg == "--in")
            g_inputFile = argv[++i];
        else if (arg == "--out")
            g_outputFile = argv[++i];
        else
        {
            cerr << "Unrecognized argument: " << arg << endl;
            usage();
            return 1;
        }
    }

    return 0;
}

/**
 * Saves the passed in location code data to the global location code list
 */
void addLocationCodeEntry(
	const string& i_loc,
	const string& i_path,
	const string& i_pos,
	const string& i_assembly_type,
	const string& i_desc)
{
    LocationCodeEntry entry;

    //Open Power doesn't use location codes, though they still need this XML
#ifndef MRW_OPENPOWER
    entry.loc = i_loc;
#else
    entry.loc = "n/a";
#endif
    entry.path = i_path;
    entry.position = i_pos;
    entry.assembly_type = i_assembly_type;
    entry.description = i_desc;
    g_locationCodes.push_back(entry);
}


/**
 * Builds the location code entries for for the part-instances that have them inside of the card elements
 */
void buildPartsOnCardLocs(const string & i_card, const string & i_parentLoc, const string& i_parentPath)
{
    XMLElement card = SystemXML::root().findPath("layout/cards").find("card", "id", i_card);
    vector<XMLElement> parts;
    vector<XMLElement>::iterator p;
    string loc, path;

    card.findPath("part-instances").getChildren(parts, "part-instance");

    for (p=parts.begin();p!=parts.end();++p)
    {
        if (!p->findPath("location").empty())
        {
            loc = i_parentLoc + "-" + p->getChildValue("location");
            path = i_parentPath + "/" + p->getChildValue("part-id") + "-" + p->getChildValue("position");
            addLocationCodeEntry(loc, path, p->getChildValue("position"), "", p->getChildValue("part-id"));
        }
    }
}




/**
 * Builds the location code entries for for the connector-instances that have them inside of the card elements
 */
void buildConnectorsOnCardLocs(const string & i_card, const string & i_parentLoc, const string & i_parentPath)
{
    XMLElement card = SystemXML::root().findPath("layout/cards").find("card", "id", i_card);
    vector<XMLElement> connectors;
    vector<XMLElement>::iterator c;
    string loc, path;

    card.findPath("connector-instances").getChildren(connectors, "connector-instance");

    for (c=connectors.begin();c!=connectors.end();++c)
    {
        //Only look for 'T' location codes that point to connectors, we already have
        //all of the card location codes, like C2, etc, that are already listed in the layout
        if (c->getChildValue("location").substr(0, 1) == "T")
        {
            loc = i_parentLoc + "-" + c->getChildValue("location");

            path = i_parentPath + "/" + c->getChildValue("connector-id") + "-" + c->getChildValue("position");

            addLocationCodeEntry(loc, path, c->getChildValue("position"), "", c->getChildValue("connector-id"));
        }
    }
}

/**
 * Returns the 'card-id' element with the passed in id value from the selection-group-entry that its in.
 */
void getSelectionGroupEntryIDs(const string & i_id, vector<XMLElement> & o_entryIDElements)
{
    vector<XMLElement> entries;
    vector<XMLElement>::iterator e;
    XMLElement group;
    XMLElement entry;

    o_entryIDElements.clear();
    group = SystemXML::root().findPath("layout/selection-groups-used").find("selection-group-used", "id", i_id);

    if (group.empty())
    {
        mrwLogger logger;
        logger() << "Couldn't find selection group: " << i_id;
        logger.error(true);
    }

    group.getChildren(entries, "selection-group-entry");

    for (e=entries.begin();e!=entries.end();++e)
    {
        entry = e->findPath("card-id");

        if (!entry.empty())
        {
            o_entryIDElements.push_back(entry);
        }
        else
        {
            entry = e->findPath("card-id");

            if (!entry.empty())
            {
                o_entryIDElements.push_back(entry);
            }
            else
                cerr << "Warning:  Couldn't find part-id or card-id element for selection group " << i_id << endl;
        }
    }

}



/**
 * Determine if this plug contains a processor under it
 */
bool plugContainsProcessor(XMLElement& i_plug)
{
	bool rc = false;
	string card_id = i_plug.getChildValue("card-id");
    vector<XMLElement> children;
    vector<XMLElement>::iterator p;

	// Check this plug
    XMLElement card = SystemXML::root().findPath("layout/cards").find("card", "id", card_id);
    if ((card.getChildValue("card-type") == "dcm-module") ||
        (card.getChildValue("card-type") == "scm-module"))
    {
    	rc = true;
    }

	// Check child plugs
	if (!rc)
	{
        i_plug.getChildren(children, "plug");
        for (p=children.begin(); (p != children.end()) && (!rc); ++p)
        {
            rc = plugContainsProcessor(*p);
        }
	}

	return(rc);
}

/**
 * Builds the location code entries for the Plug elements.
 */
void buildPlugLocs(XMLElement& i_plug, const string & i_parentLoc, const string& i_parentPath)
{
    string loc = i_parentLoc + "-" + i_plug.getChildValue("location");
    string path;
    string id;
    string desc;
    vector<XMLElement>::iterator p;
    bool card = false;
    bool selectionGroup = false;
    vector<XMLElement> children;
    XMLElement locElement;
    vector<XMLElement> sgIDs;
    vector<string> IDs;
    vector<string>::iterator i;


    locElement = i_plug.findPath("location");

    //If it's not an absolute location code, append this segment to the parent's loc.
    //If it is absolute, then just append this current segment to the assembly loc.
    string abs = locElement.getAttributeValue("absolute");
    if (abs == "yes")
        loc = g_assemblyLoc + "-" + locElement.getValue();
    else if (abs == "ignore")
        loc = i_parentLoc;
    else
        loc = i_parentLoc + "-" + locElement.getValue();

    //A plug could have a card-id, a part-id, or a selection-group-id
    if (i_plug.getChildValue("card-id") != "")
    {
        card = true;
        id = i_plug.getChildValue("card-id");
        IDs.push_back(id);
    }
    else if (i_plug.getChildValue("part-id") != "")
    {
        id = i_plug.getChildValue("part-id");
        IDs.push_back(id);
    }
    else if (i_plug.getChildValue("selection-group-id") != "")
    {
        selectionGroup = true;

        //a selection-group entry could either have card-id or part-id
        getSelectionGroupEntryIDs(i_plug.getChildValue("selection-group-id"), sgIDs);
        for (p=sgIDs.begin();p!=sgIDs.end();++p)
        {
            if (p->getName() == "card-id")
            {
                IDs.push_back(p->getValue());
                card = true;
            }
            else
            {
                //part-id
                IDs.push_back(p->getValue());
            }
        }

    }
    else
    {
        mrwLogger logger;
        logger() << "no card, part, or selection group ID found for plug with parent " << i_parentPath;
        logger.error(true);
    }



    for (i=IDs.begin();i!=IDs.end();++i)
    {
        path = i_parentPath + "/" + *i + "-" + i_plug.getChildValue("position");

        if (card)
        {
            XMLElement card = mrwGetCard(*i);
            desc = *i + " " + card.getChildValue("card-type");
        }
        else
        {
            XMLElement part = mrwGetPart(*i);
            desc = *i + " " + part.getChildValue("part-type");
        }


        addLocationCodeEntry(loc, path, i_plug.getChildValue("position"), "", desc);

        //do the location codes of parts and connectors on this card
        if (card)
        {
            buildConnectorsOnCardLocs(id, loc, path);

            buildPartsOnCardLocs(id, loc, path);
        }

        //do the child plugs
        children.clear();
        i_plug.getChildren(children, "plug");
        for (p=children.begin();p!=children.end();++p)
        {
            buildPlugLocs(*p, loc, path);
        }
    }

}


/**
 * Assembly level function to build the location code entries.
 */
void buildAssemblyLocs(XMLElement & i_assembly, string & i_loc, string & i_pos)
{
    vector<XMLElement> plugs;
    vector<XMLElement>::iterator p;
    string path = i_assembly.getChildValue("id") + "-" + i_pos;
    string this_assembly_type("control");

    g_assemblyLoc = i_loc;
    i_assembly.getChildren(plugs, "plug");

	// Determine this assembly's type: if there is a processor on it, change type to "compute"
    for (p=plugs.begin();p!=plugs.end();++p)
    {
        if (plugContainsProcessor(*p))
        {
        	this_assembly_type = "compute";
        	break;
        }
    }

    //save the assembly location code entry
    addLocationCodeEntry(i_loc, path, i_pos, this_assembly_type, i_assembly.getChildValue("description"));

    //do all the plug children
    for (p=plugs.begin();p!=plugs.end();++p)
    {
        buildPlugLocs(*p, i_loc, path);
    }

}

/**
 * High level function to build the location code entries
 */
void buildLocationCodes()
{
    vector<XMLElement> assemblies;
    vector<XMLElement>::iterator a;
    XMLElement assembly;

    //the 'assemblies-used' section has the position and location code
    SystemXML::root().findPath("layout/assemblies-used").getChildren(assemblies, "assembly-used");

    for (a=assemblies.begin();a!=assemblies.end();++a)
    {
        string loc = a->getChildValue("location");
        string pos = a->getChildValue("position");

        //find the actual assembly to traverse next
        assembly = SystemXML::root().findPath("layout/assemblies").find("assembly", "id", a->getChildValue("assembly-id"));

        buildAssemblyLocs(assembly, loc, pos);
    }
}


/**
 * Used in the sort() function to sort via location code
 */
bool sortLoc(const LocationCodeEntry & i_left, const LocationCodeEntry & i_right)
{
    return (i_left.loc < i_right.loc);
}



/**
 * Prints the location code XML entries to the location code XML file.
 */
void printLocationCodes()
{
    vector<LocationCodeEntry>::iterator e;

    FILE* fp = fopen(g_outputFile.c_str(), "w");
    if (!fp)
    {
        mrwLogger logger;
        logger() << "Error: couldn't open output file " << g_outputFile;
        logger.error(true);
        return;
    }

    //sort them
    sort(g_locationCodes.begin(), g_locationCodes.end(), sortLoc);

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
    mrwPrintTimeStampComment(fp);
    fprintf(fp, "\n<location-codes");
    fprintf(fp, " xmlns:mrwloc=\"http://w3.ibm.com/stg/power-firmware/schema/mrwloc\"\n");
    fprintf(fp, "                xmlns=\"http://w3.ibm.com/stg/power-firmware/schema/mrwloc\">\n");
    string indent(4, ' ');

    for (e=g_locationCodes.begin();e!=g_locationCodes.end();++e)
    {
        fprintf(fp, "%s<location-code-entry>\n", indent.c_str());
        fprintf(fp, "%s%s<location-code>%s</location-code>\n", indent.c_str(), indent.c_str(), e->loc.c_str());
        fprintf(fp, "%s%s<description>%s</description>\n", indent.c_str(), indent.c_str(), e->description.c_str());
        fprintf(fp, "%s%s<instance-path>%s</instance-path>\n", indent.c_str(), indent.c_str(), e->path.c_str());
        fprintf(fp, "%s%s<position>%s</position>\n", indent.c_str(), indent.c_str(), e->position.c_str());
        fprintf(fp, "%s%s<assembly-type>%s</assembly-type>\n", indent.c_str(), indent.c_str(), e->assembly_type.c_str());
        fprintf(fp, "%s</location-code-entry>\n", indent.c_str());
    }

    fprintf(fp, "</location-codes>\n");
    fclose(fp);

}



int main(int argc, char* argv[])
{

    if (mrwParseArgs(argc, argv, g_inputFile, g_outputFile))
        return 1;

    if (SystemXML::load(g_inputFile))
    {
        cerr << "Failed loading XML file " << g_inputFile << endl;
        return 1;
    }


    buildLocationCodes();

    printLocationCodes();

    return 0;
}
