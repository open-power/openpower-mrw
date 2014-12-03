/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwParserCommon.C $                                           */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwParserCommon.C 6$

#include <iostream>
#include <time.h>
#include <mrwParserCommon.H>
#include <algorithm>

using namespace std;


/**
 * Common code that can be used by the various MRW XML parsers.
 */



//Globals
vector<Plug*> g_plugs;
vector<Cable*> g_cables;


mrwErrMode mrwLogger::cv_mode = MRW_DEFAULT_ERR_MODE;
int mrwLogger::cv_rc = 0;
bool mrwLogger::cv_debugMode = 0;
XMLTree SystemXML::cv_tree;
XMLElement SystemXML::cv_root;

// XPath format strings
static const char* plug_xpath_base_fmt = "//mrw:assembly[mrw:id/text()='%s']";
static const char* plug_xpath_seg_fmt = "/mrw:plug[mrw:card-id/text()='%s' and mrw:position/text()='%s']";
static const char* part_xpath_fmt = "//mrw:card[mrw:id/text()='%s']//mrw:part-instance[mrw:part-id='%s' and mrw:position/text()='%s']";
static const char* chiplet_xpath_fmt = "//mrw:part[mrw:id/text()='%s']//mrw:chiplet[mrw:id/text()='%s']";
static const char* unit_xpath_fmt = "//mrw:part[mrw:id/text()='%s']//mrw:%s[mrw:id/text()='%s']";


/**
 * Constructor
 */
SystemEndpoint::SystemEndpoint(Endpoint & i_source, Plug* i_plug) :
    iv_source(i_source), iv_plug(i_plug)
{
    iv_partId = mrwGetPartId(i_plug->card(), i_source.id());
    iv_part = SystemXML::root().findPath("layout/parts").find("part", "id", iv_partId);
}


/**
* Returns a path like '/assembly-0/shilin-0/DPSS-0'
*/
std::string SystemEndpoint::getChipPath()
{
    if (iv_chipPath.empty())
    {
        iv_chipPath = plug()->path() + "/";

        iv_chipPath += partId() + "-";
        iv_chipPath += mrwGetPartPos(plug()->card(), source().id());

    }

    return iv_chipPath;
}

/**
 * Returns a unit path, like '/assembly-0/shilin-0/DPSS-0/GPIO5.
 */
std::string SystemEndpoint::getUnitPath()
{
    if (iv_unitPath.empty())
    {
        iv_unitPath = getChipPath() + "/" + source().unit();
    }

    return iv_unitPath;
}

/**
 * Return the restrict-to-variation-id value for this endpoint (if any)
 */
std::string SystemEndpoint::getRestriction()
{
	return mrwGetPartInstanceData(iv_plug, iv_source.id(), "restrict-to-variation-id");
}


/**
 * Loads the XML file into XMLTree cv_tree, and places
 * the root of the tree into XMLElement cv_root.
 *
 * @param i_xmlFile - the MRW XML file to load
 */
int SystemXML::load(const string & i_xmlFile)
{
    if (cv_tree.load(i_xmlFile))
    {
        mrwLogger logger;
        logger() << "Failed loading XML file " << i_xmlFile;
        logger.error(true);
        return 1;
    }

    cv_tree.getRoot(cv_root);

    return 0;
}


/**
 * Loads the XML file into XMLTree SystemXML::tree(), and places
 * the root of the tree into XMLElement SystemXML::root().
 *
 * @param i_xmlFile - the MRW XML file to load
 */
int mrwLoadGlobalTreeAndRoot(const string & i_xmlFile)
{
    return SystemXML::load(i_xmlFile);
}


/**
 * Handles parsing the command line arguments for the two standard args
 * that most parsers take:  input and output
 * @param i_argc - argc from main()
 * @param i_argv - argv from main()
 * @param o_inputFile - set to the --input argument
 * @param o_outputFile - set to the --output argument
 * @return - 0 if successful
 */
int mrwParseArgs(int i_argc, char* i_argv[],
                 string & o_inputFile,
                 string & o_outputFile)
{
    string arg;

    if (!((i_argc == 5) || (i_argc == 6)))
    {
        cerr << "Invalid number of arguments\n";
        return 1;
    }

    for (int i=1;i<i_argc;i++)
    {
        arg = i_argv[i];

        if (arg == "--in")
            o_inputFile = i_argv[++i];
        else if (arg == "--out")
            o_outputFile = i_argv[++i];
        else if (arg == "--strict")
            mrwSetErrMode(MRW_STRICT_ERR_MODE);
        else if (arg == "--debug")
            mrwSetDebugMode(true);
        else
        {
            cerr << "Unrecognized argument: " << arg << endl;
            return 1;
        }
    }

    return 0;
}

/**
 * Handles parsing the command line arguments for the three standard args
 * that most parsers take:  input, output, targets
 * @param i_argc - argc from main()
 * @param i_argv - argv from main()
 * @param o_inputFile - set to the --input argument
 * @param o_outputFile - set to the --output argument
 * @param o_targetFile - set to the --targets argument
 * @return - 0 if successful
 */
