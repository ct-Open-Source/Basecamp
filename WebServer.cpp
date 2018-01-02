/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

#include "WebServer.hpp"
#include "debug.hpp"

void WebServer::begin(Configuration &configuration) {


	SPIFFS.begin();
	server = new AsyncWebServer(80);
	events = new AsyncEventSource("/events");
	server->addHandler(events);
	server->begin();

	if (!MDNS.begin("esp32")) {
		Serial.println("Error setting up MDNS responder!");
		while(1) {
			delay(1000);
		}
	}
	MDNS.addService("http", "tcp", 80);

	server->on("/" , HTTP_GET, [](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm_gz, index_htm_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
			});

	server->on("/basecamp.css" , HTTP_GET, [](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", basecamp_css_gz, basecamp_css_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
			});

	server->on("/basecamp.js" , HTTP_GET, [](AsyncWebServerRequest * request) {
			AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", basecamp_js_gz, basecamp_js_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
			});
	
	if (configuration.get("WifiConfigured") == "True") {
		server->on("/data.json" , HTTP_GET, [&configuration, this](AsyncWebServerRequest * request) {
				AsyncJsonResponse * response = new AsyncJsonResponse();
				//AsyncResponseStream *response = request->beginResponseStream("text/json");
				DynamicJsonBuffer _jsonBuffer;
				JsonObject& _jsonData = response->getRoot();
				//JsonObject& _jsonData = _jsonBuffer.createObject();
				JsonObject& meta = _jsonData.createNestedObject("meta");
				JsonArray& elements = _jsonData.createNestedArray("elements");
				for (auto const& x : interfaceElements)
				{
					JsonObject& element = elements.createNestedObject();
					element["element"] = _jsonBuffer.strdup(x.second->element);
					element["id"] = _jsonBuffer.strdup(x.second->id);
					element["content"] = _jsonBuffer.strdup(x.second->content);
					element["parent"] = _jsonBuffer.strdup(x.second->parent);
					JsonObject& attributes = element.createNestedObject("attributes");
					for (auto const& y : x.second->attributes) {
						attributes[_jsonBuffer.strdup(y.first)] = _jsonBuffer.strdup(y.second);
					}
					//if (attributes["configvariable"] != "" && attributes["type"] != "password") {
						//attributes["value"] = _jsonBuffer.strdup(configuration.get(attributes["configvariable"]));
					//};
					_jsonData.prettyPrintTo(Serial);
					//if (attributes["type"] == "password") {
						//attributes["placeholder"] = "      ";
					//};
				};

				_jsonData.prettyPrintTo(Serial);
				response->setLength();	
				request->send(response);

		});
	} else {
		DEBUG_PRINTLN("No Config found");
		server->on("/data.json" , HTTP_GET, [](AsyncWebServerRequest * request) {
				AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", initconf_json_gz, initconf_json_gz_len);
				response->addHeader("Content-Encoding", "gzip");
				request->send(response);
				});
	}
	server->on("/submitconfig", HTTP_POST, [&configuration](AsyncWebServerRequest * request) {
			int params = request->params();
			for(int i=0;i<params;i++){
				AsyncWebParameter* p = request->getParam(i);
				if(p->isPost()){
				configuration.set(p->name().c_str(), p->value().c_str());
				} 
			}
			configuration.save();
			request->send(201);

			delay(5000);
			esp_restart();
			});

	server->onNotFound([](AsyncWebServerRequest * request) {
#ifdef DEBUG
			Serial.printf("NOT_FOUND: ");
			if (request->method() == HTTP_GET)
			Serial.printf("GET");
			else if (request->method() == HTTP_POST)
			Serial.printf("POST");
			else if (request->method() == HTTP_DELETE)
			Serial.printf("DELETE");
			else if (request->method() == HTTP_PUT)
			Serial.printf("PUT");
			else if (request->method() == HTTP_PATCH)
			Serial.printf("PATCH");
			else if (request->method() == HTTP_HEAD)
			Serial.printf("HEAD");
			else if (request->method() == HTTP_OPTIONS)
			Serial.printf("OPTIONS");
			else
			Serial.printf("UNKNOWN");
			Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

			if (request->contentLength()) {
				Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
				Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
			}

			int headers = request->headers();
			int i;
			for (i = 0; i < headers; i++) {
				AsyncWebHeader* h = request->getHeader(i);
				Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
			}

			int params = request->params();
			for (i = 0; i < params; i++) {
				AsyncWebParameter* p = request->getParam(i);
				if (p->isFile()) {
					Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
				} else if (p->isPost()) {
					Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
				} else {
					Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
				}
			}
#endif
			request->send(404);

	});

}
void WebServer::addInterfaceElement(String id, String element, String content, String parent, String configvariable) {
	interfaceElement* newElement; 
	newElement = new interfaceElement(id, element, content, parent);
	newElement->setAttribute("configvariable", configvariable);
	interfaceElements[id.c_str()] = newElement;
};

//interfaceElement WebServer::getInterfaceElement(String id) {
	//for (std::map< it =  interfaceElements.begin(); it != _interfaceElements.end(); ++it)
	//{
		////set(it->key, it->value);
		//interfaceElement currentElement = it->value;
		//if (currentElement.id == id) {
			//return currentElement;
		//}

	//}
//};
void WebServer::setInterfaceElementAttribute(String id, String key, String value) {
	interfaceElements[id.c_str()]->setAttribute(key, value);
};
