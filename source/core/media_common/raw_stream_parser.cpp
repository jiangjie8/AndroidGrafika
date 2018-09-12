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


typedef enum {
    H264_SEI_TYPE_BUFFERING_PERIOD = 0,   ///< buffering period (H.264, D.1.1)
    H264_SEI_TYPE_PIC_TIMING = 1,   ///< picture timing
    H264_SEI_TYPE_FILLER_PAYLOAD = 3,   ///< filler data
    H264_SEI_TYPE_USER_DATA_REGISTERED = 4,   ///< registered user data as specified by Rec. ITU-T T.35
    H264_SEI_TYPE_USER_DATA_UNREGISTERED = 5,   ///< unregistered user data
    H264_SEI_TYPE_RECOVERY_POINT = 6,   ///< recovery point (frame # to decoder sync)
    H264_SEI_TYPE_FRAME_PACKING = 45,  ///< frame packing arrangement
    H264_SEI_TYPE_DISPLAY_ORIENTATION = 47,  ///< display orientation
    H264_SEI_TYPE_GREEN_METADATA = 56,  ///< GreenMPEG information
    H264_SEI_TYPE_ALTERNATIVE_TRANSFER = 147, ///< alternative transfer
} H264_SEI_Type;



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


static const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end) {
    const uint8_t *out = ff_avc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1])
        out--;
    return out;
}


const uint8_t *get_sps_pps(const uint8_t *data, size_t size, int &spspps_size) {
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    const uint8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
    int type;

    int sps_offset = 0, sps_size = 0;
    int pps_offset = 0, pps_size = 0;
    spspps_size = 0;

    nal_start = avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++));

        if (nal_start == end)
            break;

        nal_end = avc_find_startcode(nal_start, end);

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


const uint8_t* decode_unregistered_user_data(const uint8_t *sei_start, const uint8_t *sei_end) {
    const uint8_t *uuid = sei_start;
    const uint8_t *payload = sei_start + 16;
    size_t sz = strlen((const char*)payload);
    if (sz > 0) {
        return payload;
    }
    return nullptr;
}



std::vector<std::string> ff_h264_sei_decode(const uint8_t *sei_start, const uint8_t *sei_end) {
    int offset = 0;
    const uint8_t *offset_ptr = sei_start;

    std::vector<std::string> sei_queue;
    while ((sei_end - offset_ptr) > 2 && *((uint16_t*)offset_ptr))
    {
        int type = 0;
        int16_t size = 0;
        int16_t next;
        int ret = 0;
        do
        {
            if ((sei_end - offset_ptr) < 1)
                return sei_queue;
            type += *((uint8_t*)offset_ptr);
        } while (*((uint8_t*)offset_ptr++) == 255);


        do
        {
            if ((sei_end - offset_ptr) < 1)
                return sei_queue;
            size += *((uint8_t*)offset_ptr);
        } while (*((uint8_t*)offset_ptr++) == 255);

        if (size > (sei_end - offset_ptr)) {
            LOGE("error SEI type %d size %d truncated at %d\n", type, size, sei_end - offset_ptr);
            return sei_queue;
        }
        switch (type)
        {
        case H264_SEI_TYPE_USER_DATA_UNREGISTERED: {
            const uint8_t *sei = decode_unregistered_user_data(offset_ptr, offset_ptr + size);
            if (sei != nullptr) {
                sei_queue.push_back((const char*)sei);
            }
            break;
        }

        default:
            break;
        }
        offset_ptr += size;
    }

    return sei_queue;
}

std::vector<std::string> vp_sei_user_data(const uint8_t *start, const uint8_t *end) {
    const uint8_t *end_ptr = end;
    const uint8_t *nal_start = start;
    const uint8_t *nal_end;
    int sei_size, type;

    nal_start = avc_find_startcode(nal_start, end_ptr);
    while (true) {
        while (nal_start < end_ptr && !(*(uint8_t*)nal_start++));

        if (nal_start == end_ptr)
            break;

        nal_end = avc_find_startcode(nal_start, end_ptr);
        sei_size = nal_end - nal_start;
        type = *(nal_start++) & 0x1F;
        if (type == OBS_NAL_SEI) {
            return ff_h264_sei_decode(nal_start, nal_end);
            break;
        }
        nal_start = nal_end;
    }
    return std::vector<std::string>();
}