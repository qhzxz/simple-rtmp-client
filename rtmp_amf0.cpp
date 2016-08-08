//
// Created by 秦汉 on 16/7/16.
//

//#include <iostream>
#include <iostream>
#include <spdlog/spdlog.h>
#include "rtmp_amf0.h"
#include "rtmp_utility.h"
#include "rtmp_log.h"


AmfData::~AmfData() {
}

int AmfArray::getSize() {
    if (size == -1) {
        size = 5; // 1 + 4:1 byte for AmfType ,4 bytes for its length
        if (items != NULL) {
            for (auto begin = items->begin(); begin != items->end(); ++begin) {
                AmfData *data = *begin;
                size += data->getSize();
            }
        }
    }
    return size;
}

AmfArray::~AmfArray() {
    clear();
}

void AmfArray::clear() {
    if (NULL != items) {
        for (auto begin = items->begin(); begin != items->end(); ++begin) {
            AmfData *data = *begin;
            delete data;
        }
        items->clear();
        delete items;
        items = NULL;
    }
}

AmfArray::AmfArray() {
    items = new std::vector<AmfData *>;
}

const std::vector<AmfData *> *AmfArray::getitems() {
    if (NULL != items) {
        return items;
    }
    return NULL;
}

void AmfArray::addItems(AmfData *data) {
    if (NULL != items) {
        items->push_back(data);
    } else {
        delete data;
    }
}

void AmfArray::writeTo(RtmpStream *stream) {}

void AmfArray::readFrom(RtmpStream *stream) {
    // Skip data type byte (we assume it's already read)
    int length = readUnsignedInt32(stream);
    size = 5; // 1 + 4
    if (NULL != items) {
        clear();
    }
    items = new std::vector<AmfData *>(length);
    for (int i = 0; i < length; i++) {
        AmfData *dataItem = AmfDecoder::decode(stream);
        size += dataItem->getSize();
        items->push_back(dataItem);
    }
}


int AmfBoolean::getSize() {
    return 2;
}

bool AmfBoolean::readBooleanFrom(RtmpStream *stream) {
    // Skip data type byte (we assume it's already read)
    return (stream->read_1byte() == 0x01) ? true : false;
}

void AmfBoolean::readFrom(RtmpStream *stream) {
    value = (stream->read_1byte() == 0x01) ? true : false;
}


void AmfBoolean::writeTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_BOOLEAN);
    stream->write_1byte(value ? 0x01 : 0x00);
}

bool AmfBoolean::isValue() {
    return value;
}

void AmfBoolean::setValue(bool value) {
    this->value = value;
}

AmfBoolean::AmfBoolean(bool value) : value(value) {

}

AmfBoolean::AmfBoolean() {

}

AmfString::AmfString() {}

AmfString::AmfString(std::string value) : value(value) {}

std::string AmfString::getValue() {
    return value;
}

void AmfString::setValue(std::string value) {
    this->value = value;
}

void AmfString::writeTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_STRING);
    // Write 2 bytes indicating string length
    writeUnsignedInt16(stream, value.length());
    // Write string
    stream->write_bytes((byte *) value.data(), value.length());
}

void AmfString::readFrom(RtmpStream *stream) {
    // Skip data type byte (we assume it's already read)
    int length = readUnsignedInt16(stream);
    size = 3 + length; // 1 + 2 + length
    // Read string value
    byte byteValue[length];
    readBytesUntilFull(stream, byteValue, length);
    value = std::string((char *) byteValue, length);
}


int AmfString::getSize() {
    if (size == -1) {
        size = 1 + 2 + value.length();
    }
    return size;
}


void AmfNumber::writeTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_NUMBER);
    writeDouble(stream, value);
}

void AmfNumber::setValue(double value) {
    this->value = value;
}

int AmfNumber::getSize() {
    return SIZE;
}

AmfNumber::AmfNumber(double value) : value(value) {}

AmfNumber::AmfNumber() {}

double AmfNumber::getValue() {
    return value;
}

double AmfNumber::readNumberFrom(RtmpStream *in) {
    // Skip data type byte
    in->read_1byte();
    return readDouble(in);
}


void AmfNumber::readFrom(RtmpStream *stream) {
    value = readDouble(stream);
}

const byte AmfObject::OBJECT_END_MARKER[3] = {0x00, 0x00, 0x09};

AmfObject::AmfObject() {
    properties = new PropertyMap;
}

AmfData *AmfObject::getProperty(std::string key) {
    AmfData *pData = NULL;
    PropertyMap::iterator it = properties->find(key);
    if (it != properties->end()) {
        pData = (*it).second;
    }

    return pData;
}

void AmfObject::setDataProperty(std::string key, AmfData *value) {
    PropertyPair pair(key, value);
    properties->insert(pair);
}

void AmfObject::setBoolProperty(std::string key, bool value) {
    PropertyPair pair(key, new AmfBoolean(value));
    properties->insert(pair);
}


void AmfObject::setStringProperty(std::string key, std::string value) {
    PropertyPair pair(key, new AmfString(value));
    properties->insert(pair);
}

void AmfObject::setIntProperty(std::string key, int value) {
    PropertyPair pair(key, new AmfNumber(value));
    properties->insert(pair);
}


void AmfObject::setDoubleProperty(std::string key, double value) {
    PropertyPair pair(key, new AmfNumber(value));
    properties->insert(pair);
}

