#include "FormatLib.h"

#define FormatLib_Perror(level, color, fmt, ...) {\
    char Buffer[160];\
    snprintf(Buffer, 160, fmt, ##__VA_ARGS__);\
    Buffer[159] = '\0';\
    AmbaPrintColor(color, "[Error]%s:%u %s", __FUNCTION__, __LINE__, Buffer);\
}

static void FormatLib_ResetMuxTrack(AMP_MEDIA_TRACK_INFO_s *track)
{
    track->FrameCount = 0;
    track->FrameNo = 0;
}

static int FormatLib_ResetMuxMovieInfo(AMP_MOVIE_INFO_s *movie)
{
    UINT32 i;
    for (i=0; i<movie->TrackCount; i++) {
        FormatLib_ResetMuxTrack(&movie->Track[i]);
    }
    return 0;
}

static int FormatLib_ResetMuxSoundInfo(AMP_SOUND_INFO_s *sound)
{
    UINT32 i;
    for (i=0; i<sound->TrackCount; i++) {
        K_ASSERT(sound->Track[i].TrackType == AMP_MEDIA_TRACK_TYPE_AUDIO);
        FormatLib_ResetMuxTrack(&sound->Track[i]);
    }
    return 0;
}

int FormatLib_ResetMuxMediaInfo(AMP_MEDIA_INFO_s *media)
{
    switch (media->MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        if (FormatLib_ResetMuxMovieInfo((AMP_MOVIE_INFO_s *)media) == 0)
            return AMP_OK;
        break;
    case AMP_MEDIA_INFO_IMAGE:
        return AMP_OK;
    case AMP_MEDIA_INFO_SOUND:
        if (FormatLib_ResetMuxSoundInfo((AMP_SOUND_INFO_s *)media) == 0)
            return AMP_OK;
        break;
    default:
        FormatLib_Perror(0, 0, "incorrect media type");
        break;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

static int FormatLib_RestoreDTSMovie(AMP_MOVIE_INFO_s *movie)
{
    UINT32 i;
    for (i=0; i<movie->TrackCount; i++) {
        movie->Track[i].DTS = movie->Track[i].NextDTS;
    }
    return 0;
}

static int FormatLib_RestoreDTSSound(AMP_SOUND_INFO_s *sound)
{
    UINT32 i;
    for (i=0; i<sound->TrackCount; i++) {
        if (sound->Track[i].TrackType != AMP_MEDIA_TRACK_TYPE_AUDIO) {
            FormatLib_Perror(0, 0, "incorrect track type");
            return -1;
        }
        sound->Track[i].DTS = sound->Track[i].NextDTS;
    }
    return 0;
}

int FormatLib_RestoreDTS(AMP_MEDIA_INFO_s *media)
{
    switch (media->MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        if (FormatLib_RestoreDTSMovie((AMP_MOVIE_INFO_s *)media) < 0)
            return AMP_ERROR_GENERAL_ERROR;
        break;
    case AMP_MEDIA_INFO_IMAGE:
        break;
    case AMP_MEDIA_INFO_SOUND:
        if (FormatLib_RestoreDTSSound((AMP_SOUND_INFO_s *)media) < 0)
            return AMP_ERROR_GENERAL_ERROR;
        break;
    default:
        FormatLib_Perror(0, 0, "incorrect media type");
        return AMP_ERROR_GENERAL_ERROR;
    }
    return AMP_OK;
}

static void FormatLib_ResetDmxTrack(AMP_MEDIA_TRACK_INFO_s *track, AMP_FIFO_HDLR_s *fifo)
{
    track->FrameNo = 0;
    if (fifo != NULL)
        track->DTS = track->InitDTS;
    else
        track->DTS = AMP_FORMAT_MAX_TIMESTAMP;
}

static int FormatLib_ResetDmxMovieInfo(AMP_MOVIE_INFO_s *movie)
{
    UINT32 i;
    AMP_MEDIA_TRACK_INFO_s *Track;
    for (i=0; i<movie->TrackCount; i++) {
        Track = &movie->Track[i];
        FormatLib_ResetDmxTrack(Track, Track->Fifo);
    }
    return 0;
}

static int FormatLib_ResetDmxSoundInfo(AMP_SOUND_INFO_s *sound)
{
    UINT32 i;
    AMP_MEDIA_TRACK_INFO_s *Track;
    for (i=0; i<sound->TrackCount; i++) {
        Track = &sound->Track[i];
        K_ASSERT(Track->TrackType == AMP_MEDIA_TRACK_TYPE_AUDIO);
        FormatLib_ResetDmxTrack(Track, Track->Fifo);
    }
    return 0;
}

int FormatLib_ResetDmxMediaInfo(AMP_MEDIA_INFO_s *media)
{
    switch (media->MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        if (FormatLib_ResetDmxMovieInfo((AMP_MOVIE_INFO_s *)media) == 0)
            return AMP_OK;
        break;
    case AMP_MEDIA_INFO_IMAGE:
        return AMP_OK;
    case AMP_MEDIA_INFO_SOUND:
        if (FormatLib_ResetDmxSoundInfo((AMP_SOUND_INFO_s *)media) == 0)
            return AMP_OK;
        break;
    default:
        FormatLib_Perror(0, 0, "incorrect media type");
        break;
    }
    return AMP_ERROR_GENERAL_ERROR;
}

AMP_MEDIA_TRACK_INFO_s *FormatLib_GetDefaultTrack(AMP_MEDIA_INFO_s *media, UINT8 trackType)
{
    UINT32 i;
    K_ASSERT(media != NULL);
    if (media->MediaType == AMP_MEDIA_INFO_MOVIE) {
        AMP_MOVIE_INFO_s *Movie = (AMP_MOVIE_INFO_s *)media;
        for (i=0; i<Movie->TrackCount; i++) {
            if (Movie->Track[i].TrackType == trackType) {
                if (trackType == AMP_MEDIA_TRACK_TYPE_VIDEO) {
                    if (Movie->Track[i].Info.Video.IsDefault)
                        return &Movie->Track[i];
                } else if (trackType == AMP_MEDIA_TRACK_TYPE_AUDIO) {
                    if (Movie->Track[i].Info.Audio.IsDefault)
                        return &Movie->Track[i];
                } else if (trackType == AMP_MEDIA_TRACK_TYPE_TEXT) {
                    if (Movie->Track[i].Info.Text.IsDefault)
                        return &Movie->Track[i];
                }
            }
        }
        FormatLib_Perror(0, 0, "track not found");
        return NULL;
    }
    if (media->MediaType == AMP_MEDIA_INFO_SOUND) {
        AMP_SOUND_INFO_s *Sound = (AMP_SOUND_INFO_s *)media;
        K_ASSERT(trackType == AMP_MEDIA_TRACK_TYPE_AUDIO);
        for (i=0; i<Sound->TrackCount; i++) {
            K_ASSERT(Sound->Track[i].TrackType == AMP_MEDIA_TRACK_TYPE_AUDIO);
            if (Sound->Track[i].Info.Audio.IsDefault)
                return &Sound->Track[i];
        }
        FormatLib_Perror(0, 0, "track not found");
        return NULL;
    }
    FormatLib_Perror(0, 0, "incorrect media type");
    return NULL;
}

AMP_MEDIA_TRACK_INFO_s *FormatLib_GetShortestTrack(AMP_MEDIA_INFO_s *media)
{
    AMP_MEDIA_TRACK_INFO_s *Track = NULL;
    AMP_MEDIA_TRACK_INFO_s *p;
    AMP_MOVIE_INFO_s *Movie;
    AMP_SOUND_INFO_s *Sound;
    UINT32 TrackCount;
    UINT64 DTS = AMP_FORMAT_MAX_TIMESTAMP;
    UINT32 i;
    UINT64 TmpDTS;
    K_ASSERT(media != NULL);
    switch (media->MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        Movie = (AMP_MOVIE_INFO_s *)media;
        TrackCount = Movie->TrackCount;
        // return the track with the min DTS
        for (i=0; i<TrackCount; i++) {
            p = &Movie->Track[i];
            K_ASSERT(p->TrackType < AMP_MEDIA_TRACK_TYPE_MAX);
            TmpDTS = p->DTS;
            if (TmpDTS < DTS) {
                DTS = TmpDTS;
                Track = p;
            }
        }
        break;
    case AMP_MEDIA_INFO_SOUND:
        Sound = (AMP_SOUND_INFO_s *)media;
        TrackCount = Sound->TrackCount;
        // return the track with the min DTS
        for (i=0; i<TrackCount; i++) {
            p = &Sound->Track[i];
            K_ASSERT(p->TrackType == AMP_MEDIA_TRACK_TYPE_AUDIO);
            TmpDTS = p->DTS;
            if (TmpDTS < DTS) {
                DTS = TmpDTS;
                Track = p;
            }
        }
        break;
    default:
        FormatLib_Perror(0, 0, "incorrect media type");
        break;
    }
    return Track;
}

void FormatLib_InitPTS(AMP_MEDIA_TRACK_INFO_s *track)
{
    AMP_VIDEO_TRACK_INFO_s * const Video = &track->Info.Video;
    K_ASSERT(track != NULL);
    K_ASSERT(track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO);
    if (!Video->ClosedGOP)
        Video->DiffPTS = 0 - (UINT64)((Video->M - 1) * track->TimePerFrame);
    Video->FrameCountAfterResume = 0;
    Video->InitPTS = 0;
    Video->PTS = 0;
}

static int FormatLib_SyncPTS(AMP_MEDIA_TRACK_INFO_s *track, UINT64 pts, UINT32 frameType)
{
    AMP_VIDEO_TRACK_INFO_s * const Video = &track->Info.Video;
    const UINT32 TimePerFrame = (Video->Mode == AMP_VIDEO_MODE_I_A_FLD_PER_SMP) ? (track->TimePerFrame << 1) : track->TimePerFrame;
    const UINT32 InitTimePerFrame = TimePerFrame / Video->VFR;
    K_ASSERT(track != NULL);
    K_ASSERT(track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO);
    K_ASSERT(TimePerFrame % Video->VFR == 0);
    if (Video->M != 1) {  // IBBP series...
        if (frameType == AMP_FIFO_TYPE_B_FRAME) {
            if (pts != (track->DTS - Video->RefDTS) - TimePerFrame) {
                AmbaPrint("%s(%p, %llu, %x) : No support : the frame rate change at B nM = %u, DTS = %llu, TimePerFrame = %u", __FUNCTION__, Video, pts, frameType, Video->M, track->DTS, track->TimePerFrame);
                return AMP_ERROR_GENERAL_ERROR;
            }
        } else {
            if (pts != (track->DTS - Video->RefDTS) + (Video->M - 1) * TimePerFrame) {
                AmbaPrint("%s(%p, %llu, %x) : No support : The frame rate change at I,IDR,P nM = %u, DTS = %llu, TimePerFrame = %u", __FUNCTION__, Video, pts, frameType, Video->M, track->DTS, track->TimePerFrame);
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    } else {    //IPPP series...
        const int AddVdelta = (int)(pts - (track->DTS - Video->RefDTS));
        if (AddVdelta != 0) {
            if (abs(AddVdelta) % InitTimePerFrame == 0) {
                AmbaPrint("%s(%u), PTS = %llu, track->DTS = %llu, TimePerFrame = %u, TimeScale = %u ", __FUNCTION__, track->FrameNo, pts, track->DTS, TimePerFrame, track->TimeScale);
                Video->IsVFR = TRUE;
                if (Video->Mode == AMP_VIDEO_MODE_I_A_FLD_PER_SMP) {
                    K_ASSERT(abs(AddVdelta) % 2 == 0);
                    track->TimePerFrame = track->TimePerFrame + AddVdelta / 2;
                    Video->VFR = track->TimePerFrame / (InitTimePerFrame >> 1);
                } else {
                    track->TimePerFrame = track->TimePerFrame + AddVdelta;
                    Video->VFR = track->TimePerFrame / InitTimePerFrame;
                }
                if (Video->VFR > AMP_FORMAT_MAX_VFR_FACTOR) {
                    AmbaPrint("%s, VFR reaches MAX_VFR_FACTOR (%u), check dsp PTS is abnormal or not !!! ", __FUNCTION__, Video->VFR);
                    return AMP_ERROR_GENERAL_ERROR;
                }
                track->DTS += AddVdelta;
                track->NextDTS = track->DTS;
                AmbaPrint("%s, Detect TimePerFrame is changed to %u  %llu", __FUNCTION__, track->TimePerFrame, track->NextDTS);
            } else {
                AmbaPrint("%s(%u), AddVdelta error! pts = %llu, track->DTS = %llu, TimePerFrame = %u", __FUNCTION__, track->FrameNo, pts, track->DTS, TimePerFrame);
                return AMP_ERROR_GENERAL_ERROR;
            }
        }
    }
    return AMP_OK;
}

UINT64 FormatLib_ConvertPTS(AMP_MEDIA_TRACK_INFO_s *track, AMP_BITS_DESC_s *frame)
{
    AMP_VIDEO_TRACK_INFO_s *const Video = &track->Info.Video;
    INT64 Diff;
    UINT64 PTS = frame->Pts;
    UINT32 TimePerFrame = track->TimePerFrame / Video->VFR;
    K_ASSERT(track != NULL);
    K_ASSERT(frame != NULL);
    K_ASSERT(track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO);
    K_ASSERT(track->TimePerFrame % Video->VFR == 0);
    if (Video->VFR <= AMP_FORMAT_MAX_VFR_FACTOR) {
        if (Video->Mode == AMP_VIDEO_MODE_I_A_FLD_PER_SMP) {
            TimePerFrame = TimePerFrame << 1;
        }
        //AmbaPrint("%s() : DspPTS=%u, TimePerFrame=%u, TimeScale=%u, PTS=%llu, InitPTS=%llu, nStepPTS=%llu, nDiffPTS=%llu, CodecTimeScale=%u, RefDTS=%llu", __FUNCTION__, frame->Pts, track->TimePerFrame, track->TimeScale, Video->PTS, Video->InitPTS, Video->StepPTS, Video->DiffPTS, Video->CodecTimeScale, Video->RefDTS);
        //AmbaPrint("%s() : DspPTS=%llu, TimePerFrame=%u, TimeScale=%u, InitDTS=%llu, RefDTS=%llu, DTS=%llu", __FUNCTION__, frame->Pts, track->TimePerFrame, track->TimeScale, track->InitDTS, Video->RefDTS, track->DTS);
        Diff = PTS - Video->InitPTS;
        Video->PTS = (UINT64)Diff - (Video->DiffPTS * Video->CodecTimeScale / track->TimeScale);
        Video->PTS = Video->PTS * track->TimeScale / Video->CodecTimeScale;
        Video->PTS = ((Video->PTS + (TimePerFrame >> 1)) / TimePerFrame) * TimePerFrame;
        if (FormatLib_SyncPTS(track, Video->PTS, frame->Type) == AMP_OK ) {
            return Video->PTS;
        }
    } else {
        FormatLib_Perror(0, 0, "Reach Max VFR factor!");
    }
    return AMP_FORMAT_MAX_TIMESTAMP;
}

int FormatLib_AdjustDTS(AMP_MOVIE_INFO_s *movie)
{
    const UINT32 TrackCount = movie->TrackCount;
    AMP_MEDIA_TRACK_INFO_s *Track;
    AMP_MEDIA_TRACK_INFO_s TmpTrack;
    AMP_BITS_DESC_s *Frame;
    UINT8 TrackId;
    int MinDiff = 0;
    int Diff;
    UINT64 Pts = 0;
    K_ASSERT(movie != NULL);
    K_ASSERT(movie->MediaType == AMP_MEDIA_INFO_MOVIE);
    for (TrackId = 0; TrackId < TrackCount; TrackId++) {
        Track = &movie->Track[TrackId];
        if (Track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO) {
            memcpy(&TmpTrack, Track, sizeof(AMP_MEDIA_TRACK_INFO_s));
            if (AmpFifo_PeekEntry(Track->Fifo, &Frame, 0) != AMP_OK) {
                FormatLib_Perror(0, 0, "peek failed");
                return AMP_ERROR_FIFO_EMPTY;
            }
            Pts = FormatLib_ConvertPTS(Track, Frame);
            if (Pts == AMP_FORMAT_MAX_TIMESTAMP) {
                FormatLib_Perror(0, 0, "ConvertPTS failed");
                return AMP_ERROR_GENERAL_ERROR;
            }
            Diff = Track->DTS - TmpTrack.DTS;
            if (Diff < MinDiff)
                MinDiff = Diff;
        }
    }
    for (TrackId = 0; TrackId < TrackCount; TrackId++) {
        movie->Track[TrackId].DTS -= MinDiff;
        if (Track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO)
            Track->Info.Video.RefDTS -= MinDiff;
    }
    return AMP_OK;
}

void FormatLib_ResetPTS(AMP_MEDIA_TRACK_INFO_s *track)
{
    AMP_VIDEO_TRACK_INFO_s * const Video = &track->Info.Video;
    K_ASSERT(track != NULL);
    K_ASSERT(track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO);
    Video->ClosedGOP = FALSE;
    FormatLib_InitPTS(track);
}

BOOL FormatLib_IsVideoComplete(AMP_MOVIE_INFO_s *movie)
{
    UINT32 i;
    K_ASSERT(movie != NULL);
    for (i=0; i<movie->TrackCount; i++) {
        if (movie->Track[i].TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO) {
            if (movie->Track[i].DTS < AMP_FORMAT_MAX_TIMESTAMP)
                return FALSE;
        }
    }
    return TRUE;
}

static BOOL FormatLib_CheckMovieEnd(AMP_MOVIE_INFO_s *movie)
{
    AMP_MEDIA_TRACK_INFO_s * const DefTrack = FormatLib_GetDefaultTrack((AMP_MEDIA_INFO_s *)movie, AMP_MEDIA_TRACK_TYPE_VIDEO);
    const UINT64 MinDTS = DefTrack->NextDTS;
    UINT32 i;
    K_ASSERT(DefTrack != NULL);
    for (i=0; i<movie->TrackCount; i++) {
        if (movie->Track[i].NextDTS < MinDTS)
            return FALSE;
    }
    return TRUE;
}

static BOOL FormatLib_CheckSoundEnd(AMP_SOUND_INFO_s *sound)
{
    AMP_MEDIA_TRACK_INFO_s * const DefTrack = FormatLib_GetDefaultTrack((AMP_MEDIA_INFO_s *)sound, AMP_MEDIA_TRACK_TYPE_AUDIO);
    const UINT64 MinDTS = DefTrack->NextDTS;
    UINT32 i;
    K_ASSERT(DefTrack != NULL);
    for (i=0; i<sound->TrackCount; i++) {
        K_ASSERT(sound->Track[i].TrackType == AMP_MEDIA_TRACK_TYPE_AUDIO);
        if (sound->Track[i].NextDTS < MinDTS)
            return FALSE;
    }
    return TRUE;
}

BOOL FormatLib_CheckEnd(AMP_MEDIA_INFO_s *media)
{
    switch (media->MediaType) {
    case AMP_MEDIA_INFO_MOVIE:
        if (FormatLib_CheckMovieEnd((AMP_MOVIE_INFO_s *)media) == FALSE)
            return FALSE;
        break;
    case AMP_MEDIA_INFO_IMAGE:
        break;
    case AMP_MEDIA_INFO_SOUND:
        if (FormatLib_CheckSoundEnd((AMP_SOUND_INFO_s *)media) == FALSE)
            return FALSE;
        break;
    default:
        FormatLib_Perror(0, 0, "incorrect media type");
        break;
    }
    return TRUE;
}

void FormatLib_UpdateMuxTrack(AMP_MEDIA_TRACK_INFO_s *track, UINT32 frameCount)
{
    track->FrameNo += frameCount;
    track->FrameCount += frameCount;
    track->DTS = track->NextDTS = track->DTS + AMP_FORMAT_DURATION(frameCount, track->TimePerFrame);
    if (track->TrackType == AMP_MEDIA_TRACK_TYPE_VIDEO)
        track->Info.Video.FrameCountAfterResume += frameCount;
}