int mrwParseArgs(int i_argc, char* i_argv[],
                 string & o_inputFile,
                 string & o_outputFile,
                 string & o_targetFile)
{
    string arg;

    if (!((i_argc == 7) || (i_argc == 8)))
    {
        cerr << "Invalid number of arguments\n";
        return 1;
    }

    for (int i=1;i<i_argc;i++)
    {
        arg = i_argv[i];

        if (arg == "--in")
            o_inputFile = i_argv[++i];
        else if (arg == "--out")
            o_outputFile = i_argv[++i];
        else if (arg == "--targets")
            o_targetFile = i_argv[++i];
        else if (arg == "--strict")
            mrwSetErrMode(MRW_STRICT_ERR_MODE);
        else if (arg == "--debug")
            mrwSetDebugMode(true);
        else
        {
            cerr << "Unrecognized argument: " << arg << endl;
            return 1;
        }
    }

    return 0;
}




/**
 * Gets an ecmd target structure from the targets XML for the instance-path
 * passed in.
 */
ecmdTarget_t mrwGetTarget(const string & i_instancePath,
                          const string & i_targetFile,
                          bool i_loadXPath)
{
    ecmdTarget_t target;
    XMLElement t;
    static XMLTree s_tree;
    static XMLElement s_root;


    if (s_root.empty())
    {
        if (s_tree.load(i_targetFile))
        {
            cerr << "Failed loading XML file " << i_targetFile << endl;
            return target;
        }

        s_tree.getRoot(s_root);
    }

    //find the target entry that matches the path
    t = s_root.find("target", "instance-path", i_instancePath);

    if (!t.empty())
    {
        target.node = atoi(t.getChildValue("node").c_str());
        target.position = atoi(t.getChildValue("position").c_str());
        target.chipUnit = atoi(t.getChildValue("chip-unit").c_str());
        target.name = t.getChildValue("ecmd-common-name");
        target.instancePath = t.getChildValue("instance-path");
        target.location = t.getChildValue("location");

        if (i_loadXPath)
        {
            target.plugXPath = t.getChildValue("plug-xpath");
            target.partXPath = t.getChildValue("part-xpath");
            target.chipletXPath = t.getChildValue("chiplet-xpath");
        }
    }


    return target;
}

/**
 * Converts an ecmd target structure into canonical string format.
 */
void mrwGetTargetString(const ecmdTarget_t& i_target,
                        std::string& o_target_str)
{
	char buf[256];

	if (i_target.name.empty())
	{
		buf[0] = '\0';
	}
	else
	{
		if (i_target.chipUnit != -1)
		{
			sprintf(buf, "%s:n%d:p%d:u%d",
				i_target.name.c_str(),
				i_target.node,
				i_target.position,
				i_target.chipUnit);
		}
		else
		{
			sprintf(buf, "%s:n%d:p%d",
				i_target.name.c_str(),
				i_target.node,
				i_target.position);
		}
	}
	o_target_str = buf;
}



/**
 * Find all the targets that have the ecmd-common-name passed in
 */
void mrwGetTargets(const string & i_name,
                   const string & i_targetFile,
                   vector<ecmdTarget_t> & o_targets,
                   bool i_loadXPath)
{
    static XMLTree s_tree;
    static XMLElement s_root;
    vector<XMLElement> targets;
    vector<XMLElement>::iterator t;

    if (s_root.empty())
    {
        if (s_tree.load(i_targetFile))
        {
            cerr << "Failed loading XML file " << i_targetFile << endl;
        }

        s_tree.getRoot(s_root);
    }


    s_root.getChildren(targets);

    for (t=targets.begin();t!=targets.end();++t)
    {
        if (t->getChildValue("ecmd-common-name") == i_name)
        {
            ecmdTarget_t target;

            target.node = atoi(t->getChildValue("node").c_str());
            target.position = atoi(t->getChildValue("position").c_str());
            target.chipUnit = atoi(t->getChildValue("chip-unit").c_str());
            target.name = t->getChildValue("ecmd-common-name");
            target.instancePath = t->getChildValue("instance-path");
            target.location = t->getChildValue("location");

            if (i_loadXPath)
            {
                target.plugXPath = t->getChildValue("plug-xpath");
                target.partXPath = t->getChildValue("part-xpath");
                target.chipletXPath = t->getChildValue("chiplet-xpath");
            }

            o_targets.push_back(target);
        }

    }

}



/**
 * Returns the <part> element for the ID passed in
 *
 * @param i_partId - the part ID, like CENTAUR
 */
XMLElement mrwGetPart(const string i_partId)
{
    if (!i_partId.empty())
        return SystemXML::root().findPath("layout/parts").find("part", "id", i_partId);
    else
        return XMLElement();
}



/**
 * Returns the <card> element for the ID passed in
 *
 * @param i_cardId - the part ID, like venice_scm
 */
XMLElement mrwGetCard(const string i_cardId)
{
    return SystemXML::root().findPath("layout/cards").find("card", "id", i_cardId);
}


/**
 * Returns the unit element specified, but can look through multiple parts
 * if i_instance ID refers to multiple parts.
 */
