/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>

#include "gd32f10x.h"
#include "cmsis_gcc.h"

extern "C" {
#include "lwip/apps/httpd.h"
#include "lwip/ip4_addr.h"
#include "./mjson.h"
#include "./ftoa.h"
};

#include "./color.h"
#include "./model.h"
#include "./netconf.h"
#include "./systick.h"
#include "./status.h"

const int32_t build_number = 
#include "./build_number.h"
;

#if LWIP_HTTPD_SUPPORT_REST

#define CRLF "\r\n"

namespace lightkraken {

class HTTPPostParser {
public:
    static HTTPPostParser &instance();
    
    void begin() {
        buf_ptr = post_buf;
        memset(post_buf, 0, sizeof(post_buf));
    }
    
    void pushData(void *data, size_t len) {
        size_t buf_max_len = sizeof(post_buf) - 1;
        size_t buf_cur_len = buf_ptr - post_buf;
        len = std::min(buf_cur_len + len, buf_max_len) - buf_cur_len;
        if (len > 0) {
            memcpy(buf_ptr, data, len);
            buf_ptr += len;
        }
    }
    
    void end() {
        char buf[64];
        char ss[64];
        int ival = 0;
        double dval = 0;
        size_t post_len = strlen(post_buf);

        if (mjson_get_bool(post_buf, post_len, "$.dhcp", &ival) > 0) {
            Model::instance().setDhcpEnabled(ival ? true : false);
        }
        
        if (mjson_get_bool(post_buf, post_len, "$.broadcast", &ival) > 0) {
            Model::instance().setBroadcastEnabled(ival ? true : false);
        }
        
        if (mjson_get_string(post_buf, post_len, "$.ipv4address", buf, sizeof(buf)) > 0) {
            int ipbits[4];
            sscanf(buf, "%d.%d.%d.%d", &ipbits[0], &ipbits[1], &ipbits[2], &ipbits[3]);
            IP4_ADDR(Model::instance().ip4Address(), ipbits[0], ipbits[1], ipbits[2], ipbits[3]);
        }
        
        if (mjson_get_string(post_buf, post_len, "$.ipv4netmask", buf, sizeof(buf)) > 0) {
            int ipbits[4];
            sscanf(buf, "%d.%d.%d.%d", &ipbits[0], &ipbits[1], &ipbits[2], &ipbits[3]);
            IP4_ADDR(Model::instance().ip4Netmask(), ipbits[0], ipbits[1], ipbits[2], ipbits[3]);
        }
        
        if (mjson_get_string(post_buf, post_len, "$.ipv4gateway", buf, sizeof(buf)) > 0) {
            int ipbits[4];
            sscanf(buf, "%d.%d.%d.%d", &ipbits[0], &ipbits[1], &ipbits[2], &ipbits[3]);
            IP4_ADDR(Model::instance().ip4Gateway(), ipbits[0], ipbits[1], ipbits[2], ipbits[3]);
        }
        
        if (mjson_get_number(post_buf, post_len, "$.outputmode", &dval) > 0) {
            Model::instance().setOutputConfig(Model::OutputConfig(int(dval)));
        } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
            Model::instance().setOutputConfig(Model::OutputConfig(int(atof(buf))));
        }
        
        if (mjson_get_number(post_buf, post_len, "$.globpwmlimit", &dval) > 0) {
            Model::instance().setGlobPWMLimit(float(dval));
        } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
            Model::instance().setGlobPWMLimit(float(atof(buf)));
        }
        
