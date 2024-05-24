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
            isButtonPressed = false;
            return true;
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