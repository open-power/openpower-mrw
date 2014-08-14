/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: mrwParserCommon.H $                                           */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/mrwParserCommon.H 4$

#ifndef MRWPARSERCOMMON_H
#define MRWPARSERCOMMON_H


/**
 * Common code that can be used by the MRW XML parsers.
 */


#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <xmlutil.H>


class Plug;


enum mrwErrMode
{
    MRW_DEFAULT_ERR_MODE,
    MRW_STRICT_ERR_MODE
};


/**
 * An ecmd target, that can be loaded from the targets XML file.
 */
struct ecmdTarget_t
{
    int node;
    int position;
    int chipUnit;
    std::string name;
    std::string instancePath;
    std::string location;
    std::string plugXPath;
    std::string partXPath;
    std::string chipletXPath;

    ecmdTarget_t() : node(-1), position(-1), chipUnit(-1) {};
};


/**
 * Holds information about an assembly.
 */
struct assembly_t
{
    std::string id;
    std::string position;
    std::string loc;
};




/**
 * Base class for a source or endpoint of a bus in the card xml.
 */
class Endpoint
{
private:

    std::string iv_id;
    std::string iv_pin;
    std::string iv_unit;

public:

    std::string id() const { return iv_id; }
    std::string pin() const { return iv_pin; }
    std::string unit() const { return iv_unit; }

    void id(const std::string & i) { iv_id = i; }
    void pin(const std::string & p) { iv_pin = p; }
    void unit(const std::string & u) { iv_unit = u; }

    bool operator==(Endpoint& i_right)
    {
        return ((iv_id == i_right.id()) && (iv_pin == i_right.pin()) && (iv_unit == i_right.unit()));
    }

};


enum endpointType
{
    SOURCE,
    ENDPOINT
};



/**
 * Base class of a bus segment found in the card xml.
 * This version has no endpoints, that is up to the child classes.
 */
class Bus
{
private:

    /**
     * The XMLElement object for this bus from the card XML.
     */
    XMLElement iv_element;

    /**
     * Pointer to the Plug object this bus is on.
     */
    Plug* iv_plug;

    /**
     * The ID of this bus, from <id>
     */
    std::string iv_id;

    /**
     * The type of this bus, taken from the name of the XML element.
     */
    std::string iv_type;

public:

    Bus(XMLElement& i_element, Plug* i_plug) : iv_element(i_element), iv_plug(i_plug)
    {
        iv_id = i_element.getChildValue("id");
        iv_type = i_element.getName();
    }

    XMLElement & element() { return iv_element; }
    Plug* plug() { return iv_plug; }
    std::string id() const { return iv_id; }
    std::string type() const { return iv_type; }

};

/**
 * A bus segment that has 1 source and 1 endpoint
 */
class BusSegment : public Bus
{

protected:
    Endpoint iv_source;
    Endpoint iv_endpoint;

public:
    BusSegment(XMLElement& i_element, Plug* i_plug) : Bus(i_element, i_plug) {}

    Endpoint & source() { return iv_source; }
    Endpoint & endpoint() { return iv_endpoint; }

};

/**
 * Represents a cable from the <cables> section in the XML.
 */
class Cable
{
public:
    Cable(Plug* i_source, const std::string & i_sourceConnector,
          Plug* i_target, const std::string & i_targetConnector,
          XMLElement & i_element) :
        iv_source(i_source), iv_sourceConnector(i_sourceConnector),
        iv_target(i_target), iv_targetConnector(i_targetConnector),
        iv_element(i_element) {}

    bool getOtherEnd(Plug* i_thisPlug, const std::string & i_thisConnector,
                     Plug *& o_nextPlug, std::string & o_nextConnector);

    XMLElement & element() { return iv_element; }

    Plug* sourcePlug() { return iv_source; }
    Plug* targetPlug() { return iv_target; }

private:

