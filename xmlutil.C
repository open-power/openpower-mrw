/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: xmlutil.C $                                                   */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/xmlutil.C 2$

//******************************************************************************
// xmlutil.C
//
//  Created on: Jul 22, 2011
//      Author: hlava
//******************************************************************************
#include <xmlutil.H>
#include <libxml/xpath.h>
#include <libxml/xinclude.h>
#include <libxml/xpathInternals.h>

using namespace std;

bool g_xmlutil_debug = false;

//******************************************************************************
// debugPrint
//******************************************************************************
static void debugPrint(const char* fmt, ...)
{
	if (g_xmlutil_debug)
	{
		va_list arg_ptr;
		va_start(arg_ptr, fmt);
		vfprintf(stderr, fmt, arg_ptr);
		fflush(NULL);
	}
}

//******************************************************************************
// strReplace
//******************************************************************************
static void strReplace(string& str, const std::string& fromStr, const std::string& toStr)
{
	size_t pos = 0;
	while((pos = str.find(fromStr, pos)) != string::npos)
	{
		str.replace(pos, fromStr.length(), toStr);
		pos += toStr.length();
	}
}

//==============================================================================
//
//
// CLASS: XMLElement
//
//
//==============================================================================

//******************************************************************************
// getName
//******************************************************************************
string XMLElement::getName() const
{
	string outstr;
	if (ivNode && ivNode->type == XML_ELEMENT_NODE)
	{
		outstr = (char*)ivNode->name;
    }
    return(outstr);
}

//******************************************************************************
// getValue
//******************************************************************************
string XMLElement::getValue() const
{
	string outstr;
	if (ivNode && ivNode->children && ivNode->children->type == XML_TEXT_NODE)
	{
		outstr = (char*)ivNode->children->content;
    }
    return(outstr);
}

//******************************************************************************
// getChildValue
//******************************************************************************
string XMLElement::getChildValue(const string i_name) const
{
	string outstr;
	string name;
	XMLElement child_elmt;
	if (ivNode)
	{
		xmlNode* cur_node = NULL;

		for (cur_node = ivNode->children; cur_node; cur_node = cur_node->next)
		{
			child_elmt.setNode(cur_node);
			name = child_elmt.getName();
			if (name == i_name)
			{
				outstr = child_elmt.getValue();
				break;
			}
		}
    }
    return(outstr);
}

//******************************************************************************
// getAttributes
//******************************************************************************
void XMLElement::getAttributes(vector<XMLAttribute>& o_list) const
{
	xmlAttr* attr;
	XMLAttribute xml_attr;

	o_list.clear();
	if (ivNode == NULL) { return; }

	for (attr = ivNode->properties; attr; attr = attr->next)
	{
		xml_attr.iv_name = (char*)attr->name;
		xml_attr.iv_value = (char*)attr->children->content;
		o_list.push_back(xml_attr);
	}
}
//******************************************************************************
// getAttributeValue
//******************************************************************************
string XMLElement::getAttributeValue(const string& i_name) const
{
	string outstr;
	vector<XMLAttribute> attrs;

	getAttributes(attrs);

	for(vector<XMLAttribute>::iterator p = attrs.begin(); p != attrs.end(); ++p)
	{
		if (p->iv_name == i_name)
		{
			outstr = p->iv_value;
			break;
		}
	}

	return(outstr);
}

//******************************************************************************
// getChildren
//******************************************************************************
void XMLElement::getChildren(vector<XMLElement>& o_list, const std::string& i_child_name ) const
{
	xmlNode* cur_node = NULL;
	XMLElement xml_elmt;

	o_list.clear();
	if (ivNode == NULL) { return; }

	for (cur_node = ivNode->children; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			xml_elmt.setNode(cur_node);
			if ((i_child_name == "") || (i_child_name == xml_elmt.getName()))
			{
				o_list.push_back(xml_elmt);
			}
		}
	}
}

//******************************************************************************
// getChild
//******************************************************************************
XMLElement XMLElement::getChild(const std::string& i_name, const int i_index) const
{
	XMLElement elmt;
	vector<XMLElement> list;
	vector<XMLElement>::iterator p;
	int found_pos = 0;

	getChildren(list);
	for(p = list.begin(); p != list.end(); ++p)
	{
		if (i_name.empty() || (i_name == p->getName()))
		{
			if (found_pos == i_index)
			{
				elmt = *p;
				break;
			}
			++found_pos;
		}
	}
	return(elmt);
}

