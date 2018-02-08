#ifndef CaptiveRequestHandler_h
#define CaptiveRequestHandler_h
class CaptiveRequestHandler : public AsyncWebHandler {
	public:
		CaptiveRequestHandler() {
		}
		virtual ~CaptiveRequestHandler() {
		}

		bool canHandle(AsyncWebServerRequest *request) {
			//skip all basecamp related sources - handle all other requests and return the default html
			if (request->url() != "/basecamp.css" && 
					request->url() != "/basecamp.js" && 
					request->url() != "/data.json" && 
					request->url() != "/logo.svg" && 
					request->url() != "/submitconfig") {
				return true;
			} else {
				return false;
			}
		}

		void handleRequest(AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse_P(
					200, "text/html", index_htm_gz, index_htm_gz_len);
			response->addHeader("Content-Encoding", "gzip");
			request->send(response);
		}
};
#endif