    Plug* iv_source;
    std::string iv_sourceConnector;
    Plug* iv_target;
    std::string iv_targetConnector;
    XMLElement iv_element;
};



/**
 * Represents a Plug entry in the layout xml, plus some enhancements like
 * bus objects and instance paths. The whole plug hierarchy can
 * be loaded into Plug objects using makePlugs().  Code can then traverse
 * these Plug objects instead of the XML.
 */
class Plug
{
public:

    /**
     * If the Plug is a card or a part
     */
    enum Type
    {
        CARD,//!< CARD
        PART //!< PART
    };

private:

    /**
     * XMLElement for this plug from the layout XML
     */
    XMLElement iv_element;

    /**
     * XML element for the card in the card XML, if this is a card
     */
    XMLElement iv_card;

    /**
    * XML element for the part in the parts XML, if this is a part
    */
    XMLElement iv_part;

    /**
     * id, either from part-id or card-id
     */
    std::string iv_id;

    /**
     * Full location code of this instance
     */
    std::string iv_location;

    /**
     * Position value of this instance
     */
    std::string iv_position;

    /**
     * ID of the parent connector
     */
    std::string iv_parentConnector;

    /**
     * ID of the child connector
     */
    std::string iv_childConnector;

    /**
     * Full instance, path, like roddick-0/connors-0/jedec-0
     */
    std::string iv_path;

    /**
     * Holds information about the assembly the plug is on
     */
    const assembly_t iv_assembly;

    /**
     * Pointer to the parent Plug
     */
    Plug* iv_parent;

    /**
     * Vector of child plugs
     */
    std::vector<Plug*> iv_children;

    /**
     * Card or Part
     */
    Type iv_type;

    /**
     * The busses, from the card XML.
     * Not filled in with makePlugs, or anything in mrwParserCommon.
     */
    std::vector<Bus*> iv_busses;

    /**
     * If the plug is part of a selection group
     */
    bool iv_inSelectionGroup;

    /**
     * Is it pluggable at standby?
     */
    bool iv_pluggable_at_standby;

    std::vector<Plug*> iv_alternates;

    std::vector<Cable*> iv_cables;

public:

    XMLElement & element() { return iv_element; }
    XMLElement & card();
    Type type() const { return iv_type; }

    std::string id();
    std::string location();
    std::string position();

    std::string parentConnector();
    std::string childConnector();

    std::string path();

    const assembly_t & assembly() { return iv_assembly; }

    Plug* parent() { return iv_parent; }
    std::vector<Plug*> & children() { return iv_children; }

    std::vector<Bus*> & busses() { return iv_busses; }

    bool inSelectionGroup() { return iv_inSelectionGroup; }

    bool isPluggableAtStandby() const { return iv_pluggable_at_standby; }

    void addAlternate(Plug* i_alt) { iv_alternates.push_back(i_alt); }

    std::vector<Plug*> & alternates() { return iv_alternates; }

    std::vector<Cable*> & cables() { return iv_cables; }

    void addCable(Cable* i_cable) { iv_cables.push_back(i_cable); }

    void getCableEndpoint(const std::string & i_thisConnector,
                          Plug *& o_nextPlug, std::string & o_nextConnector,
                          Cable *& o_cable);

    Plug(XMLElement& i_element, Plug* i_parent,
         const assembly_t & i_assembly,
         XMLElement* i_selectionGroupEntry) :
        iv_element(i_element), iv_assembly(i_assembly), iv_parent(i_parent)
    {
        init(i_selectionGroupEntry);
    }


    /**
     * Fills in most data members upon construction
     */
    void init(XMLElement* i_selectionGroupEntry);

};

/**
 * Common type
 */
typedef std::vector<Plug*> PlugList;

/**
 * Common iterator type
 */
typedef std::vector<Plug*>::iterator PlugIterator;


/**
 * Base class that represents an end of a system wide bus.  Like an I2C Master or slave.
 */
