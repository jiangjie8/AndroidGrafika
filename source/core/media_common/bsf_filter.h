#ifndef BSF_FILTER_H
#define BSF_FILTER_H

#include "core/media_common/media_struct.h"

AVBSFContext* initBsfFilter(const AVCodecParameters *codecpar, const char *filter_name);
int applyBitstream(AVBSFContext *bsf, AVPacket *packet);





#endif