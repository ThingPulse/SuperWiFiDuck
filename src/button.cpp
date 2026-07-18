#include "button.h"
#include <Arduino.h>

#if defined(TOUCH_ENABLED)

#ifndef TOUCH_DEBOUNCE_MS
#define TOUCH_DEBOUNCE_MS 50
#endif

namespace button {

    volatile boolean touchEventPending = false;
    boolean isTouched = false;
    boolean lastTouchStatus = false;
    unsigned long statusChangedAt = 0;

    void begin() {
        touchAttachInterrupt(TOUCH_PIN, touchCallback, TOUCH_THRESHOLD);
    }

    void touchCallback() {
        touchEventPending = true;
    }


    bool isPressed() {
        if (touchEventPending) {
            const bool currentStatus = touchInterruptGetLastStatus(TOUCH_PIN);
            const unsigned long now = millis();

            if (currentStatus != lastTouchStatus) {
                lastTouchStatus = currentStatus;
                statusChangedAt = now;
            }

            if ((now - statusChangedAt) < TOUCH_DEBOUNCE_MS) {
                return false;
            }

            if (!isTouched && currentStatus) {
                Serial.println(" --- T1 Touched");
                isTouched = true;
                return false;
            } else if (isTouched && !currentStatus) {
                Serial.println(" --- T1 Released");
                touchEventPending = false;
                isTouched = false;
                return true;
            } else if (!isTouched && !currentStatus) {
                // Ignore a noise pulse that did not remain active long enough.
                touchEventPending = false;
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