class SystemEndpoint
{

public:
    /**
     * Constructor
     * @param i_source - the source/endpoint that has the unit for this end of this bus
     * @param i_plug - the plug this endpoint is on
     */
    SystemEndpoint(Endpoint & i_source, Plug* i_plug);

    /**
     * Returns the Endpoint object for this endpoint
     */
    Endpoint& source() { return iv_source; }

    /**
     * Returns the Plug*
     */
    Plug* plug() { return iv_plug; }

    /**
     * Returns the part for this endpoint
     */
    XMLElement & part() { return iv_part; }

    /**
     * Returns the partId
     */
    std::string partId() { return iv_partId; }

   /**
    * Returns the part-type
    */
    std::string partType()
    {
        if (iv_partType.empty()) {
            iv_partType = iv_part.getChildValue("part-type");
        }
        return iv_partType;
    }

    /**
     * Returns the chip path, like '/assembly-0/shilin-0/DPSS-0'
     */
    std::string getChipPath();

    /**
     * Returns a unit path, like '/assembly-0/shilin-0/DPSS-0/GPIO5.
     */
    std::string getUnitPath();

	/**
	 * Return the restrict-to-variation-id value for this endpoint (if any)
	 */
	std::string getRestriction();

protected:

    Endpoint iv_source;
    Plug* iv_plug;
    std::string iv_chipPath;
    std::string iv_unitPath;
    std::string iv_partId;
    std::string iv_partType;
    XMLElement iv_part;
};



/**
 * Class that wrappers the loading the system XML files
 * and returning the tree and root objects.
 * Call load first, then can call root() and tree().
 */
class SystemXML
{

public:
    static int load(const std::string & i_xmlFile);

    static XMLTree & tree() { return cv_tree; }
    static XMLElement & root() { return cv_root; }

private:
    static XMLTree cv_tree;
    static XMLElement cv_root;
};




/**
 * Class to print out info or error messages.
 * Usage:
 *    mrwLogger logger;
 *    logger() << "hello" << g_world;
 *    logger.info();
 *
 */
class mrwLogger
{

public:
    /**
     * Constructor
     */
    mrwLogger() {};

    /**
     * Used to add a message to the log by returning a reference
     * to the ostringstream object that you can then use << on.
     */
    std::ostringstream& operator()()
    {
        return iv_msg;
    }


    /**
     * For logging an informational message.  Will print to stderr
     */
    void info();

    /**
     * For logging an error message.  Will print to stderr.  If strict mode
     * is used, will cause mrwGetReturnCode to return a nonzero return code.
     *
     * If i_force is set to true, will force an error regardless of g_errMode.
     */
    void error(bool i_force = true);

    /**
     * Can be used to add debug messages if the iv_debug is set
     */
    static void debug(const std::string & i_message);

    /**
     * Clears the current log text
     */
    void clear() { iv_msg.str(""); }

    /**
     * Sets the error mode
     */
    static void setErrMode(mrwErrMode i_mode)
    {
        cv_mode = i_mode;
    }

    /**
     * Returns the static return code, which will be set to
     * -1 if someone has called error() with either
     * errMode set to strict or with i_force = true
     */
    static int getReturnCode()
    {
        return cv_rc;
    }

    static void setDebugMode(bool i_mode)
    {
        cv_debugMode = i_mode;
    }

    static bool getDebugMode()
    {
        return cv_debugMode;
    }

private:

    std::ostringstream iv_msg;
    static mrwErrMode cv_mode;
    static int cv_rc;
    static bool cv_debugMode;

};




/**
 * Returns the XMLTree object for the document.  Must have already
 * been loaded to be valid.
 */
XMLTree& mrwGetTree();


/**
 * Returns the XMLElement object for the root of the document.  Must have
 * already been loaded to be valid.
 * @return
 */
XMLElement& mrwGetRoot();


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
                 std::string & o_inputFile,
                 std::string & o_outputFile);

