/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: xmlutil.H $                                                   */
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
// $Source: fips760 fsp/src/mrw/xml/consumers/common/xmlutil.H 1$

//******************************************************************************
// xmlutil.H
//
// XML utility classes and functions for use within build-time tools.
//
// These are a lightweight, object-oriented wrapper for the libxml2 parser and
// tree APIs.
//
//  Created on: Jul 22, 2011
//      Author: hlava
//******************************************************************************
#ifndef XMLUTIL_H_
#define XMLUTIL_H_
#include <string>
#include <vector>
#include <libxml/parser.h>
#include <libxml/tree.h>

// Set to true to get extra debug output to stderr...
extern bool g_xmlutil_debug;

//******************************************************************************
// XML Attribute
//******************************************************************************
struct XMLAttribute
{
    /**
     *  Constructor
     */
    XMLAttribute() : iv_name(""), iv_value("")
    {
    }

    /**
     * Data: Name of attribute
     */
    std::string iv_name;

    /**
     * Data: Value of attribute
     */
    std::string iv_value;
};

class XMLTree;  // forward declaration

//******************************************************************************
//
// CLASS: XMLElement
//
//******************************************************************************
class XMLElement
{
public:
	friend class XMLTree;

	/**
	 * Constructor
	 */
	XMLElement() : ivNode(NULL) { };

	/**
	 * Destructor
	 */
	virtual ~XMLElement() { };

	/**
	 * Test for "empty" element.
	 *
	 * @return true if this element is empty, otherwise false
	 */
	bool empty() const { return(ivNode == NULL); };

	/**
	 * Get the name of the element.
	 *
	 * @return element name
	 */
	std::string getName() const;

	/**
	 * Get the value of the element.
	 *
	 * @return element value
	 */
	std::string getValue() const;

	/**
	 * Get the value of the specific child element.
	 *
	 * @param i_name = the name of the child element
	 * @return element value
	 */
	std::string getChildValue(const std::string i_name) const;

	/**
	 * Get a list of the attributes of this element
	 *
	 * @param o_list = the list of attributes
	 */
	void getAttributes(std::vector<XMLAttribute>& o_list) const;

	/**
	 * Get an attribute value of this element
	 *
	 * @param i_name = the name of the attribute to return
	 * @return the specified attribute's value (empty if not found)
	 */
	std::string getAttributeValue(const std::string& i_name) const;

	/**
	 * Get a list of the child elements
	 *
	 * @param o_list = the list of child elements
	 * @param i_child_name = if specified, only return children with
	 *           the specified element name
	 */
	void getChildren(std::vector<XMLElement>& o_list, const std::string& i_child_name = "") const;

	/**
	 * Get N-th child element under this element having the specified
	 * name.
	 *
	 * Note that this call can be "chained" to easily find an element
	 * anywhere in the hierarchy when you know the "path" to it.
	 * For example: found_elmt = root_elmt.getChild("son",0).getChild("grandson",0);
	 *
	 * @param i_name = the name of the child element to get. If "" is
	 *                 passed then a child with any name is returned.
	 * @param i_index = the index (0..N) of the child element matching i_name
	 *                 to return.
	 * @return the child element found.  If no child element was found matching
	 *         the name/index specified, an empty element is returned.
	 */
	XMLElement getChild(const std::string& i_name, const int i_index = 0) const;

	/**
	 * Find a descendant element which has a child element with the specified
	 * name & value.
	 *
	 * For example, if this is a "dept" element containing "person" elements
	 * which include "employee_num" and you want to find the person element for
	 * emp# 123456:
	 *     person = dept.find("person", "employee_num", 123456);
	 *
	 * @param i_name = the name of the descendant element to find.
	 * @param i_child_name = the name of the child element to check of
	 * @param i_child_value = the value of i_child_name to look for
	 *              A value of "" indicates to find the first child with
	 *              the specified name.
	 * @return the descendant element found.  If no element was found matching
	 *         the criteria specified, an empty element is returned.
	 */
	XMLElement find(const std::string& i_name,
			const std::string& i_child_name,
			const std::string& i_child_value) const;

	/**
	 * Find a descendant element with the specified "path" to it.
	 *
	 * For example, if this is a "dept" element containing "person" elements
	 * which include "phone-nums" and you want to find the phone-nums element:
	 *     nums = dept.findByPath("person/phone-nums");
	 *
	 * @param i_path_name = the path to name of the descendant element to find.
	 * @return the descendant element found.  If no element was found matching
	 *         the criteria specified, an empty element is returned.
	 */
	XMLElement findPath(const std::string& i_path_name) const;