        if (mjson_get_number(post_buf, post_len, "$.globcomplimit", &dval) > 0) {
            Model::instance().setGlobCompLimit(float(dval));
        } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
            Model::instance().setGlobCompLimit(float(atof(buf)));
        }

        if (mjson_get_number(post_buf, post_len, "$.globillum", &dval) > 0) {
            Model::instance().setGlobIllum(float(dval));
        } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
            Model::instance().setGlobIllum(float(atof(buf)));
        }
        
        for (int c=0; c<int(Model::analogN); c++) {
            Model::AnalogConfig &config = Model::instance().analogConfig(c);

            sprintf(ss, "$.rgbconfig[%d].type", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.type = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                config.type = int(atof(buf));
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.xw", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.xw = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.xw = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.yw", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.rgbSpace.yw = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.yw = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.xr", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.xr = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.xr = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.yr", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.yr = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.yr = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.xg", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.xg = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.xg = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.yg", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.yg = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.yg = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.xb", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.xb = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.xb = float(dval);
            }

            sprintf(ss, "$.rgbconfig[%d].rgbspace.yb", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                 config.rgbSpace.yb = float(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                 config.rgbSpace.yb = float(dval);
            }

            for (int d=0; d<int(Model::analogCompN); d++) {

                sprintf(ss, "$.rgbconfig[%d].components[%d].universe", c, d);
                if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                    config.components[d].universe = int(dval);
                } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                    config.components[d].universe = int(atof(buf));
                }
                
                sprintf(ss, "$.rgbconfig[%d].components[%d].offset", c, d);
                if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                    config.components[d].offset = int(dval);
                } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                    config.components[d].offset = int(atof(buf));
                }
                
                sprintf(ss, "$.rgbconfig[%d].components[%d].value", c, d);
                if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                    config.components[d].value = int(dval);
                } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                    config.components[d].value = int(atof(buf));
                }
            }
        }

        for (int c=0; c<int(Model::stripN); c++) {
            Model::StripConfig &config = Model::instance().stripConfig(c);

            sprintf(ss, "$.stripconfig[%d].type", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.type = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                config.type = int(atof(buf));
            }
            
            sprintf(ss, "$.stripconfig[%d].length", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.len = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                config.len = int(atof(buf));
            }
            
            sprintf(ss, "$.stripconfig[%d].color.r", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.color.r = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf)) > 0) {
                config.color.r = strtol(buf, NULL, 10);
            }
            
            sprintf(ss, "$.stripconfig[%d].color.g", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.color.g = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf)) > 0) {
                config.color.g = strtol(buf, NULL, 10);
            }

            sprintf(ss, "$.stripconfig[%d].color.b", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.color.b = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf)) > 0) {
                config.color.b = strtol(buf, NULL, 10);
            }

            sprintf(ss, "$.stripconfig[%d].color.a", c);
            if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                config.color.x = int(dval);
            } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf)) > 0) {
                config.color.x = strtol(buf, NULL, 10);
            }
            
            for (int d=0; d<int(Model::universeN); d++) {
                sprintf(ss, "$.stripconfig[%d].universes[%d].universe", c, d);
                if (mjson_get_number(post_buf, post_len, ss, &dval) > 0) {
                    config.universe[d] = int(dval);
                } else if (mjson_get_string(post_buf, post_len, ss, buf, sizeof(buf))) {
                    config.universe[d] = int(atof(buf));
                }
            }
        }
    
    	Model::instance().save();
        Systick::instance().scheduleApply();
    }
    
private:
    bool initialized = false;
    void init();
    char *buf_ptr;
    char post_buf[1536];
};

HTTPPostParser &HTTPPostParser::instance() {
    static HTTPPostParser HTTPPostParser;
    if (!HTTPPostParser.initialized) {
        HTTPPostParser.initialized = true;
        HTTPPostParser.init();
    }
    return HTTPPostParser;
}

void HTTPPostParser::init() {
}

class HTTPResponseBuilder {
public:

    static HTTPResponseBuilder &instance();
    
    void beginOKResponse() {
        buf_ptr = response_buf;
        addString("HTTP/1.0 200 OK" CRLF
                  "Access-Control-Allow-Origin: *" CRLF);
        responseType = OKResponse;
    }

    void beginJSONResponse() {
        buf_ptr = response_buf;
        addString("HTTP/1.0 200 OK" CRLF 
				  "Access-Control-Allow-Origin: *" CRLF);

        addString("Content-Type: application/json; charset=utf-8" CRLF 
				  "X-Content-Type-Options: nosniff" CRLF
				  "Vary: Origin, Accept-Encoding" CRLF
				  "Content-Length: @@@@@@@@@@@" CRLF
				  "Cache-Control: no-cache" CRLF
				  CRLF);
        
        content_start = buf_ptr;
        addString("{");
        first_item = true;
        responseType = JSONResponse;
    }
    