/**
 * Handles parsing the command line arguments for the three standard args
 * that most parsers take:  input, output, targets
 * @param i_argc - argc from main()
 * @param i_argv - argv from main()
 * @param o_inputFile - set to the --input argument
 * @param o_outputFile - set to the --output argument
 * @param o_targetFile - set to the --targets argument
 * @return 0 - if successful
 */
int mrwParseArgs(int i_argc, char* i_argv[],
                 std::string & o_inputFile,
                 std::string & o_outputFile,
                 std::string & o_targetFile);

/**
 * Entry point to make all Plug objects.  The top level plugs will be put
 * into the global g_plugs.
 */
void mrwMakePlugs();


/**
 * Deletes all Bus* objects out of the iv_busses vector of every Plug starting
 * at g_plugs.
 */
void mrwClearPlugBusses();


/**
 * Recursive function to delete all Bus* objects from every plug.
 * Called from mrwClearPlugBusses();
 */
void mrwClearPlugBusses(std::vector<Plug*> & i_plugs);


/**
 * Finds a Plug object based on it's path.  It's recursive - for
 * top level call pass in g_plugs.
 */
Plug* mrwFindPlug(const std::string & i_path, std::vector<Plug*> & i_plugs);


/**
 * Gets a part ID from a card element and its instance.
 *
 * @param i_card - the card the part is on
 * @paraim i_instanceID - the ID of the instance
 */
std::string mrwGetPartId(XMLElement& i_card, const std::string& i_instanceID);

/**
 * Gets a part position from the card XML from a card element and its instance.
 *
 * @param i_card - the card the part is on
 * @param i_instanceID - the ID of the instance
 */
std::string mrwGetPartPos(XMLElement& i_card, const std::string& i_instanceID);

/**
 * Used to return an element from the <part-instance> element in the card XML,
 * such as <part-instance><some-name>name</some-name></part-instance>
 */
std::string mrwGetPartInstanceData(Plug* i_plug, const std::string& i_instanceId,
                                   const std::string& i_name);

/**
 * Returns the part type for the instance passed in.
 *
 * @param i_card - the card the part is on
 * @param i_instanceID - the ID of the instance
 */
std::string mrwGetPartType(XMLElement & i_card, const std::string& i_instanceID);

/**
 * A version of getPartType that takes a Plug, and also caches the result to make it faster
 * for repeated calls with the same instance
 */
std::string mrwGetPartType(Plug* i_plug, const std::string & i_instanceId);


/**
 * Returns the part class for the instance passed in.
 *
 * @param i_card - the card the part is on
 * @param i_instanceID - the ID of the instance
 */
std::string mrwGetPartClass(XMLElement & i_card, const std::string& i_instanceID);


/**
 * Returns the location code of a part on a card
 *
 * @param i_plug - the Plug object the part is on
 *
 * @param i_instanceID - the instance ID of the part
 */
std::string mrwGetPartLoc(Plug* i_plug, const std::string& i_instanceID);


/**
 * Returns the <part> element for the ID passed in
 *
 * @param i_partId - the part ID, like CENTAUR
 */
XMLElement mrwGetPart(const std::string i_partId);


/**
 * Returns the <card> element for the ID passed in
 *
 * @param i_cardId - the part ID, like venice_scm
 */
XMLElement mrwGetCard(const std::string i_cardId);


/**
 * Returns the unit element specified, but can look through multiple parts
 * if i_instance ID refers to multiple parts.
 */
XMLElement mrwGetUnit(const std::string & i_instanceId,
                      const std::string & i_unitId,
                      const std::string & i_unitType,
                      Plug* i_plug);

/**
 * Returns the unit element specified.
 *
 * @param i_partId - the ID of the part the unit is in
 * @param i_unitType - the type of unit, from the element, like <pcie-root-unit>
 * @param i_unitId - the ID of the unit
 */
XMLElement mrwGetUnit(const std::string & i_partId,
                      const std::string & i_unitType,
                      const std::string & i_unitId);