XMLElement mrwGetUnit(const string & i_instanceId,
                      const string & i_unitId,
                      const string & i_unitType,
                      Plug* i_plug)
{
    XMLElement unit;
    vector<XMLElement> instances;
    vector<XMLElement>::iterator i;

    //Can look through multiple parts having the same instance ID.

    i_plug->card().getChild("part-instances").getChildren(instances, "part-instance");

    for (i=instances.begin();i!=instances.end();++i)
    {
        if (i->getChildValue("id") == i_instanceId)
        {
            unit = mrwGetUnit(i->getChildValue("part-id"), i_unitType, i_unitId);

            if (!unit.empty())
                return unit;
        }
    }

    return unit;
}


/**
 * Returns the unit element specified
 */
XMLElement mrwGetUnit(const string & i_partId,
                      const string & i_unitType,
                      const string & i_unitId)
{
    XMLElement unit;
    XMLElement part = mrwGetPart(i_partId);
    vector<XMLElement> unitGroups;
    vector<XMLElement>::iterator unitGroup;

    part.findPath("units").getChildren(unitGroups);

    for (unitGroup=unitGroups.begin();unitGroup!=unitGroups.end();++unitGroup)
    {

        unit = unitGroup->find(i_unitType, "id", i_unitId);

        if (!unit.empty())
        {
            break;
        }

    }

    return unit;
}



/**
 * Returns the unit element specified.
 *
 * @param i_partId - the ID of the part the unit is in
 * @param i_unitType - the type of unit, from the element, like <pcie-root-unit>
 * @param i_unitId - the ID of the unit
 */
void mrwGetUnits(const string & i_partId,
                 const string & i_unitType,
                 vector<XMLElement> & o_units)
{
    XMLElement part = mrwGetPart(i_partId);
    vector<XMLElement> unitGroups;
    vector<XMLElement> units;
    vector<XMLElement>::iterator unitGroup;
    vector<XMLElement>::iterator unit;

    o_units.clear();
    part.findPath("units").getChildren(unitGroups);

    for (unitGroup=unitGroups.begin();unitGroup!=unitGroups.end();++unitGroup)
    {
        unitGroup->getChildren(o_units, i_unitType);

        if (!o_units.empty())
        {
            break;
        }
    }

}


/**
 * Gets a part ID from a card element and its instance.
 */
string mrwGetPartId(XMLElement& i_card, const string& i_instanceID)
{
    string id;
    if (!i_instanceID.empty())
    {
        XMLElement instance = i_card.getChild("part-instances").find("part-instance", "id", i_instanceID);
        id = instance.getChildValue("part-id");
    }

    return id;
}




/**
 * Returns the location code of a part on a card
 */
string mrwGetPartLoc(Plug* i_plug, const string& i_instanceID)
{
    string location = i_plug->location();
    string loc;
    XMLElement instance;

    instance = i_plug->card().getChild("part-instances").find("part-instance", "id", i_instanceID);

    loc = instance.getChildValue("location");
    if (!loc.empty())
    {
        location += "-" + loc;
    }

    return location;
}


/**
 * Used to return an element from the <part-instance> element in the card XML,
 * such as <part-instance><some-name>name</some-name></part-instance>
 */
string mrwGetPartInstanceData(Plug* i_plug, const string& i_instanceId,
                              const string& i_name)
{
    string data;
    XMLElement instance;

    instance = i_plug->card().getChild("part-instances").find("part-instance", "id", i_instanceId);

    data = instance.getChildValue(i_name);

    return data;
}


/**
 * Gets a part position from a card element and its instance.
 */
string mrwGetPartPos(XMLElement& i_card, const string& i_instanceID)
{
    string pos;
    XMLElement instance;

    instance = i_card.getChild("part-instances").find("part-instance", "id", i_instanceID);

    pos = instance.getChildValue("position");

    return pos;
}



/**
* Returns the part type for the instance passed in.
*/
string mrwGetPartType(XMLElement & i_card, const string & i_instanceId)
{
    return mrwGetPart(mrwGetPartId(i_card, i_instanceId)).getChildValue("part-type");
}

/**
* Returns the part class for the instance passed in.
*/
string mrwGetPartClass(XMLElement & i_card, const string & i_instanceId)
{
    string c;

    string partId = mrwGetPartId(i_card, i_instanceId);

    if  (!partId.empty())
    {
        XMLElement part = mrwGetPart(partId);
        c = part.getChildValue("part-class");
    }

    return c;
}


/**
 * For use caching part types
 */
struct partMap_t
{
    Plug* plug;
    string instance;
    string type;
};

/**
 * A version of getPartType that takes a Plug, and also caches the result to make it faster
 * for future calls
 */
string mrwGetPartType(Plug* i_plug, const string & i_instanceId)
{
    static vector<partMap_t> s_types;
    vector<partMap_t>::iterator i;

    //return the saved result if we have it
    for (i=s_types.begin();i!=s_types.end();++i)
    {
       if ((i->plug == i_plug) && (i->instance == i_instanceId))
       {
            return i->type;
       }
    }


    partMap_t m;
    m.plug = i_plug;
    m.instance = i_instanceId;
    m.type =  mrwGetPart(mrwGetPartId(i_plug->card(), i_instanceId)).getChildValue("part-type");
    s_types.push_back(m);

    return m.type;
}


