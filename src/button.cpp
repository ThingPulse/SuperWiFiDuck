#include "button.h"
#include <Arduino.h>

#if defined(TOUCH_ENABLED)

namespace button {
  
    boolean isButtonPressed = false;
    boolean isTouched = false;

    void begin() {
        touchAttachInterrupt(TOUCH_PIN, touchCallback, TOUCH_THRESHOLD);
    }

    void touchCallback() {
        isButtonPressed = true;
    }


    bool isPressed() { 
        if (isButtonPressed) {
            bool lastStatus = touchInterruptGetLastStatus(TOUCH_PIN);
            if (!isTouched && lastStatus) {
                Serial.println(" --- T1 Touched");
                isTouched = true;
                return false;
            } else if (isTouched && !lastStatus) {
                Serial.println(" --- T1 Released");
                isButtonPressed = false;
                isTouched = false;
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