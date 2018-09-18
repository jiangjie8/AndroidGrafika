#ifndef CORE_MEDIA_COMMON_BSF_FILTER_H
#define CORE_MEDIA_COMMON_BSF_FILTER_H

#include "core/media_common/media_struct.h"

AVBSFContext* initBsfFilter(const AVCodecParameters *codecpar, const char *filter_name);
int applyBitstream(AVBSFContext *bsf, AVPacket *packet);





#endif