/**
 * Returns the current date time like Thu Sep 15 09:39:22 2011
 */
string mrwGetCurrentDateTime()
{
    string current;
    time_t t;
    tm* local;

    time(&t);
    local = localtime(&t);
    current = asctime(local);

    //remove the newline
    current = current.substr(0, current.length() - 1);
    return current;
}


/**
 * Prints the XML comment <!-- Generated <current date/time> -->
 */
void mrwPrintTimeStampComment(FILE* i_fp)
{
    string dateTime = mrwGetCurrentDateTime();

    fprintf(i_fp, "<!-- Generated %s -->\n", dateTime.c_str());
}


/**
 * Opens i_fileName for writing, and prints a header that looks like:
 *
 * <chip-ids xmlns:mrwid="http://w3.ibm.com/stg/power-firmware/schema/mrwid"
             xmlns="http://w3.ibm.com/stg/power-firmware/schema/mrwid">
 *
 * @param i_fileName - the file name to open and write to
 * @param i_rootElement - the root element name, like: chip-ids
 * @param i_namespacePrefix - the namespace prefix, like: mrwid
 * @return - an open file descriptor for the file, or NULL if it couldn't be opened.
 */
FILE* mrwPrintHeader(const string & i_fileName,
                     const string & i_rootElement,
                     const string & i_namespacePrefix)
{
    FILE* fp = fopen(i_fileName.c_str(), "w");
    if (!fp)
    {
        mrwLogger logger;
        logger() << "Couldn't open output file " << i_fileName;
        logger.error(true);
    }
    else
    {
        string URI = "http://w3.ibm.com/stg/power-firmware/schema";
        string fullNS = URI + "/" + i_namespacePrefix;

        fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
        mrwPrintTimeStampComment(fp);
        fprintf(fp, "\n<%s", i_rootElement.c_str());
        fprintf(fp, " xmlns:%s=\"%s\"\n", i_namespacePrefix.c_str(), fullNS.c_str());
        fprintf(fp, "            xmlns=\"%s\">\n", fullNS.c_str());
    }

    return fp;
}


/**
 * Prints the </root-element> tag to the file and then closes to the file
 */
void mrwPrintFooter(const string & i_rootElement,
                    FILE* io_fp)
{
    fprintf(io_fp, "</%s>\n", i_rootElement.c_str());
    fclose(io_fp);
}



/**
 * Finds a Plug object based on it's path.  It's recursive - for
 * top level call pass in g_plugs.
 */
Plug* mrwFindPlug(const string & i_path, vector<Plug*> & i_plugs)
{
    vector<Plug*>::iterator p;
    Plug* plug = NULL;

    for (p=i_plugs.begin();p!=i_plugs.end();++p)
    {
        if ((*p)->path() == i_path)
            plug = *p;
        else
            plug = mrwFindPlug(i_path, (*p)->children());

        if (plug)
            break;

    }

    return plug;
}

/**
 * Returns the Plug and connector on the opposite end of the cable from the
 * plug and connector passed in.  If the parameters passed in don't match
 * an end, then returns false, otherwise, returns true.
 */
bool Cable::getOtherEnd(Plug* i_thisPlug, const std::string & i_thisConnector,
                        Plug *& o_nextPlug, std::string & o_nextConnector)
{
    bool found = false;
    o_nextPlug = NULL;
    o_nextConnector.clear();

    if ((i_thisPlug == iv_source) && (i_thisConnector == iv_sourceConnector))
    {
        found = true;
        o_nextPlug = iv_target;
        o_nextConnector = iv_targetConnector;
    }
    else if ((i_thisPlug == iv_target) && (i_thisConnector == iv_targetConnector))
    {
        found = true;
        o_nextPlug = iv_source;
        o_nextConnector = iv_sourceConnector;
    }


    if (mrwLogger::getDebugMode())
    {
        if (o_nextPlug)
        {
            string m;
            m = "Cable::getOtherEnd:  thisEnd: " + i_thisPlug->path() + "/" + i_thisConnector +
               " otherEnd: " + o_nextPlug->path() + "/" + o_nextConnector;
            mrwLogger::debug(m);
        }
    }


    return found;
}

/**
 *  Creates the Cable objects from the <cables> section in the XML, and populates
 *  the g_cables Plug::cables vectors.
 */
