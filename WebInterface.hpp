/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#ifndef WebInterface_h
#define WebInterface_h
#include <map>

// TODO: Discuss if this could not be modified to a struct as there is no
// real programatically-wise logic inside this clas.
class InterfaceElement {
	public:
		InterfaceElement(String p_id, String p_element, String p_content, String p_parent) {
			element = std::move(p_element);
			id = std::move(p_id);
			content = std::move(p_content);
			parent = std::move(p_parent);
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

		String element;
		String id;
		String content;
		String parent;
		std::map<String, String, cmp_str> attributes;

		void setAttribute(String key, String value) {
			attributes[key] = value;
		};

		// Return value for `key` or "" if `key` is not found.
		String getAttribute(const String &key) const {
			auto found = attributes.find(key);
			if (found != attributes.end()) {
				return found->second;
			}

			// TODO: What shall we really return if not found?
			// As we are not allowed to throw, maybe we shall change this function
			// to something like `getOr(const String &key, String default = "")`?
			return {""};
		}
};
#endif
