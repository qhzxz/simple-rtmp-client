//
// Created by 秦汉 on 16/7/14.
//


#include "rtmp_utility.h"

int readUnsignedInt32(RtmpStream *stream) {
    return ((stream->read_1byte()) << 24) | ((stream->read_1byte()) << 16) |
           ((stream->read_1byte()) << 8) | (stream->read_1byte());

}

int readUnsignedInt24(RtmpStream *stream) {
    return ((stream->read_1byte()) << 16) |
           ((stream->read_1byte()) << 8) | (stream->read_1byte());
}

int16_t readUnsignedInt16(RtmpStream *stream) {
    return ((stream->read_1byte()) << 8) | (stream->read_1byte());
}


int32_t toUnsignedInt32LittleEndian(RtmpStream *stream) {
    return ((stream->read_1byte()) << 24) | ((stream->read_1byte()) << 16) |
           ((stream->read_1byte()) << 8) | (stream->read_1byte());
};


int32_t toUnsignedInt32LittleEndian(byte *bytes) {
    return ((bytes[3]) << 24) | ((bytes[2]) << 16) | ((bytes[1]) << 8) | (bytes[0]);
};

void writeUnsignedInt16(RtmpStream *stream, int16_t value) {
    stream->write_1byte((byte) (value >> 8));
    stream->write_1byte((byte) (value));
};

void writeUnsignedInt24(RtmpStream *stream, int32_t value) {
    stream->write_1byte((byte) (value >> 16));
    stream->write_1byte((byte) (value >> 8));
    stream->write_1byte((byte) (value));
};

void writeUnsignedInt32(RtmpStream *stream, int32_t value) {
    stream->write_1byte((byte) (value >> 24));
    stream->write_1byte((byte) (value >> 16));
    stream->write_1byte((byte) (value >> 8));
    stream->write_1byte((byte) (value));
};


void writeUnsignedInt32LittleEndian(RtmpStream *stream, int32_t value) {
    stream->write_1byte((byte) (value));
    stream->write_1byte((byte) (value >> 8));
    stream->write_1byte((byte) (value >> 16));
    stream->write_1byte((byte) (value >> 24));
}

void writeDouble(RtmpStream *stream, double d) {
    int64_t temp = 0x00;
    memcpy(&temp, &d, 8);
    byte *p_temp = (byte *) &temp;
    stream->write_1byte(p_temp[7] & 0xff);
    stream->write_1byte(p_temp[6] & 0xff);
    stream->write_1byte(p_temp[5] & 0xff);
    stream->write_1byte(p_temp[4] & 0xff);
    stream->write_1byte(p_temp[3] & 0xff);
    stream->write_1byte(p_temp[2] & 0xff);
    stream->write_1byte(p_temp[1] & 0xff);
    stream->write_1byte(p_temp[0] & 0xff);
}


double readDouble(RtmpStream *stream) {
    int64_t temp = 0x00;
    byte *p_temp = (byte *) &temp;
    p_temp[7] = stream->read_1byte();
    p_temp[6] = stream->read_1byte();
    p_temp[5] = stream->read_1byte();
    p_temp[4] = stream->read_1byte();
    p_temp[3] = stream->read_1byte();
    p_temp[2] = stream->read_1byte();
    p_temp[1] = stream->read_1byte();
    p_temp[0] = stream->read_1byte();
    double value = 0;
    memcpy(&value, p_temp, 8);
    return value;
}

void readBytesUntilFull(RtmpStream *stream, byte *targetBuffer, int length) {
    int totalBytesRead = 0;
    int read;
    const int targetBytes = length;
    do {
        read = stream->read_bytes(targetBuffer, (targetBytes - totalBytesRead));
        if (read != -1) {
            totalBytesRead += read;
        } else {
//                throw new IOException("Unexpected EOF reached before read buffer was filled");
        }
    } while (totalBytesRead < targetBytes);
}