void mrwMakeCables()
{
    vector<XMLElement> cables;
    vector<XMLElement>::iterator cable;
    string sourcePath, targetPath;
    string sourceConnector, targetConnector;
    Plug *source, *target;

    SystemXML::root().findPath("layout/cables").getChildren(cables);

    if (mrwLogger::getDebugMode())
    {
        ostringstream m; m << "Found " << cables.size() << " cable entries";
        mrwLogger::debug(m.str());
    }

    for (cable=cables.begin();cable!=cables.end();++cable)
    {
        sourcePath = cable->getChildValue("source-id");
        targetPath = cable->getChildValue("target-id");
        sourceConnector = cable->getChildValue("source-connector-instance");
        targetConnector = cable->getChildValue("target-connector-instance");

        source = mrwFindPlug(sourcePath, g_plugs);
        target = mrwFindPlug(targetPath, g_plugs);


        if (source && target)
        {
            Cable* c = new Cable(source, sourceConnector,
                                 target, targetConnector, *cable);
            source->addCable(c);
            target->addCable(c);
            g_cables.push_back(c);
        }

        if (mrwLogger::getDebugMode())
        {
            string name = cable->getChildValue("name");
            string m;

            if (source && target)
            {
                m = "Found cable " + name + " between " + source->path() + "/" + sourceConnector +
                    " and " + target->path() + "/" + targetConnector;
            }
            else if (source)
               m = "For cable " + name + ", found source " + source->path() + " but no target";
            else if (target)
               m = "For cable " + name + ", found target " + target->path() + " but no source";
            else
               m = "Found neither a source or target for cable " + name;

            if (!m.empty()) mrwLogger::debug(m);
        }


    }

}


/**
* Initializes Plug's members
*/
void Plug::init(XMLElement* i_selectionGroupEntry)
{
	iv_pluggable_at_standby = false;
    if ( iv_element.getChildValue("pluggable-at-standby") == "yes" )
    {
     	iv_pluggable_at_standby = true;
    }

    //No more PART support, see an older version for how it was done.
    iv_type = CARD;

    //If part of a selection group, have to get the ID and child connector from there
    if (i_selectionGroupEntry != NULL)
    {
        iv_inSelectionGroup = true;
        iv_id = i_selectionGroupEntry->getChildValue("card-id");

        //have to find the actual connector of the card, not the virtual connector of the group entry
        //which is what's in the plug
        string unMappedChildConn =  iv_element.findPath("connection-point/child").getValue();

        vector<XMLElement> maps;
        vector<XMLElement>::iterator map;

        i_selectionGroupEntry->findPath("connector-maps").getChildren(maps, "connector-map");

        for (map=maps.begin();map!=maps.end();++map)
        {
            if (map->getChildValue("virtual-connector-id") == unMappedChildConn)
            {
                iv_childConnector = map->getChildValue("connector-id");
                break;
            }
        }
    }
    else
    {
        iv_inSelectionGroup = false;
    }

    if (mrwLogger::getDebugMode())
    {
        string m = "Found plug:  " + path() + "  loc:  " + location();
        mrwLogger::debug(m);
    }

}

/**
 * Returns the ID
 */
string Plug::id()
{
    if (iv_id.empty())
    {
        iv_id = iv_element.getChildValue("card-id");

        if (iv_id.empty()) //make sure nothing horrible happened
        {
            mrwLogger l;
            l() << "Couldn't find ID for plug with parent " << parent()->path();
            l.error(true);
        }
    }

    return iv_id;
}


/**
 * Returns the parent connector
 */
string Plug::parentConnector()
{
    if (iv_parentConnector.empty())
    {
        iv_parentConnector = iv_element.findPath("connection-point/parent").getValue();
    }

    return iv_parentConnector;
}


/**
 * Returns the child connector
 */
string Plug::childConnector()
{
    if (iv_childConnector.empty())
    {
        iv_childConnector = iv_element.findPath("connection-point/child").getValue();
    }

    return iv_childConnector;
}


/**
 * Returns the position
 */
string Plug::position()
{
    if (iv_position.empty())
    {
        iv_position = iv_element.getChildValue("position");
    }

    return iv_position;

}


/**
 * Returns the card element, if a card
 */
XMLElement & Plug::card()
{
    if (iv_card.empty())
    {
        iv_card = mrwGetCard(id());
    }

    return iv_card;
}



/**
 * Returns the path, like assembly-0/shilin-0
 */
string Plug::path()
{
    if (iv_path.empty())
    {
        if (iv_parent != NULL)
            iv_path = iv_parent->path() + "/" + id() + "-" + position();
        else
            iv_path = iv_assembly.id + "-" + iv_assembly.position + "/" + id() + "-" + position();
    }

    return iv_path;
}


/**
 * Returns the location code
 */
string Plug::location()
{
#ifndef MRW_OPENPOWER
    if (iv_location.empty())
    {
        if (iv_parent != NULL)
        {

            XMLElement loc = iv_element.findPath("location");

            //If it's not an absolute location code, append this segment to the parent's loc.
            //If it is absolute, then just append this current segment to the assembly loc.
            string abs = loc.getAttributeValue("absolute") ;
            if (abs == "yes")
                iv_location = iv_assembly.loc + "-" + loc.getValue();
            else if (abs == "ignore")
                iv_location = iv_parent->location(); //use the parent's
            else
                iv_location = iv_parent->location() + "-" + loc.getValue();

        }
        else
        {
            iv_location = iv_assembly.loc + "-" + iv_element.getChildValue("location");
        }

    }
#endif

    return iv_location;
}

