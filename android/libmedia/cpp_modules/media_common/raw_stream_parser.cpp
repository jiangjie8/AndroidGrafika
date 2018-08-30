//
// Created by jiangjie on 2018/8/29.
//
#include "media_struct.h"
#include "raw_stream_parser.h"

enum {
    OBS_NAL_UNKNOWN = 0,
    OBS_NAL_SLICE = 1,
    OBS_NAL_SLICE_DPA = 2,
    OBS_NAL_SLICE_DPB = 3,
    OBS_NAL_SLICE_DPC = 4,
    OBS_NAL_SLICE_IDR = 5,
    OBS_NAL_SEI = 6,
    OBS_NAL_SPS = 7,
    OBS_NAL_PPS = 8,
    OBS_NAL_AUD = 9,
    OBS_NAL_FILLER = 12,
};


static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end) {
    const uint8_t *a = p + 4 - ((intptr_t) p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t *) p;
        //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
        //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p + 1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p + 2;
                if (p[4] == 0 && p[5] == 1)
                    return p + 3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}


static const uint8_t *plu_avc_find_startcode(const uint8_t *p, const uint8_t *end) {
    const uint8_t *out = ff_avc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1])
        out--;
    return out;
}


uint8_t *get_sps_pps(const uint8_t *data, size_t size, int &spspps_size) {
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    const uint8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
    int type;

    int sps_offset = 0, sps_size = 0;
    int pps_offset = 0, pps_size = 0;
    spspps_size = 0;

    nal_start = plu_avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++));

        if (nal_start == end)
            break;

        nal_end = plu_avc_find_startcode(nal_start, end);

        type = nal_start[0] & 0x1F;
        if (type == OBS_NAL_SPS) {
            sps_offset = nal_start - data;
            sps_size = nal_end - nal_start;
        } else if (type == OBS_NAL_PPS) {
            pps_offset = nal_start - data;
            pps_size = nal_end - nal_start;
        }
        nal_start = nal_end;
    }

    if(sps_size == 0 || pps_size == 0)
        return nullptr;

    int offset = 0;
    spspps_size = sps_size + 4 + pps_size + 4;
    uint8_t *spspps_buffer = (uint8_t*)av_malloc(spspps_size);
    if(sps_size > 0){
        memcpy(spspps_buffer + offset, start_code, 4);
        offset += 4;
        memcpy(spspps_buffer + offset, data + sps_offset, sps_size);
        offset += sps_size;
    }
    if(pps_size > 0){
        memcpy(spspps_buffer + offset, start_code, 4);
        offset += 4;
        memcpy(spspps_buffer + offset, data + pps_offset, pps_size);
        offset += pps_size;
    }
    return spspps_buffer;
}


