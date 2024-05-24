/*
   This software is licensed under the MIT License. See the license file for details.
   Source: https://github.com/spacehuhntech/WiFiDuck
 */

#include "settings.h"

#include "spiffs.h"
#include "debug.h"
#include "config.h"
#include "eeprom0.h"

#define SETTINGS_ADDRES 1
#define SETTINGS_MAGIC_NUM 1234567891

namespace settings {
    // ===== PRIVATE ===== //
    typedef struct settings_t {
        uint32_t magic_num;

        char ssid[33];
        char password[65];
        char channel[5];
        char autorun[65];
        char wifimode[5];
        char buttonscript[65];
    } settings_t;

    settings_t data;

    // ===== PUBLIC ====== //
    void begin() {
        eeprom::begin();
        load();
    }

    void load() {
        eeprom::getObject(SETTINGS_ADDRES, data);
        if (data.magic_num != SETTINGS_MAGIC_NUM) reset();

        if (data.ssid[32] != 0) setSSID(WIFI_SSID);
        if (data.password[64] != 0) setPassword(WIFI_PASSWORD);
        if (data.channel[4] != 0) setChannel(WIFI_CHANNEL);
        if (data.autorun[64] != 0) setAutorun("");
        if (data.buttonscript[64] != 0) setButtonScript("");
    }

    void reset() {
        debugln("Resetting Settings");
        data.magic_num = SETTINGS_MAGIC_NUM;
        setSSID(WIFI_SSID);
        setPassword(WIFI_PASSWORD);
        setChannel(WIFI_CHANNEL);
        setWiFiMode(WIFI_MODE);
    }

    void save() {
        debugln("Saving Settings");
        eeprom::saveObject(SETTINGS_ADDRES, data);
    }

    String toString() {
        String s;

        s += "ssid=";
        s += getSSID();
        s += "\n";
        s += "password=";
        s += getPassword();
        s += "\n";
        s += "channel=";
        s += getChannel();
        s += "\n";
        s += "autorun=";
        s += getAutorun();
        s += "\n";
        s += "buttonScript=";
        s += getButtonScript();
        s += "\n";
        s += "wifi_mode=";
        s += getWiFiMode();
        s += "\n";

        return s;
    }

    const char* getSSID() {
        return data.ssid;
    }

    const char* getPassword() {
        return data.password;
    }

    const char* getChannel() {
        return data.channel;
    }

    const char* getWiFiMode() {
        return data.wifimode;
    }

    int getChannelNum() {
        if (strcmp(data.channel, "auto") != 0) return atoi(data.channel);
        return 1;
    }

    const char* getAutorun() {
        return data.autorun;
    }

    const char* getButtonScript() {
        return data.buttonscript;
    }

    void set(const char* name, const char* value) {
        if (strcmp(name, "ssid") == 0) {
            setSSID(value);
        } else if (strcmp(name, "password") == 0) {
            setPassword(value);
        } else if (strcmp(name, "channel") == 0) {
            setChannel(value);
        } else if (strcmp(name, "autorun") == 0) {
            setAutorun(value);
        } else if (strcmp(name, "buttonscript") == 0) {
            setButtonScript(value);
        } else if (strcmp(name, "wifi_mode") == 0) {
            setWiFiMode(value);
        }
    }

    void setSSID(const char* ssid) {
        if (ssid) {
            memset(data.ssid, 0, 33);
            strncpy(data.ssid, ssid, 32);

            save();
        }
    }

    void setPassword(const char* password) {
        if (password && (strlen(password) >= 8)) {
            memset(data.password, 0, 65);
            strncpy(data.password, password, 64);

            save();
        }
    }

    void setChannel(const char* channel) {
        if (channel && ((strcmp(channel, "auto") == 0) || ((atoi(channel) >= 1) && (atoi(channel) <= 13)))) {
            memset(data.channel, 0, 5);
            strncpy(data.channel, channel, 4);

            save();
        }
    }

    void setWiFiMode(const char* wifimode) {
        if (wifimode && ((strcmp(wifimode, "ap") == 0) || (strcmp(wifimode, "sta") == 0))) {
            memset(data.wifimode, 0, 5);
            strncpy(data.wifimode, wifimode, 4);

            save();
        }
    }

    void setAutorun(const char* autorun) {
        if (autorun) {
            memset(data.autorun, 0, 65);
            strncpy(data.autorun, autorun, 64);

            save();
        }
    }

    void setButtonScript(const char* buttonscript) {
        if (buttonscript) {
            memset(data.buttonscript, 0, 65);
            strncpy(data.buttonscript, buttonscript, 64);

            save();
        }
    }
}