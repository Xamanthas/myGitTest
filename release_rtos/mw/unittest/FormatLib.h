#ifndef __FORMAT_LIB_H__

#include <format/Format.h>

int FormatLib_ResetMuxMediaInfo(AMP_MEDIA_INFO_s *media);

int FormatLib_RestoreDTS(AMP_MEDIA_INFO_s *media);

int FormatLib_ResetDmxMediaInfo(AMP_MEDIA_INFO_s *media);

int FormatLib_AdjustDTS(AMP_MOVIE_INFO_s *movie);

AMP_MEDIA_TRACK_INFO_s *FormatLib_GetDefaultTrack(AMP_MEDIA_INFO_s *media, UINT8 trackType);

AMP_MEDIA_TRACK_INFO_s *FormatLib_GetShortestTrack(AMP_MEDIA_INFO_s *media);

void FormatLib_InitPTS(AMP_MEDIA_TRACK_INFO_s *track);

UINT64 FormatLib_ConvertPTS(AMP_MEDIA_TRACK_INFO_s *track, AMP_BITS_DESC_s *frame);

void FormatLib_ResetPTS(AMP_MEDIA_TRACK_INFO_s *track);

BOOL FormatLib_IsVideoComplete(AMP_MOVIE_INFO_s *movie);

BOOL FormatLib_CheckEnd(AMP_MEDIA_INFO_s *media);

void FormatLib_UpdateMuxTrack(AMP_MEDIA_TRACK_INFO_s *track, UINT32 frameCount);

#define __FORMAT_LIB_H__
#endif
