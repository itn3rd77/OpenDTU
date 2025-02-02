#include "WebApi_mqtt.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "helper.h"

void WebApiMqttClass::init(AsyncWebServer* server)
{
    using namespace std::placeholders;

    _server = server;

    _server->on("/api/mqtt/status", HTTP_GET, std::bind(&WebApiMqttClass::onMqttStatus, this, _1));
    _server->on("/api/mqtt/config", HTTP_GET, std::bind(&WebApiMqttClass::onMqttAdminGet, this, _1));
    _server->on("/api/mqtt/config", HTTP_POST, std::bind(&WebApiMqttClass::onMqttAdminPost, this, _1));
}

void WebApiMqttClass::loop()
{
}

void WebApiMqttClass::onMqttStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("mqtt_enabled")] = config.Mqtt_Enabled;
    root[F("mqtt_hostname")] = config.Mqtt_Hostname;
    root[F("mqtt_port")] = config.Mqtt_Port;
    root[F("mqtt_username")] = config.Mqtt_Username;
    root[F("mqtt_topic")] = config.Mqtt_Topic;
    root[F("mqtt_connected")] = MqttSettings.getConnected();
    root[F("mqtt_retain")] = config.Mqtt_Retain;
    root[F("mqtt_lwt_topic")] = String(config.Mqtt_Topic) + config.Mqtt_LwtTopic;
    root[F("mqtt_publish_interval")] = config.Mqtt_PublishInterval;

    response->setLength();
    request->send(response);
}

void WebApiMqttClass::onMqttAdminGet(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("mqtt_enabled")] = config.Mqtt_Enabled;
    root[F("mqtt_hostname")] = config.Mqtt_Hostname;
    root[F("mqtt_port")] = config.Mqtt_Port;
    root[F("mqtt_username")] = config.Mqtt_Username;
    root[F("mqtt_password")] = config.Mqtt_Password;
    root[F("mqtt_topic")] = config.Mqtt_Topic;
    root[F("mqtt_retain")] = config.Mqtt_Retain;
    root[F("mqtt_lwt_topic")] = config.Mqtt_LwtTopic;
    root[F("mqtt_lwt_online")] = config.Mqtt_LwtValue_Online;
    root[F("mqtt_lwt_offline")] = config.Mqtt_LwtValue_Offline;
    root[F("mqtt_publish_interval")] = config.Mqtt_PublishInterval;

    response->setLength();
    request->send(response);
}

void WebApiMqttClass::onMqttAdminPost(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("mqtt_enabled") && root.containsKey("mqtt_hostname") && root.containsKey("mqtt_port") && root.containsKey("mqtt_username") && root.containsKey("mqtt_password") && root.containsKey("mqtt_topic") && root.containsKey("mqtt_retain") && root.containsKey("mqtt_lwt_topic") && root.containsKey("mqtt_lwt_online") && root.containsKey("mqtt_lwt_offline") && root.containsKey("mqtt_publish_interval"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("mqtt_enabled")].as<bool>()) {
        if (root[F("mqtt_hostname")].as<String>().length() == 0 || root[F("mqtt_hostname")].as<String>().length() > MQTT_MAX_HOSTNAME_STRLEN) {
            retMsg[F("message")] = F("MqTT Server must between 1 and " STR(MQTT_MAX_HOSTNAME_STRLEN) " characters long!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_username")].as<String>().length() > MQTT_MAX_USERNAME_STRLEN) {
            retMsg[F("message")] = F("Username must not longer then " STR(MQTT_MAX_USERNAME_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }
        if (root[F("mqtt_password")].as<String>().length() > MQTT_MAX_PASSWORD_STRLEN) {
            retMsg[F("message")] = F("Password must not longer then " STR(MQTT_MAX_PASSWORD_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }
        if (root[F("mqtt_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg[F("message")] = F("Topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_port")].as<uint>() == 0 || root[F("mqtt_port")].as<uint>() > 65535) {
            retMsg[F("message")] = F("Port must be a number between 1 and 65535!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg[F("message")] = F("LWT topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_online")].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg[F("message")] = F("LWT online value must not longer then " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_offline")].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg[F("message")] = F("LWT offline value must not longer then " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_publish_interval")].as<uint32_t>() < 5 || root[F("mqtt_publish_interval")].as<uint32_t>() > 65535) {
            retMsg[F("message")] = F("Publish interval must be a number between 5 and 65535!");
            response->setLength();
            request->send(response);
            return;
        }
    }

    CONFIG_T& config = Configuration.get();
    config.Mqtt_Enabled = root[F("mqtt_enabled")].as<bool>();
    config.Mqtt_Retain = root[F("mqtt_retain")].as<bool>();
    config.Mqtt_Port = root[F("mqtt_port")].as<uint>();
    strcpy(config.Mqtt_Hostname, root[F("mqtt_hostname")].as<String>().c_str());
    strcpy(config.Mqtt_Username, root[F("mqtt_username")].as<String>().c_str());
    strcpy(config.Mqtt_Password, root[F("mqtt_password")].as<String>().c_str());
    strcpy(config.Mqtt_Topic, root[F("mqtt_topic")].as<String>().c_str());
    strcpy(config.Mqtt_LwtTopic, root[F("mqtt_lwt_topic")].as<String>().c_str());
    strcpy(config.Mqtt_LwtValue_Online, root[F("mqtt_lwt_online")].as<String>().c_str());
    strcpy(config.Mqtt_LwtValue_Offline, root[F("mqtt_lwt_offline")].as<String>().c_str());
    config.Mqtt_PublishInterval = root[F("mqtt_publish_interval")].as<uint32_t>();
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    MqttSettings.performReconnect();
}