/**
 * Returns a list of units with the type specified
 *
 * @param i_partId - the partId whose units are desired, like CENTAUR
 * @param i_unitType - the unit type,  like "fsi-master-unit"
 * @param o_units - filled in with the units of that type
 */
void mrwGetUnits(const std::string & i_partId,
                 const std::string & i_unitType,
                 std::vector<XMLElement> & o_units);

/**
 * Gets an ecmd target structure from the targets XML for the instance-path
 * passed in.
 *
 * @param i_instancePath - the instance path of the target desired
 * @param i_targetFile - the full path to the target XML file
 * @param i_loadXPath - if true, will load in the XPath sections as well, otherwise won't
 * @return the target structure.  will be empty if not found
 */
ecmdTarget_t mrwGetTarget(const std::string & i_instancePath,
                          const std::string & i_targetFile,
                          bool i_loadXPath = false);

/**
 * Converts an ecmd target structure into canonical string format.
 *
 * @param i_target - the ecmd target structure to convert
 * @param o_target_str - the canonical ecmd target string representation
 */
void mrwGetTargetString(const ecmdTarget_t& i_target,
                        std::string& o_target_str);

/**
 * Find all the targets that have the ecmd-common-name passed in
 */
void mrwGetTargets(const std::string & i_name,
                   const std::string & i_targetFile,
                   std::vector<ecmdTarget_t> & o_targets,
                   bool i_loadXPath = false);

/**
 * Returns the current date and time, like: Thu Sep 15 09:39:22 2011
 */
std::string mrwGetCurrentDateTime();


/**
 * Returns a string of spaces necessary to indent to i_level.
 * i_level 1 = 4 spaces, i_level 2 = 8 spaces, etc.
 */
const char* mrwIndent(int i_level);



/**
 * Prints the XML comment: <!-- Generated <current date/time> -->
 * to the file stream passed in.
 *
 * @param i_fp - the open file stream to write the comment to.
 */
void mrwPrintTimeStampComment(FILE* i_fp);


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
FILE* mrwPrintHeader(const std::string & i_fileName,
                     const std::string & i_rootElement,
                     const std::string & i_namespacePrefix);


/**
 * Prints the </root-element> tag to the file and then closes to the file
 */
void mrwPrintFooter(const std::string & i_rootElement,
                    FILE* io_fp);


/**
 * Loads the XML file into XMLTree g_tree, and places
 * the root of the tree into XMLElement g_root.
 *
 * @param i_xmlFile - the MRW XML file to load
 */
int mrwLoadGlobalTreeAndRoot(const std::string & i_xmlFile);


/**
 * Returns the MRW return code that may be nonzero when the XML fails
 * certain consistency checks, and when the strict mode has been used.
 */
int mrwGetReturnCode();


/**
 * Sets the error mode to either strict or default.  Strict will cause
 * mrwGetReturnCode to return a nonzero return code if mrwError() is called
 * by any of the parsers.  If default is used, it will still return 0.  This
 * can be used to make compiles fail when the XML models aren't correct.
 */
void mrwSetErrMode(mrwErrMode i_mode);

/**
 * Turns on or off debug mode
 */
void mrwSetDebugMode(bool i_mode);

/**
 * For logging an informational message.  Will print to stderr
 */
void mrwInfo(const std::string & i_message);


/**
 * For logging an error message.  Will print to stderr.  If strict mode
 * is used, will cause mrwGetReturnCode to return a nonzero return code.
 *
 * If i_force is set to true, will force an error regardless of g_errMode.
 */
void mrwError(const std::string & i_message, bool i_force = false);


/**
 * Converts a '1' to a '01', etc
 */
std::string mrwPadNumToTwoDigits(const std::string & i_num);


/**
 * Returns a string of the format:
 * <plug path>/<endpoint-id>/<endpoint-unit>
 */
std::string mrwUnitPath(Plug* i_plug, Endpoint & i_endpoint);