    const char *finish(u16_t &length) {
        switch (responseType) {
        case JSONResponse: {
            addString("}");
            
            // Patch content length
            char *contentLengthPtr = strstr(response_buf, "@@@@@@@@@@@");
            if (!contentLengthPtr) {
                return 0;
            }

            char numberStr[12];
            memset(numberStr, 0, sizeof(numberStr));
            sprintf(numberStr, "%d", int(buf_ptr - content_start));
            for (size_t c = sizeof(numberStr)-1; c>0; c--) {
                if (numberStr[c] == 0) numberStr[c] = ' ';
            }
            strncpy(contentLengthPtr, numberStr, 11);

            length = u16_t(buf_ptr - response_buf);
            return response_buf;
        } break;
        case OKResponse: {
            length = u16_t(buf_ptr - response_buf);
            return response_buf;
        } break;
        }
        return 0;
    }

    void addNetConfIPv4Address() {
        handleDelimiter();
        addString("\"ipv4address\":\"%d.%d.%d.%d\"", 
                ip4_addr1(&NetConf::instance().netInterface()->ip_addr),
                ip4_addr2(&NetConf::instance().netInterface()->ip_addr),
                ip4_addr3(&NetConf::instance().netInterface()->ip_addr),
                ip4_addr4(&NetConf::instance().netInterface()->ip_addr));
    }

    void addNetConfIPv4Netmask() {
        handleDelimiter();
        addString("\"ipv4netmask\":\"%d.%d.%d.%d\"", 
                ip4_addr1(&NetConf::instance().netInterface()->netmask),
                ip4_addr2(&NetConf::instance().netInterface()->netmask),
                ip4_addr3(&NetConf::instance().netInterface()->netmask),
                ip4_addr4(&NetConf::instance().netInterface()->netmask));
    }

    void addNetConfIPv4Gateway() {
        handleDelimiter();
        addString("\"ipv4gateway\":\"%d.%d.%d.%d\"", 
                ip4_addr1(&NetConf::instance().netInterface()->gw),
                ip4_addr2(&NetConf::instance().netInterface()->gw),
                ip4_addr3(&NetConf::instance().netInterface()->gw),
                ip4_addr4(&NetConf::instance().netInterface()->gw));
    }

    void addSystemTime() {
        handleDelimiter();
        addString("\"systemtime\":%d", int(Systick::instance().systemTime())); 
    }

    void addBuildNumber() {
        handleDelimiter();
        addString("\"buildnumber\": \"Rev %d (%s %s)\"  ", int(build_number), __DATE__, __TIME__); 
    }

    void addHostname() {
        handleDelimiter();
        addString("\"hostname\":\"%s\"", NetConf::instance().netInterface()->hostname); 
    }

    void addMacAddress() {
        handleDelimiter();
        addString("\"macaddress\":\"%02x:%02x:%02x:%02x:%02x:%02x\"", 
                        NetConf::instance().netInterface()->hwaddr[0],
                        NetConf::instance().netInterface()->hwaddr[1],
                        NetConf::instance().netInterface()->hwaddr[2],
                        NetConf::instance().netInterface()->hwaddr[3],
                        NetConf::instance().netInterface()->hwaddr[4],
                        NetConf::instance().netInterface()->hwaddr[5]); 
    }
    
    void addDHCP() {
        handleDelimiter();
        addString("\"dhcp\":%s",Model::instance().dhcpEnabled()?"true":"false"); 
    }

    void addBroadcast() {
        handleDelimiter();
        addString("\"broadcast\":%s",Model::instance().broadcastEnabled()?"true":"false"); 
    }
    