void AmfObject::writeTo(RtmpStream *out) {
    // Begin the object
    out->write_1byte(AMF0_OBJECT);
    // Write key/value pairs in this object
    for (auto mapIterator = properties->begin(); mapIterator != properties->end(); ++mapIterator) {
        // The key must be a AMF0_STRING type, and thus the "type-definition" byte is implied (not included in message)
        std::string key = (*mapIterator).first;
        AmfData *data = (*mapIterator).second;
        writeUnsignedInt16(out, key.length());
        if (key.length() > 0) {
            out->write_bytes((byte *) key.c_str(), key.length());
        }
        data->writeTo(out);
    }
    // End the object
    out->write_bytes((byte *) OBJECT_END_MARKER, 3);
}

void AmfObject::readFrom(RtmpStream *in) {
    // Skip data type byte (we assume it's already read)
    RtmpByteArrayStream *byte_arr = dynamic_cast<RtmpByteArrayStream *>(in);
    if (byte_arr == NULL) {
        return;
    }
    size = 1;
    while (!byte_arr->empty()) {
        // Look for the 3-byte object end marker [0x00 0x00 0x09]
        byte endMarker[3];
        byte_arr->read_bytes(endMarker, 3);
        if (endMarker[0] == OBJECT_END_MARKER[0] && endMarker[1] == OBJECT_END_MARKER[1] &&
            endMarker[2] == OBJECT_END_MARKER[2]) {
            // End marker found
            size += 3;
            return;
        } else {
            // End marker not found; reset the stream to the marked current_position_pointer and read an AMF property

            byte_arr->skip(-3);
            // Read the property key...
            int length = readUnsignedInt16(byte_arr);
            // Read string value
            byte byteValue[length];
            readBytesUntilFull(byte_arr, byteValue, length);
            std::string key = std::string((char *) byteValue, length);
            int key_length = 2 + length;
            size += key_length;
            // ...and the property value
            AmfData *value = AmfDecoder::decode(byte_arr);

            size += value->getSize();
            PropertyPair pair(key, value);
            properties->insert(pair);
        }
    }
}

int AmfObject::getSize() {
    if (size == -1) {
        size = 1; // object marker
        for (auto mapIterator = properties->begin(); mapIterator != properties->end(); ++mapIterator) {
            // The key must be a AMF0_STRING type, and thus the "type-definition" byte is implied (not included in message)
            std::string key = (*mapIterator).first;
            size += (2 + key.length());// 2 byte for key length
            AmfData *data = (*mapIterator).second;
            size += data->getSize();
        }
        size += 3; // end of object marker
    }
    return size;
}

AmfObject::~AmfObject() {
    for (auto mapIterator = properties->begin(); mapIterator != properties->end(); ++mapIterator) {
        // The key must be a AMF0_STRING type, and thus the "type-definition" byte is implied (not included in message)
        AmfData *data = (*mapIterator).second;
        delete data;
        data = NULL;
    }
    properties->clear();
}

void AmfEcmaArray::writeTo(RtmpStream *out) {
    // Begin the object
    out->write_1byte(AMF0_MAP);
    // Write key/value pairs in this object
    writeUnsignedInt32(out, properties->size());
    for (auto mapIterator = properties->begin(); mapIterator != properties->end(); ++mapIterator) {
        // The key must be a AMF0_STRING type, and thus the "type-definition" byte is implied (not included in message)
        std::string key = (*mapIterator).first;
        AmfData *data = (*mapIterator).second;
        writeUnsignedInt16(out, key.length());
        if (key.length() > 0) {
            out->write_bytes((byte *) key.c_str(), key.length());
        }
        data->writeTo(out);
    }
    // End the object
    out->write_bytes((byte *) OBJECT_END_MARKER, 3);
}

void AmfEcmaArray::readFrom(RtmpStream *in) {
    int32_t length = readUnsignedInt32(in);
    AmfObject::readFrom(in);
    size += 4;
}

int AmfEcmaArray::getSize() {
    if (size == -1) {
        size = AmfObject::getSize();
        size += 4;
    }
    return size;
}

void AmfNull::writeTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_NULL);
}

void AmfNull::writeNullTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_NULL);
}

void AmfNull::readFrom(RtmpStream *stream) {}

int AmfNull::getSize() { return 1; }

int AmfUndefined::getSize() { return 1; }

void AmfUndefined::writeTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_UNDEFINED);
}

void AmfUndefined::writeUndefinedTo(RtmpStream *stream) {
    stream->write_1byte(AMF0_UNDEFINED);
}

void AmfUndefined::readFrom(RtmpStream *stream) {}

AmfData *AmfDecoder::decode(RtmpStream *stream) {
    byte amfTypeByte = stream->read_1byte();
    AmfType amfType = AmfType(amfTypeByte);
    AmfData *amfData = NULL;
    switch (amfType) {
        case AMF0_NUMBER:
            amfData = new AmfNumber;
            break;
        case AMF0_BOOLEAN:
            amfData = new AmfBoolean;
            break;
        case AMF0_STRING:
            amfData = new AmfString;
            break;
        case AMF0_OBJECT:
            amfData = new AmfObject;
            break;
        case AMF0_NULL:
            return new AmfNull;

        case AMF0_UNDEFINED:
            return new AmfUndefined;
        case AMF0_MAP:
            amfData = new AmfEcmaArray;
            break;
        case AMF0_ARRAY:
            amfData = new AmfArray;
            break;
        default:
            RTMP_LOG_INFO("unknow AMF0 TYPE:{0:x}", amfTypeByte);
            break;
    }
    if (NULL != amfData) {
        amfData->readFrom(stream);
    }
    return amfData;
}