void Plug::getCableEndpoint(const string & i_thisConnector,
                            Plug *& o_nextPlug, string & o_nextConnector,
                            Cable *& o_cable)
{
    bool found = false;
    o_nextPlug = NULL;
    o_nextConnector.clear();

    if (!iv_cables.empty())
    {
        vector<Cable*>::iterator c;

        for (c=iv_cables.begin();c!=iv_cables.end();++c)
        {
            found = (*c)->getOtherEnd(this, i_thisConnector,
                                      o_nextPlug, o_nextConnector);

            o_cable = *c;
            if (found)
                break;

        }

    }

}


/**
 * Makes a Plug object.  Saves the top level ones to g_plugs.
 */
void mrwMakePlug(XMLElement& i_plug, const assembly_t & i_assembly, Plug* i_parent)
{
    vector<XMLElement> children;
    vector<XMLElement>::iterator p;

    //expand the selection groups into separate Plug objects
    if (i_plug.getChildValue("selection-group-id") != "")
    {

        XMLElement group = SystemXML::root().findPath("layout/selection-groups-used").find("selection-group-used", "id", i_plug.getChildValue("selection-group-id"));
        vector<XMLElement> entries;
        vector<XMLElement>::iterator entry;

        if (group.empty())
            cerr << "Error: couldn't find selection group " << i_plug.getChildValue("selection-group-id") << endl;

        group.getChildren(entries, "selection-group-entry");

        for (entry=entries.begin();entry!=entries.end();++entry)
        {
            Plug* plug = new Plug(i_plug, i_parent, i_assembly, &(*entry));

            if (i_parent == NULL)
                g_plugs.push_back(plug); //save the top level plugs in the global array
            else
                i_parent->children().push_back(plug);

            i_plug.getChildren(children, "plug");

            for (p=children.begin();p!=children.end();++p)
            {
                mrwMakePlug(*p, i_assembly, plug);
            }

        }
    }
    else
    {
        Plug* plug = new Plug(i_plug, i_parent, i_assembly, NULL);

        if (i_parent == NULL) //top level
            g_plugs.push_back(plug);
        else
            i_parent->children().push_back(plug);

        i_plug.getChildren(children, "plug");

        for (p=children.begin();p!=children.end();++p)
        {
            mrwMakePlug(*p, i_assembly, plug);
        }
    }
}


struct plugConn
{
    string connector;
    vector<Plug*> plugs;

    bool operator==(string i_connector)
    {
        return connector == i_connector;
    }
};


/**
 * Tells a plug about the other plugs that alternatively can plug into it's same slot.
 * For example:
 *      <plug>
 *          <id>card1</id>
 *          <parent-connector>JCONN</parent-connector>
 *          <child-connector>JCARD1</child-connector>
 *          <location>C2</location>
 *      </plug>
 *      <plug>
 *          <id>card2</id>
 *          <parent-connector>JCONN</parent-connector>
 *          <child-connector>JCARD2</child-connector>
 *          <location>C2</location
 *      </plug>
 *
 *  Recursive.  Pass in the top plugs, and it will drill down to the children.
 */
void mrwAssignAlternates(vector<Plug*> & io_plugs)
{
    vector<Plug*>::iterator p;
    vector<Plug*>::iterator plug;
    vector<Plug*>::iterator child;
    vector<plugConn> connectors;
    vector<plugConn>::iterator c;
    plugConn con;

    for (p=io_plugs.begin();p!=io_plugs.end();++p)
    {
        //If this plugs into the same connector as another plug, then these all
        //fit in the same slot and we need to tell each one about the others

        c = find(connectors.begin(), connectors.end(), (*p)->parentConnector());
        if (c == connectors.end())
        {
            con.plugs.clear();
            con.connector = (*p)->parentConnector();
            con.plugs.push_back(*p);
            connectors.push_back(con);
        }
        else
        {
            //Tell this new alternate about the others,
            //and tell the others about this new one

            for (plug=c->plugs.begin();plug!=c->plugs.end();++plug)
            {
                (*plug)->addAlternate(*p);
                (*p)->addAlternate(*plug);
            }

            //now add this plug under its connector
            c->plugs.push_back(*p);
        }

        //do the chidren
        mrwAssignAlternates((*p)->children());

    }

}



/**
 * Used to sort a plug based on its position member
 */
bool sortPlugByPosition(Plug* i_left, Plug* i_right)
{
    return (atoi(i_left->position().c_str()) < atoi(i_right->position().c_str()));
}


/**
 * Sorts the plugs at the same level based on <position> in the layout xml.
 */
void mrwSortPlugs(vector<Plug*> & io_plugs)
{
    vector<Plug*>::iterator p;

    sort(io_plugs.begin(), io_plugs.end(), sortPlugByPosition);

    //now sort the children
    for (p=io_plugs.begin();p!=io_plugs.end();++p)
    {
        mrwSortPlugs((*p)->children());
    }

}


/**
 * Entry point to make all Plug objects.  The top level plugs will be put
 * into the global g_plugs.
 */
