 /*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: Amba_ImgExif.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Structure type definiton of Exif
 *
 *  @History        ::
 *      Date        Name        Comments
 *      06/10/2013  Eathan Wu Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef __AMBA_IMGEXIF_H__
#define __AMBA_IMGEXIF_H__

/**
 *  Definition of Capture mode
 */
#define IMG_EXIF_STILL                  (0x00)
#define IMG_EXIF_PIV                    (0x01)
#define IMG_EXIF_PB                     (0x02)
#define IMG_EXIF_LASTMODE               (0X02)

/**
 * EXIF private tag, ensure that the data can be referenced until it is processed by the exif muxer
 */
typedef struct _EXIF_PRI_INFO_s
{
    UINT8     *Data;
    UINT32    Size;
} EXIF_PRI_INFO_s;

/**
 * EXIF information structure
 */
typedef struct _EXIF_INFO_s {
    UINT32    ExposureTimeNum;
    UINT32    ExposureTimeDen;
    UINT32    FNumberNum;
    UINT32    FNumberDen;
    UINT32    ExposureProgram;
    UINT32    FocalLengthNum;
    UINT32    FocalLengthDen;
    UINT32    SubjectDistanceNum;
    UINT32    SubjectDistanceDen;
    UINT16    CustomRendered;
    UINT16    ExposureMode;
    UINT16    WhiteBalance;
    UINT16    FocalLength35mmFilm;
    UINT16    SceneCaptureType;
    UINT16    Contrast;
    UINT16    Saturation;
    UINT16    Sharpness;
    UINT16    SubjectDistanceRange;
    INT32     ShutterSpeedNum;
    INT32     ShutterSpeedDen;
    UINT32    ApertureValueNum;
    UINT32    ApertureValueDen;
    INT16     ExposureBiasValueNum;
    INT16     ExposureBiasValueDen;
    UINT32    MaxApertureValueNum;
    UINT32    MaxApertureValueDen;
    UINT32    DigitalZoomRatioNum;
    UINT32    DigitalZoomRatioDen;
    UINT16    MeteringMode;
    UINT16    LightSource;
    UINT8     FlashPixVersion[4];
    UINT16    ColorSpace;
    UINT16    SensingMethod;
    UINT32    ExposureIndexNum;
    UINT32    ExposureIndexDen;
    UINT16    FileSource;
    UINT16    SceneType;
    UINT16    Flash;
    UINT16    IsoSpeedRating;
    UINT8     *PrivateImageData;
    UINT32    PriImgDataSize;
    UINT8     GainControl;
} EXIF_INFO_s;

/**
 * Raw Header information
 */
typedef struct _RAW_HEADER_INFO_s {
    INT32     OpticalBlack[4]; //4*4
    UINT32    LensDistortionRatio[46]; //46*4
    UINT32    IndividualColorGain[8]; //8*4
    UINT32    AutoColorGain[3]; //3*4
    UINT32    CustomcolorGain[3]; //3*4;
    UINT32    ChromaticAberration[62]; //62*4
    UINT32    LensRelativeIllumination[46]; //46*4
} RAW_HEADER_INFO_s;

#endif  /* __AMBA_IMGEXIF_H__ */
