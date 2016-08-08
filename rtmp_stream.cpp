//
// Created by 秦汉 on 16/7/15.
//

#include <cstddef>
#include <cstring>
#include <assert.h>
#include "rtmp_stream.h"

RtmpByteArrayStream::RtmpByteArrayStream(int len) : len(len) {
    this->bytes = this->current_position_pointer = new byte[len];
}

byte *RtmpByteArrayStream::data() {
    return bytes;
}

bool RtmpByteArrayStream::empty() {
    return !bytes || (current_position_pointer >= bytes + len);
}

RtmpByteArrayStream::~RtmpByteArrayStream() {
    delete[]bytes;
}

int RtmpByteArrayStream::size() {
    return len;
}

int RtmpByteArrayStream::pos() {
    int position = (int) (current_position_pointer - bytes);
    return position;
}

void RtmpByteArrayStream::skip(int size) {
    current_position_pointer += size;
}

int RtmpByteArrayStream::read_bytes(byte *data, int size) {
    assert(require(size));
    memcpy(data, current_position_pointer, size);
    current_position_pointer += size;
    return size;
}


int RtmpByteArrayStream::write_bytes(byte *data, int size) {
    assert(require(size));
    memcpy(current_position_pointer, data, size);
    current_position_pointer += size;
    return size;
}

void RtmpByteArrayStream::reset() {
    current_position_pointer = bytes;
}

bool RtmpByteArrayStream::require(int size) {
    return size <= len - (current_position_pointer - bytes);
}

byte RtmpByteArrayStream::read_1byte() {
    assert(require(1));
    return (byte) *current_position_pointer++;
}


int RtmpByteArrayStream::write_1byte(byte value) {
    assert(require(1));
    * current_position_pointer++ = value;
    return 1;
}

RtmpByteArrayStream::RtmpByteArrayStream(byte *data, int len) : len(len) {
    this->bytes = this->current_position_pointer = new byte[len];
    if (NULL != data) {
        memcpy(this->bytes, data, len);
    }

}