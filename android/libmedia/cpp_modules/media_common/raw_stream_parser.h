//
// Created by jiangjie on 2018/8/29.
//

#ifndef ANDROIDGRAFIKA_RAW_STREAM_PARSER_H
#define ANDROIDGRAFIKA_RAW_STREAM_PARSER_H

#include <inttypes.h>

uint8_t *get_sps_pps(const uint8_t *data, size_t size, int &spspps_size);


#endif //ANDROIDGRAFIKA_RAW_STREAM_PARSER_H