void mrwMakePlugs()
{
    vector<XMLElement> assemblies;
    vector<XMLElement>::iterator a;
    vector<XMLElement>plugs;
    vector<XMLElement>::iterator p;
    XMLElement assembly;


    //Already been created, don't do again
    if (!g_plugs.empty())
    {
        return;
    }

    SystemXML::root().findPath("layout/assemblies-used").getChildren(assemblies, "assembly-used");

    for (a=assemblies.begin();a!=assemblies.end();++a)
    {
        //find the actual assembly to traverse next
        assembly = SystemXML::root().findPath("layout/assemblies").find("assembly", "id", a->getChildValue("assembly-id"));

        assembly.getChildren(plugs, "plug");

        for (p=plugs.begin();p!=plugs.end();++p)
        {
            assembly_t info;
            info.id = a->getChildValue("assembly-id");
            info.loc = a->getChildValue("location");
            info.position = a->getChildValue("position");

            mrwMakePlug(*p, info, NULL);
        }
    }

    //now sort them
    mrwSortPlugs(g_plugs);

    //Find the alternates
    mrwAssignAlternates(g_plugs);

    mrwMakeCables();
}



/**
 * Recursive function to delete all Bus* objects from every plug.
 * Called from mrwClearPlugBusses();
 */
void mrwClearPlugBusses(vector<Plug*> & i_plugs)
{
    vector<Plug*>::iterator p;
    vector<Bus*>::iterator b;

    for (p=i_plugs.begin();p!=i_plugs.end();++p)
    {
        for (b=(*p)->busses().begin();b!=(*p)->busses().end();++b)
        {
            delete *b;
        }
        (*p)->busses().clear();

        mrwClearPlugBusses((*p)->children());
    }

}

/**
 *  Deletes all Bus* objects out of the iv_busses vector of every Plug
 *  starting at g_plugs.
 */
void mrwClearPlugBusses()
{
    mrwClearPlugBusses(g_plugs);
}



/**
 * Returns the XMLTree object for the document.  Must have already
 * been loaded to be valid.
 */
XMLTree& mrwGetTree()
{
    return SystemXML::tree();
}


/**
 * Returns the XMLElement object for the root of the document.  Must have
 * already been loaded to be valid.
 * @return
 */
XMLElement& mrwGetRoot()
{
    return SystemXML::root();
}


/**
 * Returns the MRW return code that may be nonzero when the XML fails
 * certain consistency checks, and when the strict mode has been used.
 */
int mrwGetReturnCode()
{
   return mrwLogger::getReturnCode();
}


/**
 * Sets the error mode to either strict or default.  Strict will cause
 * mrwGetReturnCode to return a nonzero return code if mrwError() is called
 * by any of the parsers.  If default is used, it will still return 0.  This
 * can be used to make compiles fail when the XML models aren't correct.
 */
void mrwSetErrMode(mrwErrMode i_mode)
{
    mrwLogger::setErrMode(i_mode);
}

/**
 * Turns on or off debug mode
 */
void mrwSetDebugMode(bool i_mode)
{
    mrwLogger::setDebugMode(i_mode);
}

/**
 * For logging an informational message.  Will print to stderr
 */
void mrwLogger::info()
{
    string msg = "INFO:  " + iv_msg.str();
    cerr << msg << endl;
    iv_msg.str("");
}


/**
 * For logging an error message.  Will print to stderr.  If strict mode
 * is used, will cause mrwGetReturnCode to return a nonzero return code.
 *
 * If i_force is set to true, will force an error regardless of g_errMode.
 */
void mrwLogger::error(bool i_force)
{
    string msg;

    if ((cv_mode == MRW_STRICT_ERR_MODE) || (i_force))
    {
        msg = "ERROR:  " + iv_msg.str();
        cv_rc = -1;
    }
    else
    {
        msg = "INFO:  " + iv_msg.str();
    }

    cerr << msg << endl;

    iv_msg.str("");
}


/**
 * Can be used to add debug messages if the iv_debug is set
 */
void mrwLogger::debug(const string & i_message)
{
    if (cv_debugMode)
    {
        cout << "DEBUG:  " << i_message << endl; //to stdout instead of stderr
    }
}


/**
 * For logging an informational message.  Will print to stderr
 */
void mrwInfo(const string & i_message)
{
    mrwLogger logger;
    logger() << i_message;
    logger.info();
}


/**
 * For logging an error message.  Will print to stderr.  If strict mode
 * is used, will cause mrwGetReturnCode to return a nonzero return code.
 *
 * If i_force is set to true, will force an error regardless of g_errMode.
 */
void mrwError(const string & i_message, bool i_force)
{
    mrwLogger logger;
    logger() << i_message;
    logger.error(i_force);
}



/**
 * Converts a '1' to a '01', etc
 */
string mrwPadNumToTwoDigits(const string & i_num)
{
    string num = i_num;

    if (num.length() == 1)
        num = "0" + num;

    return num;
}


/**
 * Returns a string of the format:
 * <plug path>/<endpoint-id>/<endpoint-unit>
 */
string mrwUnitPath(Plug* i_plug, Endpoint & i_endpoint)
{
   string path = i_plug->path() + "/";
   path += i_endpoint.id() + "/" + i_endpoint.unit();
   return path;
}

/**
 * Returns a string of the format:
 * <plug path>/<endpoint-id>/<endpoint-pin>
 */
