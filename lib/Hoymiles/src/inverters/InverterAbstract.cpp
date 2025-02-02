#include "InverterAbstract.h"
#include "crc.h"
#include <cstring>

InverterAbstract::InverterAbstract(uint64_t serial)
{
    _serial.u64 = serial;
    _alarmLogParser.reset(new AlarmLogParser());
    _statisticsParser.reset(new StatisticsParser());
}

uint64_t InverterAbstract::serial()
{
    return _serial.u64;
}

void InverterAbstract::setName(const char* name)
{
    uint8_t len = strlen(name);
    if (len + 1 > MAX_NAME_LENGTH) {
        len = MAX_NAME_LENGTH - 1;
    }
    strncpy(_name, name, len);
    _name[len] = '\0';
}

const char* InverterAbstract::name()
{
    return _name;
}

AlarmLogParser* InverterAbstract::EventLog()
{
    return _alarmLogParser.get();
}

StatisticsParser* InverterAbstract::Statistics()
{
    return _statisticsParser.get();
}

void InverterAbstract::clearRxFragmentBuffer()
{
    memset(_rxFragmentBuffer, 0, MAX_RF_FRAGMENT_COUNT * MAX_RF_PAYLOAD_SIZE);
    _rxFragmentMaxPacketId = 0;
    _rxFragmentLastPacketId = 0;
    _rxFragmentRetransmitCnt = 0;

    // This has to be done here because:
    // Not possible in constructor --> virtual function
    // Not possible in verifyAllFragments --> Because no data if nothing is ever received
    // It has to be executed because otherwise the getChannelCount method in stats always returns 0
    _statisticsParser.get()->setByteAssignment(getByteAssignment(), getAssignmentCount());
}

void InverterAbstract::addRxFragment(uint8_t fragment[], uint8_t len)
{
    uint8_t fragmentCount = fragment[9];
    if ((fragmentCount & 0b01111111) < MAX_RF_FRAGMENT_COUNT) {
        // Packets with 0x81 will be seen as 1
        memcpy(_rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].fragment, &fragment[10], len - 11);
        _rxFragmentBuffer[(fragmentCount & 0b01111111) - 1].len = len - 11;

        if ((fragmentCount & 0b01111111) > _rxFragmentLastPacketId) {
            _rxFragmentLastPacketId = fragmentCount & 0b01111111;
        }
    }

    // 0b10000000 == 0x80
    if ((fragmentCount & 0b10000000) == 0b10000000) {
        _rxFragmentMaxPacketId = fragmentCount & 0b01111111;
    }
}

// Returns Zero on Success or the Fragment ID for retransmit or error code
uint8_t InverterAbstract::verifyAllFragments()
{
    // All missing
    if (_rxFragmentLastPacketId == 0) {
        Serial.println(F("All missing"));
        return FRAGMENT_ALL_MISSING;
    }

    // Last fragment is missing (thte one with 0x80)
    if (_rxFragmentMaxPacketId == 0) {
        Serial.println(F("Last missing"));
        if (_rxFragmentRetransmitCnt++ < MAX_RETRANSMIT_COUNT) {
            return _rxFragmentLastPacketId + 1;
        } else {
            return FRAGMENT_RETRANSMIT_TIMEOUT;
        }
    }

    // Middle fragment is missing
    for (uint8_t i = 0; i < _rxFragmentMaxPacketId - 1; i++) {
        if (_rxFragmentBuffer[i].len == 0) {
            Serial.println(F("Middle missing"));
            if (_rxFragmentRetransmitCnt++ < MAX_RETRANSMIT_COUNT) {
                return i + 1;
            } else {
                return FRAGMENT_RETRANSMIT_TIMEOUT;
            }
        }
    }

    // All fragments are available --> Check CRC
    uint16_t crc = 0xffff, crcRcv;

    for (uint8_t i = 0; i < _rxFragmentMaxPacketId; i++) {
        if (i == _rxFragmentMaxPacketId - 1) {
            // Last packet
            crc = crc16(_rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len - 2, crc);
            crcRcv = (_rxFragmentBuffer[i].fragment[_rxFragmentBuffer[i].len - 2] << 8)
                | (_rxFragmentBuffer[i].fragment[_rxFragmentBuffer[i].len - 1]);
        } else {
            crc = crc16(_rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len, crc);
        }
    }

    if (crc != crcRcv) {
        return FRAGMENT_CRC_ERROR;
    }

    if (getLastRequest() == RequestType::Stats) {
        // Move all fragments into target buffer
        uint8_t offs = 0;
        _statisticsParser.get()->clearBuffer();
        for (uint8_t i = 0; i < _rxFragmentMaxPacketId; i++) {
            _statisticsParser.get()->appendFragment(offs, _rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len);
            offs += (_rxFragmentBuffer[i].len);
        }
        _lastStatsUpdate = millis();

    } else if (getLastRequest() == RequestType::AlarmLog) {
        // Move all fragments into target buffer
        uint8_t offs = 0;
        _alarmLogParser.get()->clearBuffer();
        for (uint8_t i = 0; i < _rxFragmentMaxPacketId; i++) {
            _alarmLogParser.get()->appendFragment(offs, _rxFragmentBuffer[i].fragment, _rxFragmentBuffer[i].len);
            offs += (_rxFragmentBuffer[i].len);
        }
        _lastAlarmLogUpdate = millis();

    } else {
        Serial.println("Unkown response received");
    }

    setLastRequest(RequestType::None);
    return FRAGMENT_OK;
}

uint32_t InverterAbstract::getLastStatsUpdate()
{
    return _lastStatsUpdate;
}

void InverterAbstract::setLastRequest(RequestType request)
{
    _lastRequest = request;
}

RequestType InverterAbstract::getLastRequest()
{
    return _lastRequest;
}