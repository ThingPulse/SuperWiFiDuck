#pragma once

#define WEBSERVER_CALLBACK \
server.on("/credits.html", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "text/html", credits_html_start, credits_html_end - credits_html_start);\
});\
server.on("/error404.html", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 404, "text/html", error404_html_start, error404_html_end - error404_html_start);\
});\
server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "text/html", index_html_start, index_html_end - index_html_start);\
});\
server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "application/javascript", index_js_start, index_js_end - index_js_start);\
});\
server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "application/javascript", script_js_start, script_js_end - script_js_start);\
});\
server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "text/html", settings_html_start, settings_html_end - settings_html_start);\
});\
server.on("/settings.js", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "application/javascript", settings_js_start, settings_js_end - settings_js_start);\
});\
server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "text/css", style_css_start, style_css_end - style_css_start);\
});\
server.on("/terminal.html", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "text/html", terminal_html_start, terminal_html_end - terminal_html_start);\
});\
server.on("/terminal.js", HTTP_GET, [](AsyncWebServerRequest* request) {\
	reply(request, 200, "application/javascript", terminal_js_start, terminal_js_end - terminal_js_start);\
});

extern const uint8_t credits_html_start[] asm("_binary_web_credits_html_start");
extern const uint8_t credits_html_end[] asm("_binary_web_credits_html_end");

extern const uint8_t error404_html_start[] asm("_binary_web_error404_html_start");
extern const uint8_t error404_html_end[] asm("_binary_web_error404_html_end");

extern const uint8_t index_html_start[] asm("_binary_web_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_web_index_html_end");

extern const uint8_t index_js_start[] asm("_binary_web_index_js_start");
extern const uint8_t index_js_end[] asm("_binary_web_index_js_end");

extern const uint8_t script_js_start[] asm("_binary_web_script_js_start");
extern const uint8_t script_js_end[] asm("_binary_web_script_js_end");

extern const uint8_t settings_html_start[] asm("_binary_web_settings_html_start");
extern const uint8_t settings_html_end[] asm("_binary_web_settings_html_end");

extern const uint8_t settings_js_start[] asm("_binary_web_settings_js_start");
extern const uint8_t settings_js_end[] asm("_binary_web_settings_js_end");

extern const uint8_t style_css_start[] asm("_binary_web_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_web_style_css_end");

extern const uint8_t terminal_html_start[] asm("_binary_web_terminal_html_start");
extern const uint8_t terminal_html_end[] asm("_binary_web_terminal_html_end");

extern const uint8_t terminal_js_start[] asm("_binary_web_terminal_js_start");
extern const uint8_t terminal_js_end[] asm("_binary_web_terminal_js_end");


