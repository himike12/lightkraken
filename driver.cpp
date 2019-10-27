/*
* driver.cpp
*
*  Created on: Sep 17, 2019
*      Author: Tinic Uro
*/

#include <stdio.h>
#include <math.h>

extern "C" {
#include "gd32f10x.h"
}

#include "./main.h"
#include "./driver.h"
#include "./pwmtimer.h"
#include "./model.h"

namespace lightguy {

Driver &Driver::instance() {
    static Driver driver;
    if (!driver.initialized) {
        driver.initialized = true;
        driver.init();
    }
    return driver;
}

void Driver::setRGBWWCIE(size_t terminal, const rgbww &rgb) {

    maybeUpdateCIE();

    terminal %= terminalN;
    _rgbww[terminal] = rgb;

    switch(Model::instance().outputConfig()) {
    case Model::OUTPUT_CONFIG_DUAL_STRIP: {
    } break;
    case Model::OUTPUT_CONFIG_RGB_STRIP: {
        if (terminal == 0) {
            // TODO: Convert through XYZ color space
            setPulse(0 + 0, cie_lookup[_rgbww[terminal].r]);
            setPulse(0 + 1, cie_lookup[_rgbww[terminal].g]);
            setPulse(0 + 2, cie_lookup[_rgbww[terminal].b]);
        }
    } break;
    case Model::OUTPUT_CONFIG_RGB_DUAL_STRIP: {
        if (terminal == 0) {
            // TODO: Convert through XYZ color space
            setPulse(3 + 0, cie_lookup[_rgbww[terminal].r]);
            setPulse(0 + 1, cie_lookup[_rgbww[terminal].g]);
            setPulse(0 + 2, cie_lookup[_rgbww[terminal].b]);
        }
    } break;
    case Model::OUTPUT_CONFIG_RGBW_STRIP: {
        if (terminal == 0) {
            // TODO: Convert through XYZ color space
            setPulse(0, cie_lookup[_rgbww[terminal].r]);
            setPulse(1, cie_lookup[_rgbww[terminal].g]);
            setPulse(2, cie_lookup[_rgbww[terminal].b]);
            setPulse(3, cie_lookup[_rgbww[terminal].w]);
        }
    } break;
    case Model::OUTPUT_CONFIG_RGB_RGB: {
        // TODO: Convert color space
        setPulse(terminal*3 + 0, cie_lookup[_rgbww[terminal].r]);
        setPulse(terminal*3 + 1, cie_lookup[_rgbww[terminal].g]);
        setPulse(terminal*3 + 2, cie_lookup[_rgbww[terminal].b]);
    } break;
    }
    
}

void Driver::setPulse(size_t idx, uint16_t pulse) {
    idx %= 6;
    switch(idx) {
    case 0: {
        PwmTimer1::instance().setPulse(pulse);
    } break;
    case 1: {
        PwmTimer2::instance().setPulse(pulse);
    } break;
    case 2: {
        PwmTimer0::instance().setPulse(pulse);
    } break;
    case 3: {
        PwmTimer3::instance().setPulse(pulse);
    } break;
    case 4: {
        PwmTimer5::instance().setPulse(pulse);
    } break;
    case 5: {
        PwmTimer6::instance().setPulse(pulse);
    } break;
    }
}

void Driver::maybeUpdateCIE() {
    if (Model::instance().globPWMLimit() != pwm_limit) {
        pwm_limit = Model::instance().globPWMLimit();
        for (size_t c = 0; c<256; c++) {
            float f = float(c) * (1.0f / 255.0f);
            float v = (f > 0.08f) ? powf( (f + 0.160f) / 1.160f, 3.0f) : (f / 9.03296296296296296294f);
            cie_lookup[c] = uint16_t(v * float(PwmTimer::pwmPeriod) * pwm_limit);
        }
    }
}

void Driver::init() {
    maybeUpdateCIE();
    
    PwmTimer0::instance().setPulse(0x0);
    PwmTimer1::instance().setPulse(0x0);
    PwmTimer2::instance().setPulse(0x0);
    PwmTimer3::instance().setPulse(0x0);
    PwmTimer5::instance().setPulse(0x0);
    PwmTimer6::instance().setPulse(0x0);
    
    DEBUG_PRINTF(("Driver up.\n"));
}

}