    void addIPv4Address() {
        handleDelimiter();
        addString("\"ipv4address\":\"%d.%d.%d.%d\"", 
            ip4_addr1(Model::instance().ip4Address()),
            ip4_addr2(Model::instance().ip4Address()),
            ip4_addr3(Model::instance().ip4Address()),
            ip4_addr4(Model::instance().ip4Address()));
    }
    
    
    void addIPv4Netmask() {
        handleDelimiter();
        addString("\"ipv4netmask\":\"%d.%d.%d.%d\"", 
            ip4_addr1(Model::instance().ip4Netmask()),
            ip4_addr2(Model::instance().ip4Netmask()),
            ip4_addr3(Model::instance().ip4Netmask()),
            ip4_addr4(Model::instance().ip4Netmask()));
    }
    
    void addIPv4Gateway() {
        handleDelimiter();
        addString("\"ipv4gateway\":\"%d.%d.%d.%d\"", 
            ip4_addr1(Model::instance().ip4Gateway()),
            ip4_addr2(Model::instance().ip4Gateway()),
            ip4_addr3(Model::instance().ip4Gateway()),
            ip4_addr4(Model::instance().ip4Gateway()));
    }
    
    void addOutputMode() {
        handleDelimiter();
        addString("\"outputmode\":%d",Model::instance().outputConfig()); 
    }

    void addPwmLimit() {
        handleDelimiter();
        char str[32];
        ftoa(str, Model::instance().globPWMLimit(), NULL);
        addString("\"globpwmlimit\":%s",str); 
    }

    void addCompLimit() {
        handleDelimiter();
        char str[32];
        ftoa(str, Model::instance().globCompLimit(), NULL);
        addString("\"globcomplimit\":%s",str); 
    }

    void addIllum() {
        handleDelimiter();
        char str[32];
        ftoa(str, Model::instance().globIllum(), NULL);
        addString("\"globillum\":%s",str); 
    }

    void addAnalogConfig() {
        handleDelimiter();
		char str[32];
        addString("\"rgbconfig\":["); 
        for (size_t c=0; c<Model::analogN; c++) {
            const Model::AnalogConfig &a = Model::instance().analogConfig(c);
            addString("{");
            addString("\"type\":%d,",int(a.type)); 
            addString("\"rgbspace\" : {");
			ftoa(str, a.rgbSpace.xw, NULL); addString("\"xw\":%s,",str); 
			ftoa(str, a.rgbSpace.yw, NULL); addString("\"yw\":%s,",str); 
			ftoa(str, a.rgbSpace.xr, NULL); addString("\"xr\":%s,",str); 
			ftoa(str, a.rgbSpace.yr, NULL); addString("\"yr\":%s,",str); 
			ftoa(str, a.rgbSpace.xg, NULL); addString("\"xg\":%s,",str); 
			ftoa(str, a.rgbSpace.yg, NULL); addString("\"yg\":%s,",str); 
			ftoa(str, a.rgbSpace.xb, NULL); addString("\"xb\":%s,",str); 
			ftoa(str, a.rgbSpace.yb, NULL); addString("\"yb\":%s",str); 
            addString("},");
            addString("\"components\" : [");
            for (size_t d=0; d<Model::analogCompN; d++) {
                addString("{");
                addString("\"universe\":%d,",int(a.components[d].universe)); 
                addString("\"offset\":%d,",int(a.components[d].offset)); 
                addString("\"value\":%d",int(a.components[d].value)); 
                addString("}%c", (d==Model::analogCompN-1)?' ':','); 
            }
            addString("]");
            addString("}%c", (c==Model::analogN-1)?' ':',');
        }
        addString("]");
    }

