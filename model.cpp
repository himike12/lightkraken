/*
* endpoint.cpp
*
*  Created on: Sep 5, 2019
*      Author: turo
*/

extern "C" {
#include "gd32f10x.h"
}

#include "./model.h"
#include "./strip.h"
#include "./driver.h"
#include "./spi.h"

namespace lightkraken {

/*
static uint32_t readu32(const uint8_t **page) {
    uint32_t v = ((*page[0])    )|
                ((*page[1])<< 8)|
                ((*page[2])<<16)|
                ((*page[3])<<24);
    *page+=4;
    return v;
}

static uint16_t readu16(const uint8_t **page) {
    uint16_t v = ((*page[0])    )|
                ((*page[1])<< 8);
    *page+=2;
    return v;
}
*/

void Model::readFlash() {
}

/*
static void writeu32(uintptr_t *addr, uint32_t v) {
    (void)v;
    *addr += 4;
}

static void writeu16(uintptr_t *addr, uint16_t v) {
    (void)v;
    *addr += 2;
}
*/

void Model::writeFlash() {
}

void Model::init() {

    const uint8_t IP_ADDRESS0 =  169;
    const uint8_t IP_ADDRESS1 =  254;
    const uint8_t IP_ADDRESS2 = 0x1e;
    const uint8_t IP_ADDRESS3 = 0xd5;

    const uint8_t IP_NETMASK0 =  255;
    const uint8_t IP_NETMASK1 =  255;
    const uint8_t IP_NETMASK2 =	   0;
    const uint8_t IP_NETMASK3 =	   0;

    const uint8_t IP_GATEWAY0 =  169;
    const uint8_t IP_GATEWAY1 =  254;
    const uint8_t IP_GATEWAY2 =	   0;
    const uint8_t IP_GATEWAY3 =	   1;

    IP4_ADDR(&ip4_address, IP_ADDRESS0, IP_ADDRESS1, IP_ADDRESS2, IP_ADDRESS3);
    IP4_ADDR(&ip4_netmask, IP_NETMASK0, IP_NETMASK1, IP_NETMASK2, IP_NETMASK3);
    IP4_ADDR(&ip4_gateway, IP_GATEWAY0, IP_GATEWAY1, IP_GATEWAY2, IP_GATEWAY3);

    ip_dhcp = true;
    
    receive_broadcast = false;

    glob_pwmlimit = 1.0f;
    glob_illum = 0x1F;
    glob_comp_lim = 0xFF;

    output_config = OUTPUT_CONFIG_RGBW_STRIP;

    burst_mode = false;

    int32_t counter = 0;
    for (size_t c = 0; c < stripN; c++) {
        strip_config[c].type = Strip::GS8208_RGB;
        lightkraken::Strip::get(c).setStripType(Strip::Type(strip_config[c].type));
        strip_config[c].len = 256;
        strip_config[c].color = rgb8();
        lightkraken::Strip::get(c).setPixelLen(strip_config[c].len);
        for (size_t d = 0; d < universeN; d++) {
            strip_config[c].universe[d] = counter++;
        }
    }

    counter = 0;
    for (size_t c = 0; c < analogN; c++) {
        analog_config[c].type = 0;
        for (size_t d = 0; d < analogCompN; d++) {
            analog_config[c].components[d].value = 0;
            analog_config[c].components[d].universe = 0;
            analog_config[c].components[d].offset = counter++;
        }
    }

    readFlash();
}

void Model::setOutputConfig(OutputConfig outputConfig) {
    output_config = outputConfig;
}

Model &Model::instance() {
    static Model model;
    if (!model.initialized) {
        model.initialized = true;
        model.init();
    }
    return model;
}

} /* namespace lightkraken */

