#include "button.h"
#include <Arduino.h>

#if defined(TOUCH_ENABLED)

namespace button {
  
    boolean isButtonPressed = false;

    void begin() {
        touchAttachInterrupt(TOUCH_PIN, touchCallback, TOUCH_THRESHOLD);
    }

    void touchCallback() {
        isButtonPressed = true;
    }


    bool isPressed() { 
        if (isButtonPressed) {
            if (touchInterruptGetLastStatus(TOUCH_PIN)) {
                Serial.println(" --- T1 Touched");
                return false;
            } else {
                Serial.println(" --- T1 Released");
                isButtonPressed = false;
                return true;
            }
        }
        return false;
    }
}
#else
namespace button {
    void begin() {}

    bool isPressed() { return false;}

    void isTouched() {};
}
#endif