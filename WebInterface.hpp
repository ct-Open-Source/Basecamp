/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef WebInterface_h
#define WebInterface_h
#include <Arduino.h>
#include "map"


// FIXME: I nterfaceElement
class interfaceElement {
	public:
		interfaceElement(String p_id, String p_element, String p_content, String p_parent) {
			element = p_element;
			id = p_id;
			content = p_content;
			parent = p_parent;
		};
		struct cmp_str
		{
			bool operator()(const String &a, const String &b) const
			{
				return strcmp(a.c_str(), b.c_str()) < 0;
			}
		};

		const String& getId() const
		{
			return id;
		}

		// FIXME: GETTER and SETTER!
		String element;
		String id;
		String content;
		String parent;
		std::map<String, String, cmp_str> attributes;
		void setAttribute(String key, String value) {
			attributes[key] = value;
		};

// FIXME: Refs const param!
		String getAttribute(String key) const {
			auto found = attributes.find(key);
			if (found != attributes.end()) {
				return found->second;
			}

			// TODO: Say something
			return {""};
		}
};
#endif
