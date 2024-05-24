#include <Arduino.h>

#include "config.h"
#include "debug.h"
#include "duckscript.h"
#include "duckparser.h"
#include "webserver.h"
#include "spiffs.h"
#include "settings.h"
#include "cli.h"
#include "USB.h"
#include "led.h"
#include "button.h"


void setup() {
    debug_init();
    duckparser::beginKeyboard();
    USB.begin();
    delay(200);
    spiffs::begin();
    settings::begin();
    cli::begin();
    webserver::begin();
    led::begin();
    button::begin();

    duckscript::run(settings::getAutorun());
}

void loop() {
    webserver::update();
    if (button::isPressed()) {
        duckscript::run(settings::getButtonScript());
        while(duckscript::isRunning()) {
            
        }
    }
    debug_update();
}