//
// Created by 秦汉 on 16/7/26.
//

#include <sys/time.h>
#include <assert.h>
#include "rtmp_complex_hand_shake_v3.h"
#include "rtmp_log.h"


RtmpHandShake::RtmpHandShake() {
    srand((unsigned) time(NULL));
}

byte RtmpHandShake::getRandomByte() {
    return (byte) (rand() % 256);
}

void RtmpHandShake::getRandomByteArray(byte *b_arr, int len) {
    for (int i = 0; i < len; ++i) {
        b_arr[i] = getRandomByte();
    }
}

void RtmpHandShake::copyByteArray(byte *src, int src_start, byte *dst, int dst_start, int length) {
    for (int i = 0; i < length; ++i) {
        dst[dst_start + i] = src[src_start + i];
    }
}


int RtmpHandShake::do_openssl_HMACsha256(HMAC_CTX *ctx, const void *data, int data_size, void *digest,
                                         unsigned int *digest_size) {
    int ret;

    if (HMAC_Update(ctx, (unsigned char *) data, data_size) < 0) {
        ret = -2;
        return ret;
    }

    if (HMAC_Final(ctx, (unsigned char *) digest, digest_size) < 0) {
        ret = -3;
        return ret;
    }

    return ret;
}

int RtmpHandShake::openssl_HMACsha256(const void *key, int key_size, const void *data, int data_size, void *digest) {
    int ret = RESULT_SUCCESS;

    unsigned int digest_size = 0;

    unsigned char *temp_key = (unsigned char *) key;
    unsigned char *temp_digest = (unsigned char *) digest;


    HMAC_CTX ctx;

    // @remark, if no key, use EVP_Digest to digest,
    // for instance, in python, hashlib.sha256(data).digest().
    HMAC_CTX_init(&ctx);
    if (HMAC_Init_ex(&ctx, temp_key, key_size, EVP_sha256(), NULL) < 0) {
        ret = -2;
        return ret;
    }

    ret = do_openssl_HMACsha256(&ctx, data, data_size, temp_digest, &digest_size);
    HMAC_CTX_cleanup(&ctx);

    return ret;
}

int RtmpHandShake::getDigestOffset1(byte *bytes) {
    int offset = bytes[0] & 255;
    offset += bytes[1] & 255;
    offset += bytes[2] & 255;
    offset += bytes[3] & 255;
    int res = offset % 728 + 12;
    return res;
}

