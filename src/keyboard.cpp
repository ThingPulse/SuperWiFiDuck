/*
   This software is licensed under the MIT License. See the license file for details.
   Source: https://github.com/spacehuhntech/WiFiDuck
 */

/*
   This software is licensed under the MIT License. See the license file for details.
   Source: https://github.com/wasdwasd0105/SuperWiFiDuck
 */


#include "USBHID.h"


#include "keyboard.h"
#include "debug.h"



const uint8_t report_descriptor[] {
        //  Keyboard
        0x05, 0x01, //   USAGE_PAGE (Generic Desktop)  // 47
        0x09, 0x06, //   USAGE (Keyboard)
        0xa1, 0x01, //   COLLECTION (Application)
        0x85, 0x01, //   REPORT_ID (1)
        0x05, 0x07, //   USAGE_PAGE (Keyboard)

        0x19, 0xe0, //   USAGE_MINIMUM (Keyboard LeftControl)
        0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)
        0x15, 0x00, //   LOGICAL_MINIMUM (0)
        0x25, 0x01, //   LOGICAL_MAXIMUM (1)
        0x75, 0x01, //   REPORT_SIZE (1)

        0x95, 0x08, //   REPORT_COUNT (8)
        0x81, 0x02, //   INPUT (Data,Var,Abs)
        0x95, 0x01, //   REPORT_COUNT (1)
        0x75, 0x08, //   REPORT_SIZE (8)
        0x81, 0x03, //   INPUT (Cnst,Var,Abs)

        0x95, 0x06, //   REPORT_COUNT (6)
        0x75, 0x08, //   REPORT_SIZE (8)
        0x15, 0x00, //   LOGICAL_MINIMUM (0)
        0x25, 0x73, //   LOGICAL_MAXIMUM (115)
        0x05, 0x07, //   USAGE_PAGE (Keyboard)

        0x19, 0x00, //   USAGE_MINIMUM (Reserved (no event indicated))
        0x29, 0x73, //   USAGE_MAXIMUM (Keyboard Application)
        0x81, 0x00, //   INPUT (Data,Ary,Abs)
        0xc0,       //   END_COLLECTION
};


esp_err_t arduino_usb_event_post(esp_event_base_t event_base, int32_t event_id, void *event_data, size_t event_data_size, TickType_t ticks_to_wait);

esp_err_t arduino_usb_event_handler_register_with(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void *event_handler_arg);


hid_locale_t* HIDKeyboard::locale  { &locale_us };


HIDKeyboard::report HIDKeyboard::prev_report = report { KEY_NONE, KEY_NONE, { KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE } };

uint16_t HIDKeyboard::_onGetDescriptor(uint8_t* dst){
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}




