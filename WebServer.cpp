/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */
#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "WebServer.hpp"
#include "debug.hpp"

namespace {
	template<typename NAMEVALUETYPE>
	void debugPrint(std::ostream &stream, NAMEVALUETYPE &nameAndValue)
	{
		stream << nameAndValue->name().c_str() << ": " << nameAndValue->value().c_str();
	}
}

WebServer::WebServer()
	: events("/events")
	, server(80)
{
	server.addHandler(&events);
}

void WebServer::begin(Configuration &configuration) {
	SPIFFS.begin();
	// TODO: "begin" is not making too much sense. Means "listen"?
	server.begin();

	if (!MDNS.begin("esp32")) {
		Serial.println("Error setting up MDNS responder!");
		while(1) {
			// FIXME: Why should we do this?
			delay(1000);
		}
	}

	// TODO: Again 80?
	MDNS.addService("http", "tcp", 80);

	server.on("/" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
		  DEBUG_PRINTLN("GET /");
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm_gz, index_htm_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});

	server.on("/basecamp.css" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
			DEBUG_PRINTLN("GET css");
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", basecamp_css_gz, basecamp_css_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});

	server.on("/basecamp.js" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
			DEBUG_PRINTLN("GET js");
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/js", basecamp_js_gz, basecamp_js_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});

	server.on("/data.json" , HTTP_GET, [&configuration, this](AsyncWebServerRequest * request)
	{
			DEBUG_PRINTLN("GET json");
		  debugPrintRequest(request);

			AsyncJsonResponse *response = new AsyncJsonResponse();
			DynamicJsonBuffer _jsonBuffer;

			JsonObject &_jsonData = response->getRoot();
			JsonObject &meta = _jsonData.createNestedObject("meta");
			meta["title"] = configuration.get("DeviceName");
			JsonArray &elements = _jsonData.createNestedArray("elements");

			for (const auto &interfaceElement : interfaceElements)
			{
				JsonObject &element = elements.createNestedObject();
				JsonObject &attributes = element.createNestedObject("attributes");
				element["element"] = _jsonBuffer.strdup(interfaceElement.element);
				element["id"] = _jsonBuffer.strdup(interfaceElement.id);
				element["content"] = _jsonBuffer.strdup(interfaceElement.content);
				element["parent"] = _jsonBuffer.strdup(interfaceElement.parent);

				for (const auto &attribute : interfaceElement.attributes)
				{
					attributes[attribute.first] = String{attribute.second};
				}

				if (interfaceElement.getAttribute("data-config").length() != 0)
				{
					if (interfaceElement.getAttribute("type")=="password")
					{
						attributes["placeholder"] = "Password unchanged";
						attributes["value"] = "";
					} else {
						attributes["value"] = String{configuration.get(interfaceElement.getAttribute("data-config"))};
					}
				}
			}
#ifdef DEBUG
			_jsonData.prettyPrintTo(Serial);
#endif
			response->setLength();
			// NOTE: AsyncServer.send(ptr* foo) deletes foo after async send.
			// As this is not documented in the header: thanks for nothing.
			request->send(response);
	});

	server.on("/submitconfig", HTTP_POST, [&configuration, this](AsyncWebServerRequest *request)
	{
			if (request->params() == 0) {
				DEBUG_PRINTLN("Refusing to take over an empty configuration submission.");
				request->send(500);
				return;
			}
			debugPrintRequest(request);

			// TODO: What type is params? iterate
			// FIXME: Template that with the debugOut
			for (int i = 0; i < request->params(); i++)
			{
				AsyncWebParameter *webParameter = request->getParam(i);
				if (webParameter->isPost() && webParameter->value().length() != 0)
				{
						configuration.set(webParameter->name().c_str(), webParameter->value().c_str());
				}
			}

// FIXME: RE-ENABLE!
			configuration.save();
			request->send(201);

			// Why? What is this magic value for?
			delay(2000);
			esp_restart();
	});

	server.onNotFound([this](AsyncWebServerRequest *request)
	{
#ifdef DEBUG
	  	DEBUG_PRINTLN("WebServer request not found: ");
			debugPrintRequest(request);
#endif
			request->send(404);
	});
}

// TODO: Operator <<
void WebServer::debugPrintRequest(AsyncWebServerRequest *request)
{
		/**
		 That AsyncWebServer code uses some strange bit-consstructs instead of enum
		 class. Also no const getter. As I refuse to bring that code to 21st century,
		 we have to live with it until someone brave fixes it.
		*/
		const std::map<WebRequestMethodComposite, std::string> requestMethods{
			{ HTTP_GET, "GET" },
			{ HTTP_POST, "POST" },
			{ HTTP_DELETE, "DELETE" },
			{ HTTP_PUT, "PUT" },
			{ HTTP_PATCH, "PATCH" },
			{ HTTP_HEAD, "HEAD" },
			{ HTTP_OPTIONS, "OPTIONS" },
		};

		std::ostringstream output;

		output << "Method: ";
		auto found = requestMethods.find(request->method());
		if (found != requestMethods.end()) {
			output << found->second;
		} else {
			output << "Unknown (" << static_cast<unsigned int>(request->method()) << ")";
		}

		output << std::endl;
		output << "URL: " << request->url().c_str() << std::endl;
		output << "Content-Length: " << request->contentLength() << std::endl;
		output << "Content-Type: " << request->contentType().c_str() << std::endl;

		// TODO: Is there an iterator on the "LinkedList" from that crap library?
		output << "Headers: " << std::endl;
		for (int i = 0; i < request->headers(); i++) {
				auto *header = request->getHeader(i);
//				output << "\t" << header->name().c_str() << ": " << header->value().c_str() << std::endl;
				output << "\t";
				debugPrint(output, header);
				output << std::endl;
		}

		// TODO: dto params(). BTW - what should "params" represent in a http request in this object?
		output << "Parameters: " << std::endl;
		for (int i = 0; i < request->params(); i++) {
				auto *parameter = request->getParam(i);
				output << "\t";
				// TODO: What is isFile()?
				if (parameter->isFile()) {
					output << "FileSize: " << parameter->size() << std::endl << "\t\t";
				}
				debugPrint(output, parameter);
				output << std::endl;
				//parameter->name().c_str() << ": " << parameter->value().c_str() << std::endl;
		}

		Serial.println(output.str().c_str());
}

void WebServer::addInterfaceElement(String id, String element, String content, String parent, String configvariable) {
	interfaceElements.emplace_back(id, std::move(element), std::move(content), std::move(parent));
	if (configvariable.length() != 0) {
		setInterfaceElementAttribute(id, "data-config", std::move(configvariable));
	}
}

void WebServer::setInterfaceElementAttribute(const String &id, const String &key, String value)
{
	for (auto &element : interfaceElements) {
		if (element.getId() == id) {
			element.setAttribute(key, std::move(value));
			return;
		}
	}
}
