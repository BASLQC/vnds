#include "xml_parser.h"
#include "filehandle.h"

using namespace std;

//-----------------------------------------------------------------------------

XmlNode::XmlNode()
: text(NULL), type(NODE_ELEMENT)
{

}
XmlNode::~XmlNode() {
	u32 len = children.size();
	XmlNode* nodes[len];
	for (u32 n = 0; n < len; n++) {
		nodes[n] = children[n];
	}
	children.clear();
	for (u32 n = 0; n < len; n++) {
		delete nodes[n];
	}
}

XmlNode& XmlNode::AddChild() {
	children.push_back(new XmlNode());
	return *children[children.size()-1];
}

const char* XmlNode::GetAttribute(const char* name) {
	for (u32 n = 0; n < attNames.size(); n++) {
		if (strcmp(attNames[n], name) == 0) {
			return attValues[n];
		}
	}
	return NULL;
}

const char* XmlNode::GetTextContent() {
	return children.size() > 0 ? children[0]->text : NULL;
}

XmlNode* XmlNode::GetChild(const char* path) {
	int pathL = strlen(path);
	char temp[pathL+1];
	strcpy(temp, path);
	temp[pathL] = '\0';

	XmlNode* current = this;

	char* s = strtok(temp, "/");
	while (s) {
		vector<XmlNode*>::iterator it = current->children.begin();
		while (it != current->children.end()) {
			XmlNode* node = *it;
			if (node->type == NODE_ELEMENT && strcmp(node->text, s) == 0) {
				current = node;
				break;
			}
			++it;
		}
		if (it == current->children.end()) {
			return NULL;
		}
		s = strtok(NULL, "/");
	}
	return current;
}

//-----------------------------------------------------------------------------

XmlFile::XmlFile()
: root(NULL), buffer(NULL), bufferL(0)
{

}
XmlFile::~XmlFile() {
	Close();
}

XmlNode* XmlFile::Open(const char* filename) {
	FileHandle* fh = fhOpen(filename);
	if (!fh) {
		return false;
	}

	Close();
	buffer = new char[fh->length+5];

	fh->ReadFully(buffer);
	for (u32 n = fh->length; n < fh->length+5; n++) {
		buffer[n] = '\0';
	}
	fhClose(fh);

	return Parse();
}

void XmlFile::Close() {
	if (buffer) delete[] buffer;
	if (root) delete root;

	buffer = NULL;
	bufferL = 0;
	root = NULL;
}

#define TOP (stack.back())

#define PARSE_ERR { \
	if (root) { delete root; } \
	return NULL; } \

#define INCR { \
	n++; \
	if (b[n] == '\0') { PARSE_ERR } } \

#define SPACES \
	{ while (isspace(b[n])) INCR } \

#define NOT_SPACES \
	{ while (!isspace(b[n])) INCR } \

XmlNode* XmlFile::Parse() {
	bool inElem = false;
	char* b = buffer;
	u32 n = 0;

	XmlNode* root = NULL;

	//consoleDemoInit();
	//iprintf("Parsing...\n");
	//waitForAnyKey();

	vector<XmlNode*> stack;
	while (b[n] != '\0') {
		if (!inElem) {
			if (b[n] == '<' && b[n+1] == '/') {
				b[n] = '\0';
				while (b[n] != '>') INCR

				stack.pop_back();
				if (stack.size() == 0) {
					break; //Root popped, bail
				}
			} else if (b[n] == '<') {
				b[n] = '\0';
				n++;

				char* name = b+n;
				while (b[n] != '>' && !isspace(b[n])) INCR
				n--;

				if (stack.size() == 0) {
					if (root != NULL) PARSE_ERR
					stack.push_back(root = new XmlNode());
				} else {
					stack.push_back(&TOP->AddChild());
				}

				TOP->text = name;
				inElem = true;

				//iprintf("%d %c%c%c%c\n", stack.size(), name[0], name[1], name[2], name[3]);
				//waitForAnyKey();
			} else {
				char* txt = b+n;
				bool nonSpace = false;
				while (b[n] != '\0' && b[n] != '<') {
					if (!isspace(b[n])) nonSpace = true;
					n++;
				}

				if (nonSpace) {
					if (!root) PARSE_ERR

					stack.push_back(&TOP->AddChild());
					TOP->text = txt;
					TOP->type = NODE_TEXT;
					stack.pop_back();
				}
				n--;
			}
		} else {
			if (b[n] == '/' && b[n+1] == '>') {
				b[n] = '\0';
				n++;
				inElem = false;
				stack.pop_back();
			} else if (b[n] == '>') {
				b[n] = '\0';
				inElem = false;
			} else if (!isspace(b[n])) {
				char* name = b+n;
				while (b[n] != '=' && !isspace(b[n])) INCR
				if (isspace(b[n])) b[n] = '\0';
				SPACES
				if (b[n] != '=') PARSE_ERR
				b[n] = '\0';
				n++;
				SPACES

				bool quoted = false;
				if (b[n] == '\"') {
					quoted = true;
					INCR
				}
				char* value = b+n;
				while ((quoted && b[n] != '\"') || (!quoted && !isspace(b[n]))) {
					if (b[n] == '\\') n++;
					INCR
				}
				if (quoted) b[n] = '\0';

				unescapeString(value);
				TOP->attNames.push_back(name);
				TOP->attValues.push_back(value);
			} else {
				b[n] = '\0';
			}
		}
		n++;
	}
	return root;
}

//-----------------------------------------------------------------------------
