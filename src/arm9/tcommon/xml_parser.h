#ifndef T_XML_PARSER_H
#define T_XML_PARSER_H

/*
 * Extremely small XML parser
 * Features: None
 */

#include "common.h"
#include <vector>

enum XmlNodeType {
	NODE_ELEMENT,
	NODE_TEXT
};

class XmlNode {
	public:
		const char* text;
		XmlNodeType type;
		std::vector<const char*> attNames;
		std::vector<const char*> attValues;
		std::vector<XmlNode*> children;

		XmlNode();
		virtual ~XmlNode();

		XmlNode& AddChild();
		XmlNode* GetChild(const char* path);
		const char* GetAttribute(const char* name);
		const char* GetTextContent();

};

class XmlFile {
	private:
		XmlNode* root;
		char* buffer;
		int bufferL;

		XmlNode* Parse();

	public:
		XmlFile();
		virtual ~XmlFile();

		XmlNode* Open(const char* filename);
		void Close();

};

#endif