HIDKeyboard::HIDKeyboard(): hid(){
    static bool initialized = false;
    if(!initialized){
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

void HIDKeyboard::begin(){
    hid.begin();
}


void HIDKeyboard::onEvent(esp_event_handler_t callback){
    onEvent(ARDUINO_USB_HID_KEYBOARD_ANY_EVENT, callback);
}
void HIDKeyboard::onEvent(arduino_usb_hid_keyboard_event_t event, esp_event_handler_t callback){
    arduino_usb_event_handler_register_with(ARDUINO_USB_HID_KEYBOARD_EVENTS, event, callback, this);
}

void HIDKeyboard::_onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len){
    Serial.println(report_id, HEX);
    if(report_id == HID_REPORT_ID_KEYBOARD){
        arduino_usb_hid_keyboard_event_data_t p;
        p.leds = buffer[0];
        
        arduino_usb_event_post(ARDUINO_USB_HID_KEYBOARD_EVENTS, ARDUINO_USB_HID_KEYBOARD_LED_EVENT, &p, sizeof(arduino_usb_hid_keyboard_event_data_t), portMAX_DELAY);
    }
}


HIDKeyboard::report HIDKeyboard::makeReport(uint8_t modifiers, uint8_t key1, uint8_t key2, uint8_t key3, uint8_t key4, uint8_t key5, uint8_t key6) {
        report k;

        k.modifiers = modifiers;

        k.reserved = 0x00;

        k.keys[0] = key1;
        k.keys[1] = key2;
        k.keys[2] = key3;
        k.keys[3] = key4;
        k.keys[4] = key5;
        k.keys[5] = key6;

        return k;
}


void HIDKeyboard::setLocale(hid_locale_t* locale) {
    this->locale = locale;
}

    

void HIDKeyboard::release() {
    prev_report = makeReport();
    send(&prev_report);
}




uint8_t HIDKeyboard::press(const char* strPtr) {
        // Convert string pointer into a byte pointer
        uint8_t* b = (uint8_t*)strPtr;

        // Key combinations (accent keys)
        // We have to check them first, because sometimes ASCII keys are in here
        for (uint8_t i = 0; i<locale->combinations_len; ++i) {
            uint8_t res = 0;

            // Read utf8 code and match it with the given data
            for (uint8_t j = 0; j<4; ++j) {
                uint8_t key_code = pgm_read_byte(locale->combinations + (i * 8) + j);

                if (key_code == 0) {
                    break;
                }

                if (key_code == b[j]) {
                    ++res;
                } else {
                    res = 0;
                    break;
                }
            }

            // If a match was found, read out the data and type it
            if (res > 0) {
                uint8_t comboModifiers = pgm_read_byte(locale->combinations + (i * 8) + 4);
                uint8_t comboKey       = pgm_read_byte(locale->combinations + (i * 8) + 5);

                uint8_t modifiers = pgm_read_byte(locale->combinations + (i * 8) + 6);
                uint8_t key       = pgm_read_byte(locale->combinations + (i * 8) + 7);

                pressKey(comboKey, comboModifiers);
                release();
                pressKey(key, modifiers);
                release();

                // Return the number of extra bytes we used from the string pointer
                return res-1;
            }
        }

        // ASCII
        if (b[0] < locale->ascii_len) {
            uint8_t modifiers = pgm_read_byte(locale->ascii + (b[0] * 2) + 0);
            uint8_t key       = pgm_read_byte(locale->ascii + (b[0] * 2) + 1);

            pressKey(key, modifiers);

            return 0;
        }
        
        // UTF8
        for (size_t i = 0; i<locale->utf8_len; ++i) {
            uint8_t res = 0;

            // Read utf8 code and match it with the given data
            for (uint8_t j = 0; j<4; ++j) {
                uint8_t key_code = pgm_read_byte(locale->utf8 + (i * 6) + j);

                if (key_code == 0) {
                    break;
                }

                if (key_code == b[j]) {
                    ++res;
                } else {
                    res = 0;
                    break;
                }
            }

            // If a match was found, read out the data and type it
            if (res > 0) {
                uint8_t modifiers = pgm_read_byte(locale->utf8 + (i * 6) + 4);
                uint8_t key       = pgm_read_byte(locale->utf8 + (i * 6) + 5);

                pressKey(key, modifiers);

                // Return the number of extra bytes we used from the string pointer
                return res-1;
            }
        }
        return 0;
}

void HIDKeyboard::send(report* k) {
#ifdef ENABLE_DEBUG
        debug("Sending Report [");
        for (uint8_t i = 0; i<6; ++i) {
            debug(String(prev_report.keys[i], HEX));
            debug(",");
        }
        debug("#");
        debug(String(prev_report.modifiers, HEX));
        debugln("]");
#endif // ENABLE_DEBUG
        hid.SendReport(1, (uint8_t*)k, sizeof(report));
}


void HIDKeyboard::pressKey(uint8_t key, uint8_t modifiers) {
        for (uint8_t i = 0; i<6; ++i) {
            if (prev_report.keys[i] == KEY_NONE) {
                prev_report.modifiers |= modifiers;
                prev_report.keys[i]    = key;
                send(&prev_report);
                return;
            }
        }
}

void HIDKeyboard::pressModifier(uint8_t key) {

    ESP_LOGI("", "%s", key);
        prev_report.modifiers |= key;

        send(&prev_report);
}



uint8_t HIDKeyboard::write(const char* c) {
        uint8_t res = press(c);

        release();

        return res;
}

void HIDKeyboard::write(const char* str, size_t len) {
        for (size_t i = 0; i<len; ++i) {
            i += write(&str[i]);
        }
}
