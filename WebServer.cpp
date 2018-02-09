/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "WebServer.hpp"

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
#ifdef BASECAMP_USEDNS
#ifdef DNSServer_h
	server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
#endif
#endif
}

void WebServer::begin(Configuration &configuration) {
	SPIFFS.begin();
	server.begin();

	server.on("/" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm_gz, index_htm_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});

	server.on("/basecamp.css" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", basecamp_css_gz, basecamp_css_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});

	server.on("/basecamp.js" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/js", basecamp_js_gz, basecamp_js_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});
	server.on("/logo.svg" , HTTP_GET, [](AsyncWebServerRequest * request)
	{
			AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg+xml", logo_svg_gz, logo_svg_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
	});

	server.on("/data.json" , HTTP_GET, [&configuration, this](AsyncWebServerRequest * request)
	{
			AsyncJsonResponse *response = new AsyncJsonResponse();
			DynamicJsonBuffer _jsonBuffer;

			JsonObject &_jsonData = response->getRoot();
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
			// NOTE: AsyncServer.send(ptr* foo) deletes `response` after async send.
			// As this is not documented in the header there: thanks for nothing.
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

			for (int i = 0; i < request->params(); i++)
			{
				AsyncWebParameter *webParameter = request->getParam(i);
				if (webParameter->isPost() && webParameter->value().length() != 0)
				{
						configuration.set(webParameter->name().c_str(), webParameter->value().c_str());
				}
			}

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

void WebServer::debugPrintRequest(AsyncWebServerRequest *request)
{
#ifdef DEBUG
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

		output << "Headers: " << std::endl;
		for (int i = 0; i < request->headers(); i++) {
				auto *header = request->getHeader(i);
				output << "\t";
				debugPrint(output, header);
				output << std::endl;
		}

		output << "Parameters: " << std::endl;
		for (int i = 0; i < request->params(); i++) {
				auto *parameter = request->getParam(i);
				output << "\t";
				if (parameter->isFile()) {
					output << "This is a file. FileSize: " << parameter->size() << std::endl << "\t\t";
				}
				debugPrint(output, parameter);
				output << std::endl;
		}

		Serial.println(output.str().c_str());
#endif
}

void WebServer::addInterfaceElement(const String &id, String element, String content, String parent, String configvariable) {
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