	/**
	 * Find all descendant elemenst with the specified "XPath".
	 *
	 * The XPath context for the search is the XMLElement instance on which
	 * it is called.  The XPath syntax is the standard XML XPath syntax.
	 *
	 * @param i_tree = the XMLTree containing this element
	 * @param i_xpath = the XPath identifying the element(s) to find.
	 * @param o_elmts = the elements found
	 */
	void findXPath(XMLTree& i_tree, const std::string& i_xpath, std::vector<XMLElement>& o_elmts) const;

	/**
	 * Find a descendant element with the specified "XPath" to it.
	 *
	 * The XPath context for the search is the XMLElement instance on which
	 * it is called.  The XPath syntax is the standard XML XPath syntax.
	 *
	 * In this function, it is assumed that the XPath identifies a single,
	 * unique element within the context.  If it matches more than one
	 * element, only the first one is returned.
	 *
	 * @param i_tree = the XMLTree containing this element
	 * @param i_xpath = the XPath identifying the element to find.
	 * @return the descendant element found.  If no element was found matching
	 *         the criteria specified, an empty element is returned.
	 */
	XMLElement findXPath(XMLTree& i_tree, const std::string& i_xpath) const;


private:
	void setNode(xmlNode* i_node) { ivNode = i_node; };

	xmlNode* ivNode;
};

//******************************************************************************
//
// CLASS: XMLTree
//
//******************************************************************************
class XMLTree
{
public:
	friend class XMLElement;

	/**
	 * Constructor
	 */
	XMLTree();

	/**
	 * Destructor
	 */
	virtual ~XMLTree();

	/**
	 * Load tree from XML file.
	 *
	 * @param i_filename = the name of the XML file
	 * @return 0 if successful, non-zero error code if failure
	 */
	unsigned int load(const std::string& i_filename);

	/**
	 * Get the root element of the tree.
	 *
	 * @param o_root = Pointer to root element or NULL if tree is empty
	 */
	void getRoot(XMLElement& o_root) const;

	//==========================================================================
	// The following methods should not be needed by normal FSP parser code,
	// but may be useful for other applications doing otherXML-related
	// manipulations.
	//==========================================================================

	/**
	 * Remove an element and all of its subelements from the tree.  Note that
	 * this actually modifies the XMLTree as opposed to the XMLElement
	 * destructor which only removes a reference to the element but leaves
	 * the tree unchanged.
	 *
	 * @param io_elmt = the element to be removed.  Upon return, this element will
	 *     be set to "empty"
	 */
	void removeElement(XMLElement& io_elmt);

	// Relative element position
	typedef enum
	{
		CHILD = 0,            // Child of specified element
		SIBLING_BEFORE = 1,   // Sibling before specified element
		SIBLING_AFTER  = 2    // Sibling after specified element
	} elmt_position_t;

	/**
	 * Add an element and all of its subelements to the tree.
	 *
	 * @param from_elmt = the element to be added.  A COPY of this element will be
	 *     added to this tree
	 * @param to_elmt = the element marking where it will be moved to
	 * @param rel_pos = the position relative to to_elmt that from_elmt will
	 *    be added to
	 */
	void addElement(XMLElement& from_elmt, XMLElement& to_elmt, XMLTree::elmt_position_t rel_pos);

	/**
	 * Move an element and all its subelements to a different location in the tree.
	 * Note that moveElement only operates within the same XMLTree.  Use addElement and
	 * removeElement to move elements between trees.
	 *
	 * @param from_elmt = the element to be moved
	 * @param to_elmt = the element marking where it will be moved to
	 * @param rel_pos = the position relative to to_elmt that from_elmt will
	 *    be moved to
	 */
	void moveElement(XMLElement& from_elmt, XMLElement& to_elmt, XMLTree::elmt_position_t rel_pos);

	/**
	 * Write the tree as an XML document to the specified file.
	 *
	 * @param filename = the name of the file
	 * @return 0 if successful, non-zero error code if failure
	 */
	unsigned int save(const std::string& filename);

private:
	void clear();

    xmlDoc* ivDoc;
    xmlNode* ivRoot;
    std::vector<std::string> ivNamespaces;
};

#endif /* XMLUTIL_H_ */
