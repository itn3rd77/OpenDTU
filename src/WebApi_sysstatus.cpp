#include "WebApi_sysstatus.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include <LittleFS.h>
#include <ResetReason.h>

#ifndef AUTO_GIT_HASH
#define AUTO_GIT_HASH ""
#endif

void WebApiSysstatusClass::init(AsyncWebServer* server)
{
    using namespace std::placeholders;

    _server = server;

    _server->on("/api/system/status", HTTP_GET, std::bind(&WebApiSysstatusClass::onSystemStatus, this, _1));
}

void WebApiSysstatusClass::loop()
{
}

void WebApiSysstatusClass::onSystemStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root[F("hostname")] = WiFi.getHostname();

    root[F("sdkversion")] = ESP.getSdkVersion();
    root[F("cpufreq")] = ESP.getCpuFreqMHz();

    root[F("heap_total")] = ESP.getHeapSize();
    root[F("heap_used")] = ESP.getHeapSize() - ESP.getFreeHeap();
    root[F("sketch_total")] = ESP.getFreeSketchSpace();
    root[F("sketch_used")] = ESP.getSketchSize();
    root[F("littlefs_total")] = LittleFS.totalBytes();
    root[F("littlefs_used")] = LittleFS.usedBytes();

    root[F("chiprevision")] = ESP.getChipRevision();
    root[F("chipmodel")] = ESP.getChipModel();
    root[F("chipcores")] = ESP.getChipCores();

    String reason;
    reason = ResetReason.get_reset_reason_verbose(0);
    root[F("resetreason_0")] = reason;

    reason = ResetReason.get_reset_reason_verbose(1);
    root[F("resetreason_1")] = reason;

    root[F("cfgsavecount")] = Configuration.get().Cfg_SaveCount;

    char version[16];
    sprintf(version, "%d.%d.%d", CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);
    root[F("firmware_version")] = version;
    root[F("git_hash")] = AUTO_GIT_HASH;

    root[F("uptime")] = esp_timer_get_time() / 1000000;

    response->setLength();
    request->send(response);
}