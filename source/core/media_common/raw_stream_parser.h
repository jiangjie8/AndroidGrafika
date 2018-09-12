//
// Created by jiangjie on 2018/8/29.
//

#ifndef ANDROIDGRAFIKA_RAW_STREAM_PARSER_H
#define ANDROIDGRAFIKA_RAW_STREAM_PARSER_H

#include <inttypes.h>
#include <vector>
const uint8_t *get_sps_pps(const uint8_t *data, size_t size, int &spspps_size);
std::vector<std::string> vp_sei_user_data(const uint8_t *start, const uint8_t *end);

#endif //ANDROIDGRAFIKA_RAW_STREAM_PARSER_H