/**
 * Returns 'A' if FSP A, or 'B' if FSP B.
 */
char mrwGetFspAorB(Plug* i_fspCard);


/**
 * Makes the xpath expression for a plug
 *
 * @param i_plug - pointer to the plug
 * @return - string containing the XPath expression to find that element
 */
std::string mrwMakePlugXPath(Plug* i_plug);

/**
 * Makes the xpath expression for a part
 *
 * @param i_plug - pointer to the plug
 * @param i_partInstance - reference to the part-instance element
 * @return - string containing the XPath expression to find that element
 */
std::string mrwMakePartXPath(Plug* i_plug, XMLElement & i_partInstance);

/**
 * Makes the xpath expression for a chiplet
 *
 * @param i_partId - the part <id> value
 * @param i_chipletId - the chiplet <id> value
 * @return - string containing the XPath expression to find that element
 */
std::string mrwMakeChipletXPath(const std::string& i_partId, const std::string& i_chipletId);

/**
 * Makes the xpath expression for a unit
 *
 * @param i_partId - the part <id> value
 * @param i_unitElmt - the unit element name
 * @param i_unitId - the unit <id> value
 * @return - string containing the XPath expression to find that element
 */
std::string mrwMakeUnitXPath(const std::string& i_partId,
							const std::string& i_unitElmt,
							const std::string& i_unitId);

/**
 * Validates that the specified XPath expression references one and only one element
 * in the full XML.  It is assumed the platform full XML has been loaded into the
 * static SystemXML object.
 *
 * @param i_xpath - the XPath expression to validate
 * @return - 0 if OK, 1 if validation failed
 */
unsigned int mrwValidateXPath(const std::string& i_xpath);

/**
 * Case insensitive string compare.
 * @return 0 - strings are equal, 1 - not equal
 */
int mrwStrcmpi(const std::string & i_left, const std::string & i_right);


/**
 * Returns a string of the format:
 * <plug path>/<endpoint-id>/<endpoint-pin>
 */
std::string mrwPinPath(Plug* i_plug, Endpoint & i_endpoint);




/**
 * A structure used in mrwGetNextBusses to show which bus segments are connected
 * to the current one on the other side of a connector, and which end, either
 * source or endpoint, that connection uses.
 * T must be a child class of Bus.
 *
 */
template<class T>
struct busAndType_t
{
    T* bus;
    endpointType type;

    busAndType_t(T* i_bus, endpointType i_type) :
        bus(i_bus), type(i_type) {};

};


/**
 * Finds the bus segments that are attached to the plug/endpoint passed in on the other
 * side of a connector. This version must be used when there is 1 bus object
 * on one side of a connector, and on the other side there can be multiple bus objects.
 * Usually used in a recursive function.  See callers for examples.
 */