    void addStripConfig() {
        handleDelimiter();
        addString("\"stripconfig\":["); 
        for (size_t c=0; c<Model::stripN; c++) {
            const Model::StripConfig &s = Model::instance().stripConfig(c);
            addString("{");
            addString("\"type\":%d,",int(s.type)); 
            addString("\"length\":%d,",int(s.len)); 
            addString("\"color\":{\"r\":%d,\"g\":%d,\"b\":%d,\"a\":%d},",
                            (int)s.color.r,
                            (int)s.color.g,
                            (int)s.color.b,
                            (int)s.color.x); 
            addString("\"universes\" : [");
            for (size_t d=0; d<Model::universeN; d++) {
                addString("{");
                addString("\"universe\":%d",int(s.universe[d])); 
                addString("}%c", (d==Model::universeN-1)?' ':','); 
            }
            addString("]");
            addString("}%c", (c==Model::stripN-1)?' ':',');
        }
        addString("]");
    }
    
private:
    void handleDelimiter() {
        if (first_item) {
            first_item = false;
        } else {
            addString(",");
        }
    }

	template<typename... Args> void addString(const char *fmt, Args... args) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
        buf_ptr += sprintf(buf_ptr, fmt, args...); 
#pragma GCC diagnostic pop
    }

    enum ResponseType {
        OKResponse = 0,
        JSONResponse = 1,
    } responseType = OKResponse;
    
    bool initialized = false;
    void init();
    
    bool first_item = true;
    char *buf_ptr;
    char *content_start;
    char response_buf[2048];
};

HTTPResponseBuilder &HTTPResponseBuilder::instance() {
    static HTTPResponseBuilder HTTPResponseBuilder;
    if (!HTTPResponseBuilder.initialized) {
        HTTPResponseBuilder.initialized = true;
        HTTPResponseBuilder.init();
    }
    return HTTPResponseBuilder;
}

void HTTPResponseBuilder::init() {
}

class ConnectionManager {
public:
	enum RestMethod {
		MethodNone,
		MethodGetStatus,
		MethodGetSettings,
		MethodPostSettings,
		MethodPostBootLoader,
	};

	constexpr static size_t maxConnections = MEMP_NUM_TCP_PCB;
	constexpr static uint32_t connectionTimeout = 10000;

    static ConnectionManager &instance();

	void init() {
		memset(connections, 0, sizeof(connections));
	}

	struct ConnectionInfo {
		public:
			void *handle;
			RestMethod method;
			struct pbuf *buffers[16];
			size_t buffer_index;
			uint32_t time_stamp;
	};
	
	ConnectionInfo *begin(void *handle) {
		uint32_t now = Systick::instance().systemTime();
		for (size_t c = 0; c < maxConnections; c++) {
			if ((connections[c].handle == NULL) || 
			    (connections[c].time_stamp && ((connections[c].time_stamp - now) > connectionTimeout)) ) {
				memset(&connections[c], 0, sizeof(ConnectionInfo));
				connections[c].handle = handle;
				connections[c].time_stamp = now;
				return &connections[c];
			}
		}
		return NULL;
	}
	
	ConnectionInfo *get(void *handle) {
		for (size_t c = 0; c < maxConnections; c++) {
			if (connections[c].handle == handle) {
				return &connections[c];
			}
		}
		return NULL;
	}
	
	void end(void *handle) {
		for (size_t c = 0; c < maxConnections; c++) {
			if (connections[c].handle == handle) {
				connections[c].handle = NULL;
				break;
			}
		}
	}

private:
    bool initialized = false;
	ConnectionInfo connections[maxConnections];
};

ConnectionManager &ConnectionManager::instance() {
    static ConnectionManager connectionManager;
    if (!connectionManager.initialized) {
        connectionManager.initialized = true;
        connectionManager.init();
    }
    return connectionManager;
}

}

using namespace lightkraken;

err_t httpd_rest_begin(void *handle, rest_method_t method, const char *url, const char *, u16_t, int, u8_t *) {
	ConnectionManager::ConnectionInfo *info = ConnectionManager::instance().begin(handle);
	if (!info) {
		return ERR_ARG;
	}
    switch(method) {
        case REST_METHOD_OPTIONS: {
        } break;
        case REST_METHOD_GET: {
            if (strcmp(url, "/status") == 0) {
                info->method = ConnectionManager::MethodGetStatus;
                return ERR_OK;
            } else if (strcmp(url, "/settings") == 0) {
                info->method = ConnectionManager::MethodGetSettings;
                return ERR_OK;
            }
        } break;
        case REST_METHOD_POST: {
            if (strcmp(url, "/settings") == 0) {
                info->method = ConnectionManager::MethodPostSettings;
                return ERR_OK;
            } else if (strcmp(url, "/bootloader") == 0) {
                info->method = ConnectionManager::MethodPostBootLoader;
                return ERR_OK;
            }
        } break;
        case REST_METHOD_PUT: {
        } break;
        case REST_METHOD_PATCH: {
        } break;
        case REST_METHOD_DELETE: {
        } break;
        case REST_METHOD_NONE: {
        } break;
    }
	ConnectionManager::instance().end(handle);
    return ERR_ARG;
}