string mrwPinPath(Plug* i_plug, Endpoint & i_endpoint)
{
    string path = i_plug->path() + "/";
    path += i_endpoint.id() + "/" + i_endpoint.pin();
    return path;
}


/**
 * constants for indentation
 */
const char* MRW_INDENT_1 = "    ";
const char* MRW_INDENT_2 = "        ";
const char* MRW_INDENT_3 = "            ";
const char* MRW_INDENT_4 = "                ";
const char* MRW_INDENT_5 = "                    ";
const char* MRW_INDENT_6 = "                        ";
const char* MRW_INDENT_7 = "                            ";
const char* MRW_INDENT_8 = "                                ";
const char* MRW_INDENT_9 = "                                    ";


/**
 * Returns a string of spaces necessary to indent to i_level.
 * i_level 1 = 4 spaces, i_level 2 = 8 spaces, etc.
 */
const char* mrwIndent(int i_level)
{
    switch (i_level)
    {
        case 1:
            return MRW_INDENT_1;
            break;
        case 2:
            return MRW_INDENT_2;
            break;
        case 3:
            return MRW_INDENT_3;
            break;
        case 4:
            return MRW_INDENT_4;
            break;
        case 5:
            return MRW_INDENT_5;
            break;
        case 6:
            return MRW_INDENT_6;
            break;
        case 7:
            return MRW_INDENT_7;
            break;
        case 8:
            return MRW_INDENT_8;
            break;
        case 9:
            return MRW_INDENT_9;
            break;
        default:
            cerr << "A bad indent level " << i_level << " was passed to mrwIndent()\n";
            exit(-1);
            break;
    }
}


/**
 * Case insensitive string compare
 */
int mrwStrcmpi(const string & i_left, const string & i_right)
{
    if (i_left.size() == i_right.size())
    {
        for (size_t i=0;i<i_left.size();i++)
        {
            if (tolower(i_left[i]) != tolower(i_right[i]))
                return 1;
        }
        return 0;
    }

    return 1;
}


/**
 * Returns 'A' if FSP A, or 'B' if FSP B.
 */
char mrwGetFspAorB(Plug* i_fspCard)
{

    //On planar systems, the backplane is always position 0.
    //On brazos, chief engineer says the FSP card A is position 0 also.
    //If this changes, obviously will need another check
    if (i_fspCard->position() == "0")
        return 'A';
    else
        return 'B';
}

/**
 * Makes the xpath expression for a plug
 */
string mrwMakePlugXPath(Plug* i_plug)
{
    string xpath;
    char buf[2048];
    Plug* plug = i_plug;
    vector<string> segments;
    vector<string>::reverse_iterator s;

    sprintf(buf, plug_xpath_base_fmt, i_plug->assembly().id.c_str());
    xpath = buf;

    do
    {

        sprintf(buf, plug_xpath_seg_fmt, plug->id().c_str(), plug->position().c_str());

        segments.push_back(buf);

        plug = plug->parent();

    } while (plug != NULL);

    for (s=segments.rbegin();s!=segments.rend();++s)
        xpath += *s;

    return xpath;
}



/**
 * Makes the xpath expression for a part
 */
string mrwMakePartXPath(Plug* i_plug, XMLElement & i_partInstance)
{
    string xpath;
    char buf[2048];


    sprintf(buf, part_xpath_fmt,
            i_plug->id().c_str(),
            i_partInstance.getChildValue("part-id").c_str(),
            i_partInstance.getChildValue("position").c_str());

    xpath = buf;

    return xpath;
}



/**
 * Makes the xpath expression for a chiplet
 */
string mrwMakeChipletXPath(const string& i_partId, const string& i_chipletId)
{
    string xpath;
    char buf[2048];

    sprintf(buf, chiplet_xpath_fmt, i_partId.c_str(), i_chipletId.c_str());

    xpath = buf;
    return xpath;
}

/**
 * Makes the xpath expression for a unit
 */
string mrwMakeUnitXPath(const string& i_partId,
						const string& i_unitElmt,
						const string& i_unitId)
{
    string xpath;
    char buf[2048];

    sprintf(buf, unit_xpath_fmt, i_partId.c_str(), i_unitElmt.c_str(), i_unitId.c_str());

    xpath = buf;
    return xpath;
}

/**
 * Validates that the specified XPath expression references one and only one element
 * in the full XML.
 */
unsigned int mrwValidateXPath(const std::string& i_xpath)
{
	if (i_xpath.empty()) { return(0); }

	unsigned int rc = 0;
	vector<XMLElement> elmts_found;
	SystemXML::root().findXPath(SystemXML::tree(), i_xpath, elmts_found);
	if (elmts_found.empty())
	{
		fprintf(stderr, "ERROR: found generated XPath that does not reference any element: \"%s\"\n", i_xpath.c_str());
		rc = 1;
	}
	else if (elmts_found.size() > 1)
	{
		fprintf(stderr, "ERROR: found generated XPath that points to %ld elements: \"%s\"\n",
				elmts_found.size(),
				i_xpath.c_str());
		rc = 1;
	}
	return(rc);
}