template<class T>
void mrwGetNextBusses(Plug* i_plug, Endpoint & i_endpoint, std::vector<busAndType_t<T> > & o_busses, Cable ** o_cable = NULL)
{
    std::string thisConnector = i_endpoint.id();
    std::string thisPin = i_endpoint.pin();
    std::vector<std::pair<std::string, Plug*> > next;  //nextConnector, nextPlug
    std::string nextConnector;
    Plug* nextPlug = NULL;
    Cable* cable = NULL;
    o_busses.clear();

#ifdef MRWDEBUG

    std::cout << ">> mrwGetNextBusses: thisConnector: " << thisConnector << " thisPin: "
              << thisPin << " on " << i_plug->path() << std::endl;
#endif

    if (o_cable) *o_cable = NULL;

    //Check if it's a cable connection first
    i_plug->getCableEndpoint(i_endpoint.id(), nextPlug, nextConnector, cable);

    if (nextPlug)
    {
        next.push_back(std::pair<std::string, Plug*>(nextConnector, nextPlug));

        //set the cable traversed, if user passed one in
        if (o_cable)
            *o_cable = cable;
    }

    //Check the plugs to get the next Plug object and connector name the bus will
    //go through
    //If the endpoint matches the child connector on the plug, then next bus
    //must be on the parent plug
    else if (thisConnector == i_plug->childConnector())
    {
        next.push_back(std::pair<std::string, Plug*>(i_plug->parentConnector(), i_plug->parent()));
    }
    else
    {
        //it must be a connector to a child, so next bus is on the child plug
        std::vector<Plug*>::iterator plug;

        for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
        {
            if (thisConnector == (*plug)->parentConnector())
            {
                next.push_back(std::pair<std::string, Plug*>((*plug)->childConnector(), *plug));
            }
        }
    }

    //Now look through the busses on the plug objects we found to find the exact bus object
    //that is connected, using the connector name & pin name

    if (!next.empty())
    {
        std::vector<std::pair<std::string, Plug*> >::iterator n;

        for (n=next.begin();n!=next.end();++n)
        {
            std::vector<Bus*>::iterator b;
            nextConnector = n->first;
            nextPlug = n->second;
            T* bus;

#ifdef MRWDEBUG
        std::cout << "mrwGetNextBusses: Found nextConnector " << nextPlug->path() << "/" << nextConnector << std::endl;
#endif

            //look through the busses on the next plug to find one that connects via
            //this connector and pin
            for (b=nextPlug->busses().begin();b!=nextPlug->busses().end();++b)
            {
                bus = (T*) *b;

                //if both the connector and pin matches, then we found a bus connection
                //try both the source & endpoint of the bus
                if ((nextConnector == bus->source().id()) && (thisPin == bus->source().pin()))
                {
                    o_busses.push_back(busAndType_t<T>(bus, SOURCE));
                }
                else if ((nextConnector == bus->endpoint().id()) && (thisPin == bus->endpoint().pin()))
                {
                    o_busses.push_back(busAndType_t<T>(bus, ENDPOINT));
                }
            }
        }
    }
    else
    {
        mrwLogger logger;
        logger() << "Couldn't find connection on other side of: Plug: " << i_plug->path() << " Connector: " <<
             i_endpoint.id() << " Pin: " << i_endpoint.pin();
        logger.info();
    }
}


/**
 * Finds the next bus object that connects to i_bus on the end io_type
 * T must be a child of class Bus.
 * This version can be used when a bus cannot split up on the other side
 * of a connector.
 */