int RtmpHandShake::handShake(RtmpSocket &rtmpSocket) {

    int ret = RESULT_SUCCESS;

    byte client_version = 0x03;
    byte server_s0s1[1537];


    struct timeval tv;
    gettimeofday(&tv, NULL);
    int time = (int) (tv.tv_sec / 1000);
    byte timeStamp[4];
    unsignedInt32ToByteArray(timeStamp, time);
    byte versions[4] = {0x80, 0x00, 0x07, 0x02};


    int rangOfDigestOffset = BLOCK_SIZE - OFFSET_SIZE - SHA256_DIGEST_SIZE;
    int digestOffset =
            rand() % rangOfDigestOffset;
    int sizeBeforeDigest = TIME_SIZE + VERSION_SIZE + OFFSET_SIZE + digestOffset;
    int remaining = digestOffset;
    byte digestOffsetBytes[4];
    for (int i = 0; i < 4; i++) {
        if (remaining > 255) {
            digestOffsetBytes[i] = (byte) 255;
            remaining -= 255;
        } else {
            digestOffsetBytes[i] = (byte) remaining;
            remaining -= remaining;
        }
    }

    int rangeOfKeyOffset = BLOCK_SIZE - OFFSET_SIZE - KEY_SIZE;
    int keyOffset = rand() % rangeOfKeyOffset;
    byte keyRandomData[keyOffset];
    getRandomByteArray(keyRandomData, keyOffset);

    byte keyData[KEY_SIZE];
    getRandomByteArray(keyData, KEY_SIZE);

    byte restKeyRandomData[rangeOfKeyOffset - keyOffset];
    getRandomByteArray(restKeyRandomData, rangeOfKeyOffset - keyOffset);

    remaining = keyOffset;
    byte keyOffsetByte[4];
    for (int i = 3; i >= 0; i--) {
        if (remaining > 255) {
            keyOffsetByte[i] = (byte) 255;
            remaining -= 255;
        } else {
            keyOffsetByte[i] = (byte) remaining;
            remaining -= remaining;
        }
    }

    byte partBeforeDigest[sizeBeforeDigest];
    getRandomByteArray(partBeforeDigest, sizeBeforeDigest);
    copyByteArray(timeStamp, 0, partBeforeDigest, 0, 4);
    copyByteArray(versions, 0, partBeforeDigest, 4, 4);
    copyByteArray(digestOffsetBytes, 0, partBeforeDigest, 8, 4);

    int restSize = HANDSHAKE_SIZE - sizeBeforeDigest - SHA256_DIGEST_SIZE;
    byte partAfterDigest[restSize]; // subtract 8 because of initial 8 bytes already written
    getRandomByteArray(partAfterDigest, restSize);

    int keyBlockPosition = restSize - BLOCK_SIZE;
    copyByteArray(keyRandomData, 0, partAfterDigest, keyBlockPosition, keyOffset);
    keyBlockPosition += keyOffset;
    copyByteArray(keyData, 0, partAfterDigest, keyBlockPosition, KEY_SIZE);
    keyBlockPosition += KEY_SIZE;
    copyByteArray(restKeyRandomData, 0, partAfterDigest, keyBlockPosition, rangeOfKeyOffset - keyOffset);
    keyBlockPosition += (rangeOfKeyOffset - keyOffset);
    copyByteArray(keyOffsetByte, 0, partAfterDigest, +keyBlockPosition, OFFSET_SIZE);
    byte tempBuffer[HANDSHAKE_SIZE - SHA256_DIGEST_SIZE];
    copyByteArray(partBeforeDigest, 0, tempBuffer, 0, sizeBeforeDigest);
    copyByteArray(partAfterDigest, 0, tempBuffer, sizeBeforeDigest, restSize);

    byte c1_digest[SHA256_DIGEST_SIZE];
    openssl_HMACsha256(GENUINE_FP_KEY, 30, tempBuffer, HANDSHAKE_SIZE - SHA256_DIGEST_SIZE, c1_digest);

    ret = rtmpSocket.write_bytes(&client_version, 1, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }
    ret = rtmpSocket.write_bytes(partBeforeDigest, sizeBeforeDigest, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }
    ret = rtmpSocket.write_bytes(c1_digest, SHA256_DIGEST_SIZE, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }
    ret = rtmpSocket.write_bytes(partAfterDigest, restSize, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }

    ret = rtmpSocket.read_full_bytes(server_s0s1, 1537, NULL);
    RTMP_LOG_INFO("server version :{0:02x}", server_s0s1[0]);
    if (ret != RESULT_SUCCESS) { return ret; }
    byte s1_digest_offset_bytes[4];
    copyByteArray(server_s0s1, 1, s1_digest_offset_bytes, 0, 4);
    int s1_digest_offset = getDigestOffset1(s1_digest_offset_bytes);
    byte s1_digest[SHA256_DIGEST_SIZE];
    copyByteArray(server_s0s1, s1_digest_offset + 1, s1_digest, 0, SHA256_DIGEST_SIZE);


    byte c2_random_data[1504];
    getRandomByteArray(c2_random_data, 1504);
    byte c1_temp_key[32];
    openssl_HMACsha256(GENUINE_FP_KEY, 62, s1_digest, 32, c1_temp_key);
    byte c2_digest[32];
    openssl_HMACsha256(c1_temp_key, 32, c2_random_data, 1504, c2_digest);
    ret = rtmpSocket.write_bytes(c2_random_data, 1504, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }
    ret = rtmpSocket.write_bytes(c2_digest, 32, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }
    byte s2[HANDSHAKE_SIZE];
    ret = rtmpSocket.read_full_bytes(s2, HANDSHAKE_SIZE, NULL);
    if (ret != RESULT_SUCCESS) { return ret; }
    byte s2_random_data[1504];
    byte result[32];
    copyByteArray(s2, 0, s2_random_data, 0, 1504);
    copyByteArray(s2, 1504, result, 0, 32);
    byte s1_temp_key[32];
    openssl_HMACsha256(GENUINE_FMS_KEY, 68, c1_digest, 32, s1_temp_key);
    byte s2_digest[SHA256_DIGEST_SIZE];
    openssl_HMACsha256(s1_temp_key, 32, s2_random_data, 1504, s2_digest);
    for (int j = 0; j < 32; ++j) {
        if (s2_digest[j] != result[j]) {
            ret = RESULT_HAND_SHAKE_NOT_VALIDATE;
            break;
        }
    }
    return ret;
}


void RtmpHandShake::unsignedInt32ToByteArray(byte *byteArr, int value) {
    byteArr[0] = (byte) (value >> 24);
    byteArr[1] = (byte) (value >> 16);
    byteArr[2] = (byte) (value >> 8);
    byteArr[3] = (byte) value;
}