//******************************************************************************
// find
//******************************************************************************
XMLElement XMLElement::find(const std::string& i_name,
		const std::string& i_child_name,
		const std::string& i_child_value) const
{
	XMLElement elmt;
	vector<XMLElement> list;
	vector<XMLElement>::iterator p;


	if (i_name.empty() || i_child_name.empty())
	    return elmt;


	debugPrint(">find() called for element %s\n", (char*)getName().c_str());
	getChildren(list);

	// See if I fit the bill....
	if (getName() == i_name)
	{
		for(p = list.begin(); p != list.end(); ++p)
		{
			if ( (i_child_name == p->getName()) &&
				 (i_child_value.empty() ||
				  (i_child_value == p->getValue())) )
			{
				elmt = *this;
				break;
			}
		}
		if (!elmt.empty())
		{
			debugPrint("<find() returning successfully with *this\n");
			return(elmt);
		}
	}

	// If we reach this point, we have to search our descendants...
	for (p = list.begin(); p != list.end(); ++p)
	{
		elmt = p->find(i_name, i_child_name, i_child_value);
		if (!elmt.empty())
		{
			debugPrint("   find successful in child element\n");
			break;
		}
	}

	debugPrint("<find returning element \"%s\"\n", elmt.getName().c_str());
	return(elmt);

}

//******************************************************************************
// findPath
//******************************************************************************
XMLElement XMLElement::findPath(const std::string& i_path_name) const
{
	XMLElement elmt;
	size_t x;
	debugPrint(">findPath(%s)\n", i_path_name.c_str());

	string child_name(i_path_name);
	string remaining_path;
	x = child_name.find('/');
	if (x != string::npos)
	{
		remaining_path = child_name.substr(x+1);
		child_name = child_name.substr(0, x);
	}

	elmt = getChild(child_name);
	if (!elmt.empty())
	{
		if (!remaining_path.empty())
		{
			// Still more digging to do...
			elmt = elmt.findPath(remaining_path);
		}
	}

	debugPrint("<findPath() returning element \"%s\"\n", elmt.getName().c_str());
	return(elmt);
}

//******************************************************************************
// registerNamespaces (non-member helper)
//******************************************************************************
unsigned int registerNamespace(const string& i_ns, xmlXPathContext* i_context)
{
	debugPrint(">registerNamespace()\n");
	string ns(i_ns);

	if (i_ns.empty())
    {
		// Try env var...
        const char* n = getenv("XML_NAMESPACE");
        if (n)
            ns = n;
    }

    if (!ns.empty())
    {
        debugPrint("   namespace string: \"%s\"\n", ns.c_str());

        size_t pos = ns.find_first_of('=');
        if (pos == std::string::npos)
        {
            debugPrint("   Invalid namespace string\n");
            debugPrint("   Need something like  \"vpd=http://vpd.namespace\"\n");
            debugPrint("<registerNamespace()\n");
            return 1;
        }

        string prefix = ns.substr(0, pos);
        string href = ns.substr(pos+1, ns.length() - pos);

        debugPrint("   prefix = %s, href = %s\n", prefix.c_str(), href.c_str());

        if (!prefix.empty() && !href.empty())
        {
            if (xmlXPathRegisterNs(i_context,
                                   BAD_CAST prefix.c_str(),
                                   BAD_CAST href.c_str()) != 0)
            {
                debugPrint("   Failed to register namespace\n");
                debugPrint("<registerNamespace()\n");
                return 1;
            }
            debugPrint("   namespace registered OK\n");
        }

    }
    debugPrint("<registerNamespace()\n");
    return 0;
}

//******************************************************************************
// findXPath (set)
//******************************************************************************
void XMLElement::findXPath(XMLTree& i_tree, const std::string& i_xpath, vector<XMLElement>& o_elmts) const
{
	debugPrint(">findXPath(%s)\n", i_xpath.c_str());
	xmlXPathContext* context;
	xmlXPathObject* result;
	xmlNodeSet* nodeset;
	XMLElement elmt;

	o_elmts.clear();

	// Set up the context and namespace (if any)
	context = xmlXPathNewContext(i_tree.ivDoc);
	registerNamespace("", context); // Register env var namespace (if any)
	vector<string>::iterator p;
	for(p = i_tree.ivNamespaces.begin(); p != i_tree.ivNamespaces.end(); ++p)
	{
		if (registerNamespace(*p, context))
		{
			debugPrint("   unable to register namespace\n");
			debugPrint("<findXPath(%s)\n", i_xpath.c_str());
			return;
		}
	}

	// Do the XPath processing
	result = xmlXPathEvalExpression((const xmlChar*)i_xpath.c_str(), context);
	if (result)
	{
		nodeset = result->nodesetval;
		debugPrint("   xmlXPathEvalExpression returned result.  Number of nodes=%d\n", nodeset->nodeNr);
		for (int i=0; i < nodeset->nodeNr; ++i)
		{
			elmt.setNode( nodeset->nodeTab[i] );
			o_elmts.push_back(elmt);
		}
	}
	debugPrint("<findXPath(%s)\n", i_xpath.c_str());
}

//******************************************************************************
// findXPath (single)
//******************************************************************************
XMLElement XMLElement::findXPath(XMLTree& i_tree, const std::string& i_xpath) const
{
	XMLElement return_elmt;
	vector<XMLElement> elmts;
	findXPath(i_tree, i_xpath, elmts);
	if (elmts.begin() != elmts.end())
	{
		return_elmt = *(elmts.begin());
	}
	return(return_elmt);
}


