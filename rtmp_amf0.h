//
// Created by 秦汉 on 16/7/16.
//

#ifndef SOCKET_RTMP_AMF0_H
#define SOCKET_RTMP_AMF0_H

#include "rtmp_stream.h"
#include <map>
#include <vector>
#include <string>

enum AmfType {

    /** Number (encoded as IEEE 64-bit double precision floating point number) */
            AMF0_NUMBER = 0x00,
    /** Boolean (Encoded as a single byte of value 0x00 or 0x01) */
            AMF0_BOOLEAN = 0x01,
    /** String (ASCII encoded) */
            AMF0_STRING = 0x02,
    /** Object - set of key/value pairs */
            AMF0_OBJECT = 0x03,
    AMF0_NULL = 0x05,
    AMF0_UNDEFINED = 0x06,
    AMF0_MAP = 0x08,
    AMF0_ARRAY = 0x0A
};


class AmfData {
public:
    virtual int getSize() = 0;

    virtual void writeTo(RtmpStream *stream) = 0;

    virtual void readFrom(RtmpStream *stream) = 0;

    virtual ~AmfData();

};

class AmfArray : public AmfData {

private:

    int size = -1;
    std::vector<AmfData *> *items = NULL;
public:
    AmfArray();

    ~AmfArray();

    virtual int getSize();

    virtual void writeTo(RtmpStream *stream);

    virtual void readFrom(RtmpStream *stream);

    const std::vector<AmfData *> *getitems();

    void addItems(AmfData *data);

    void clear();

};

class AmfBoolean : public AmfData {

private :
    bool value;

public :
    bool isValue();

    void setValue(bool value);

    AmfBoolean(bool value);


    AmfBoolean();

    virtual void writeTo(RtmpStream *stream);


    virtual void readFrom(RtmpStream *stream);


    static bool readBooleanFrom(RtmpStream *stream);

    int getSize();
};

class AmfString : public AmfData {
private:
    std::string value;
    int size = -1;

public:

    AmfString();


    AmfString(std::string value);


    std::string getValue();


    void setValue(std::string value);


    virtual void writeTo(RtmpStream *stream);

    virtual void readFrom(RtmpStream *stream);


    int getSize();
};


class AmfNumber : public AmfData {
private:
    double value;
    /** Size of an AMF number, in bytes (including type bit) */
public:
    static const int SIZE = 9;

    AmfNumber(double value);

    AmfNumber();

    double getValue();

    void setValue(double value);

    void writeTo(RtmpStream *stream);

    void readFrom(RtmpStream *stream);

    int getSize();

    static double readNumberFrom(RtmpStream *in);

};

class AmfObject : public AmfData {

protected :
    typedef std::map<std::string, AmfData *> PropertyMap;
    typedef std::pair<std::string, AmfData *> PropertyPair;
    PropertyMap *properties = NULL;
    int size = -1;
/** Byte sequence that marks the end of an AMF object */
    static const byte OBJECT_END_MARKER[3];

public :
    AmfObject();

    AmfData *getProperty(std::string key);

    virtual void setDataProperty(std::string key, AmfData *value);

    virtual void setBoolProperty(std::string key, bool value);

    virtual void setStringProperty(std::string key, std::string value);

    virtual void setIntProperty(std::string key, int value);

    virtual void setDoubleProperty(std::string key, double value);

    virtual void writeTo(RtmpStream *out);

    virtual void readFrom(RtmpStream *in);

    virtual int getSize();

    virtual ~AmfObject();
};


class AmfEcmaArray : public AmfObject {
public:
    virtual void writeTo(RtmpStream *out);

    virtual void readFrom(RtmpStream *in);

    virtual int getSize();
};

class AmfNull : public AmfData {
public:
    void writeTo(RtmpStream *stream);

    void readFrom(RtmpStream *stream);

    static void writeNullTo(RtmpStream *stream);

    int getSize();
};


class AmfUndefined : public AmfData {
public:
    void writeTo(RtmpStream *stream);

    void readFrom(RtmpStream *stream);

    static void writeUndefinedTo(RtmpStream *stream);

    int getSize();
};

class AmfDecoder {
public:
    static AmfData *decode(RtmpStream *stream);

};

#endif //SOCKET_RTMP_AMF0_H