err_t httpd_rest_receive_data(void *handle, struct pbuf *p) {
	ConnectionManager::ConnectionInfo *info = ConnectionManager::instance().get(handle);
	if (!info) {
		return ERR_ARG;
	}
    switch(info->method) {
		case ConnectionManager::MethodPostSettings: {
		   info->buffers[info->buffer_index++] = p;
		} break;
		default:
        case ConnectionManager::MethodNone:
        case ConnectionManager::MethodPostBootLoader:
        case ConnectionManager::MethodGetSettings:
        case ConnectionManager::MethodGetStatus: {
           // drop buffers to the floor
		   pbuf_free(p);
        } break;
    }
    return ERR_OK;
}

err_t httpd_rest_finished(void *handle, const char **data, u16_t *dataLen) {
	ConnectionManager::ConnectionInfo *info = ConnectionManager::instance().get(handle);
	if (!info) {
		return ERR_ARG;
	}
    switch(info->method) {
        case ConnectionManager::MethodGetStatus: {
            
            HTTPResponseBuilder &response = HTTPResponseBuilder::instance();
            response.beginJSONResponse();
            response.addNetConfIPv4Address();
            response.addNetConfIPv4Netmask();
            response.addNetConfIPv4Gateway();
            response.addSystemTime();
            response.addBuildNumber();
            response.addHostname();
            response.addMacAddress();
            *data = response.finish(*dataLen);
            
            ConnectionManager::instance().end(handle);
            return ERR_OK;
        } break;
        case ConnectionManager::MethodGetSettings: {
            
            HTTPResponseBuilder &response = HTTPResponseBuilder::instance();
            response.beginJSONResponse();
            response.addDHCP();
            response.addBroadcast();
            response.addIPv4Address();
            response.addIPv4Netmask();
            response.addIPv4Gateway();
            response.addOutputMode();
            response.addPwmLimit();
            response.addCompLimit();
            response.addIllum();
            response.addAnalogConfig();
            response.addStripConfig();
            *data = response.finish(*dataLen);
            
            ConnectionManager::instance().end(handle);
            return ERR_OK;
        } break;
        case ConnectionManager::MethodPostBootLoader: {
            Systick::instance().scheduleReset(4000, true);

            HTTPResponseBuilder &response = HTTPResponseBuilder::instance();
            response.beginOKResponse();
            *data = response.finish(*dataLen);

            ConnectionManager::instance().end(handle);

            return ERR_OK;
        } break;
        case ConnectionManager::MethodPostSettings: {
            
            HTTPPostParser::instance().begin();
            for (size_t c = 0; c < info->buffer_index; c++) {
	        	HTTPPostParser::instance().pushData(info->buffers[c]->payload, info->buffers[c]->len);
	        	pbuf_free(info->buffers[c]);
	        	info->buffers[c] = 0;
	        }
   	     	HTTPPostParser::instance().end();
            
            HTTPResponseBuilder &response = HTTPResponseBuilder::instance();
            response.beginOKResponse();
            *data = response.finish(*dataLen);

            ConnectionManager::instance().end(handle);
            return ERR_OK;
        } break;
        case ConnectionManager::MethodNone: {

            ConnectionManager::instance().end(handle);
            return ERR_OK;
        } break;
    }
    
	ConnectionManager::instance().end(handle);
    return ERR_ARG;
}

#endif  // #if LWIP_HTTPD_SUPPORT_REST