template<class T>
T* mrwGetNextBus(T* i_bus, endpointType & io_type, Cable** o_cable = NULL)
{
    T* bus = NULL;
    Endpoint* e = (io_type == SOURCE) ? &i_bus->source() : &i_bus->endpoint();
    std::string thisConnector = e->id();
    std::string thisPin = e->pin();
    std::string nextConnector;
    Plug* nextPlug = NULL;
    Cable* cable = NULL;

#ifdef MRWDEBUG
    std::cout << ">> mrwGetNextBus: thisConnector: " << thisConnector << " thisPin: "
              << thisPin << " on " << i_bus->plug()->path() << std::endl;
#endif

    if (o_cable) *o_cable = NULL;

    //Check if it's a cable connection first
    i_bus->plug()->getCableEndpoint(thisConnector, nextPlug, nextConnector, cable);

    if (nextPlug)
    {
        //set the cable traversed, if user passed one in
        if (o_cable)
            *o_cable = cable;
    }
    else
    {
        //if the endpoint is a child connector, then the next bus
        //must be on the parent plug
        if (thisConnector == i_bus->plug()->childConnector())
        {
            nextConnector = i_bus->plug()->parentConnector();
            nextPlug = i_bus->plug()->parent();
        }
        else
        {
            //it must be a connector to a child, so next bus is on the child plug
            std::vector<Plug*>::iterator plug;

            for (plug=i_bus->plug()->children().begin();plug!=i_bus->plug()->children().end();++plug)
            {
                if (thisConnector == (*plug)->parentConnector())
                {
                    nextConnector = (*plug)->childConnector();
                    nextPlug = *plug;
                    break;
                }
            }
        }
    }


    if (nextPlug)
    {

#ifdef MRWDEBUG
        std::cout << "mrwGetNextBus: Found nextConnector " << nextPlug->path() << "/" << nextConnector << std::endl;
#endif

        //now find the bus on the plug we know it is
        std::vector<Bus*>::iterator b;

        for (b=nextPlug->busses().begin();b!=nextPlug->busses().end();++b)
        {
            bus = (T*) *b;

            //find the match on the connector and pin
            //we don't know which way the signals go, could either be the source or an endpoint

            if ((nextConnector == bus->source().id()) && (thisPin == bus->source().pin()))
            {
                //next time through, we'll want the endpoint of this bus
                io_type = ENDPOINT;
                break;
            }
            else if ((nextConnector == bus->endpoint().id()) && (thisPin == bus->endpoint().pin()))
            {
                //next time through, we want the source of this bus
                io_type = SOURCE;
                break;
            }
            else
                bus = NULL;
        }

    }
    else
    {
        mrwLogger logger;
        logger() << "Couldn't find connection on other side of: Plug: " << i_bus->plug()->path() << " Connector: "
                 << e->id() << " Pin: " << e->pin();
        logger.info();
    }

#ifdef MRWDEBUG
    if (bus)
        std::cout << "mrwGetNextBus: Found connection to " << bus->plug()->path() << "/" << nextConnector  << "/" << thisPin << std::endl;
    else
        std::cout << "mrwGetNextBus: Found no connection\n";
#endif

    return bus;
}


/**
 * Recursive function only called by itself and the higher level mrwMakeBusses.
 * Creates bus objects of type T for all of the busses of i_busType on i_plug.
 * Then, calls itself on all the child plugs of i_plug.
 */
template<class T>
void mrwMakeBusses(Plug* i_plug,
                   const std::string & i_busType,
                   std::vector<T*> & o_busses)
{

    std::vector<XMLElement> busGroups;
    std::vector<XMLElement> busses;
    std::vector<XMLElement>::iterator busGroup;
    std::vector<XMLElement>::iterator b;
    std::string busGroupType = i_busType + "s"; //an <fsi> bus element will be in <fsis>


    if (i_plug->type() == Plug::CARD)
    {
        i_plug->card().findPath("busses").getChildren(busGroups, busGroupType);

        for (busGroup=busGroups.begin();busGroup!=busGroups.end();++busGroup)
        {
            busGroup->getChildren(busses, i_busType);

            for (b=busses.begin();b!=busses.end();++b)
            {
                T* bus = new T(*b, i_plug);
                i_plug->busses().push_back(bus);
                o_busses.push_back(bus);

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

            }
        }
    }


    std::vector<Plug*>::iterator plug;
    for (plug=i_plug->children().begin();plug!=i_plug->children().end();++plug)
    {
        mrwMakeBusses(*plug, i_busType, o_busses);
    }



}


/**
 * Function that will walk all plugs passed in and fill in o_busses with
 * instances of class T for every bus of type i_busType.
 * Just fills in the id, unit, and pin for each endpoint.  If you need more,
 * then you'll have to implement the function separately.
 *
 * T must be derived from Bus or BusSegment.
 *
 * @param i_plugs - the top of the plugs hierarch (g_plugs)
 * @param i_busType - the bus type, like 'fsi', 'i2c', 'dmi'
 * @param o_busses - fill in with the bus objects for each bus
 */
template<class T>
void mrwMakeBusses(std::vector<Plug*> & i_plugs,
                   const std::string & i_busType,
                   std::vector<T*> & o_busses)
{
    std::vector<Plug*>::iterator plug;

    for (plug=i_plugs.begin();plug!=i_plugs.end();++plug)
    {
        mrwMakeBusses(*plug, i_busType, o_busses);
    }

}

#endif