//==============================================================================
//
//
// CLASS: XMLTree
//
//
//==============================================================================

//******************************************************************************
// XMLTree constructor
//******************************************************************************
XMLTree::XMLTree()
: ivDoc(NULL)
, ivRoot(NULL)
{ }

//******************************************************************************
// XMLTree destructor
//******************************************************************************
XMLTree::~XMLTree()
{
	clear();
}

//******************************************************************************
// clear
//******************************************************************************
void XMLTree::clear()
{
	if (ivDoc)
	{
		// Free the document
	    xmlFreeDoc(ivDoc);

	    // Clean up any other parser allocations
	    xmlCleanupParser();

	    ivDoc = NULL;
	    ivRoot = NULL;
	}
}

//******************************************************************************
// load
//******************************************************************************
unsigned int XMLTree::load(const string& i_filename)
{
	unsigned int rc = 0;

	clear();

    // Parse the file and get the DOM
    ivDoc = xmlReadFile(i_filename.c_str(), NULL, XML_PARSE_NOBLANKS);
    if (ivDoc)
   	{
    	// Process xincludes
    	if (xmlXIncludeProcessFlags(ivDoc, 0) < 0)
    	{
    		rc = 1;
    	}
    	else
    	{
    		// Get the root element node
    		ivRoot = xmlDocGetRootElement(ivDoc);

    		// Get the namespaces (if any).  I haven't figured out a way to
    		// retrieve this via libxml2, so it's brute force...
    		string cmd = "grep xmlns: ";
    		cmd += i_filename;
    		cmd += " | head -1";
    		FILE* fp = popen(cmd.c_str(), "r");
    		char buf[2048];
    		string instr;
    		string ns;
    		if (fp)
    		{
    			fseek(fp , 0 ,SEEK_SET);
                fgets(buf, sizeof(buf), fp);
                pclose(fp);
                instr = buf;
                size_t x, y;
                while(!instr.empty())
                {
                	x = instr.find("xmlns:");
                	if (x == string::npos) { break; }
               		instr = instr.substr(x+6);
           			y = instr.find("\"");
           			if (y == string::npos) { break; }
           			x = instr.find("\"", y+1);
           			if (x == string::npos) { break; }
           			ns = instr.substr(0, x+1);
           			instr = instr.substr(x+2);
           			strReplace(ns, "\"", "");
           			ivNamespaces.push_back(ns);
                }
    		}

    	}
   	}
    else
    {
    	rc = 1;
    }
    return(rc);
}

//******************************************************************************
// getRoot
//******************************************************************************
void XMLTree::getRoot(XMLElement& o_root) const
{
	o_root.setNode(ivRoot);
}

//******************************************************************************
// removeElement
//******************************************************************************
void XMLTree::removeElement(XMLElement& io_elmt)
{
	xmlUnlinkNode(io_elmt.ivNode);
	xmlFreeNode(io_elmt.ivNode);
	io_elmt.ivNode = NULL;
}

//******************************************************************************
// addElement
//******************************************************************************
void XMLTree::addElement(XMLElement& from_elmt, XMLElement& to_elmt, XMLTree::elmt_position_t rel_pos)
{
	// Make a copy of the from_elmt node and all its children
	xmlNodePtr new_from = xmlCopyNode(from_elmt.ivNode, 1);

	// Add the copy in the appropriate spot
	switch(rel_pos)
	{
	case SIBLING_BEFORE:
		xmlAddPrevSibling(to_elmt.ivNode, new_from);
		break;
	case SIBLING_AFTER:
		xmlAddNextSibling(to_elmt.ivNode, new_from);
		break;
	case CHILD:
	default:
		xmlAddChild(to_elmt.ivNode, new_from);
		break;
	};
}

//******************************************************************************
// moveElement
//******************************************************************************
void XMLTree::moveElement(XMLElement& from_elmt, XMLElement& to_elmt, XMLTree::elmt_position_t rel_pos)
{
	// Make a copy of the from_elmt node and all its children
	xmlNodePtr new_from = xmlCopyNode(from_elmt.ivNode, 1);

	// Add the copy in the appropriate spot
	switch(rel_pos)
	{
	case SIBLING_BEFORE:
		xmlAddPrevSibling(to_elmt.ivNode, new_from);
		break;
	case SIBLING_AFTER:
		xmlAddNextSibling(to_elmt.ivNode, new_from);
		break;
	case CHILD:
	default:
		xmlAddChild(to_elmt.ivNode, new_from);
		break;
	};

	// Delete the old from_elmt and then set it to the new node
	removeElement(from_elmt);
	from_elmt.ivNode = new_from;
}

//******************************************************************************
// save
//******************************************************************************
unsigned int XMLTree::save(const std::string& filename)
{
	int save_rc = xmlSaveFormatFileEnc(filename.c_str(), ivDoc, "UTF-8", 1);
	if (save_rc > 0)
		return(0);
	else
		return(1);
}
