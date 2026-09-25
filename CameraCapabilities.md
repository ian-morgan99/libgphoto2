# Pentax Camera Capabilities for libgphoto2 Integration

This document contains the complete capability matrix extracted from Pentax Image Transmitter 2 (Windows application) for integration into libgphoto2.

## 1. Camera Model Capability Matrix

Extracted from `MtpDevice.cs` Model property setter (lines 60-215). Each model reports:
- `_camModelNo`: Internal capability tier identifier
- `_camVdrExtVer`: Vendor extension version (0 or 1 observed)
- Capability flags (boolean properties)

| Model | USB VID:PID (PTP) | Model No | Vendor Ext | Dual Slot | New Transfer | New Focus Fine | Movie | Movie Settings | Movie Sub-Image | Composition Adjust | Aperture Keep | Color Temp Setting | CI Detail Param | Self-Timer Cont New | Remote Cont New | Slot Change | PC LV High Res |
|-------|-------------------|----------|------------|-----------|--------------|----------------|-------|----------------|-----------------|--------------------|---------------|--------------------|-----------------|---------------------|-----------------|-------------|----------------|
| PENTAX K-3 Mark III | 0x25fb:0x018f* | 78420 | 1 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| PENTAX K-3 | 0x25fb:0x0165 | 77760 | 1 | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| PENTAX 645Z | 0x25fb:0x017b? | 77840 | 1 | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| PENTAX 645D | 0x25fb:0x0130? | 77320 | 0 | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| PENTAX K-1 Mark II | 0x25fb:0x0183 | 78400 | 1 | ✓ | ✓ | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ | ✓ | ✓ | ✓ |
| PENTAX K-1 | 0x25fb:0x0178 | 77970 | 1 | ✓ | ✓ | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ | ✓ | ✓ | ✓ |
| PENTAX KP | 0x25fb:0x017f | 78380 | 1 | ✗ | ✓ | ✓ | ✓ | ✓ | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| PENTAX K-70 | 0x25fb:0x017d | 78370 | 1 | ✗ | ✓ | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| RICOH GR III | 0x25fb:0x210f | 78350 | 1 | ✗ | ✓ | ✓ | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| RICOH G900SE (LS) | 0x25fb:0x78460 | 78460 | 1 | ✗ | ✗ | ✗ | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ |

*Notes:
- K-3 Mark III Monochrome PID is 0x018f (confirmed working in upstream libgphoto2 via PR #1273)
- K-3 Mark III color PID is believed to be 0x018c (found in user's branches, needs validation)
- K-1 II PID is 0x0183 (confirmed in user's branches)
- K-1 PID is 0x0178 (longstanding)
- 645D PID is 0x0130 (same as K-01 legacy MSC mode, needs PTP mode verification)
- LS = Ricoh G900SE family (different sub-protocol)

## 2. Settable Properties (MtpSetDevicePropValue)

These properties can be changed via `MtpSetDevicePropValue(code, value)` and correspond to libgphoto2's standard/gettable PTP device properties or Pentax vendor extensions.

| Property Name | Code (Dec) | Code (Hex) | Type | Notes |
|---------------|------------|------------|------|-------|
| WhiteBalance | 20485 | 0x5005 | byte[] | Standard PTP 0x5005 |
| FNumber | 20487 | 0x5007 | byte[] | Aperture (F-stop) |
| FocalLength | 20488 | 0x5008 | byte[] |  |
| FocusMode | 20490 | 0x500A | byte[] |  |
| ExposureIndex | 20495 | 0x500F | byte[] | ISO setting |
| ExposureBiasCompensation | 20496 | 0x5010 | byte[] | Exposure compensation |
| Contrast | 20500 | 0x5014 | byte[] |  |
| Sharpness | 20501 | 0x5015 | byte[] |  |
| ShutterSpeed | 53263 | 0xD00F | byte[] | Pentax vendor |
| DriveMode | 53267 | 0xD013 | byte[] | Single/continuous/self-timer/remote/bracket modes |
| ExposureBrackettingMode | 53268 | 0xD014 | byte[] |  |
| ExposureBrackettingStep | 53269 | 0xD015 | byte[] |  |
| ColorTemp | 53272 | 0xD018 | byte[] |  |
| WritingFileFormatSetting | 53275 | 0xD01B | byte[] | File format (JPEG/PEF/DNG/TIFF) |
| ExposureIndexEx | 53278 | 0xD01E | byte[] | Extended ISO? |
| ImageTone | 53280 | 0xD020 | byte[] |  |
| Saturation | 53281 | 0xD021 | byte[] |  |
| Hue | 53282 | 0xD022 | byte[] |  |
| Key | 53283 | 0xD023 | byte[] |  |
| FilterEffect | 53284 | 0xD024 | byte[] |  |
| MonotoneToning | 53285 | 0xD025 | byte[] |  |
| MutedBleachBypassToning | 53286 | 0xD026 | byte[] |  |
| FineSharpness | 53287 | 0xD027 | byte[] |  |
| ExContrastHighLightAdjustment | 53288 | 0xD028 | byte[] |  |
| ExContrastShadowAdjustment | 53289 | 0xD029 | byte[] |  |
| CompositionAdjustment | 53290 | 0xD02A | byte[] | On/off toggle (see X/Y/R offsets via 0x9018) |
| FocusAssistMode | 53291 | 0xD02B | byte[] |  |
| CrossProcessType | 53292 | 0xD02C | byte[] |  |
| FilterEffectUserSetting | 53293 | 0xD02D | byte[] |  |
| UsbLiveViewMode | 53301 | 0xD035 | byte[] |  |
| LiveViewZoom | 53302 | 0xD036 | byte[] |  |
| LiveViewMode | 53303 | 0xD037 | byte[] |  |
| MovieMode | 53305 | 0xD039 | byte[] |  |

## 3. GetAllConditions Status Blob (0x900F) Offsets

Extracted from `MtpDevice.cs` where the 0x900F response blob is parsed. These are read-only status indicators.

| Field Name | Offset (Dec) | Offset (Hex) | Type | Description |
|------------|--------------|--------------|------|-------------|
| transCandidateObjectFlag | 12 | 0x0C | uint32 | 1 = transfer candidate object exists |
| transCandidateObjectHandle | 16 | 0x10 | uint32 | Handle of transfer candidate object |
| releaseCanceled | 24 | 0x18 | byte | Bitmask from bits 8-10? |
| isMovieRecording | 24 | 0x18 | bool | (num & 255) == 26 |
| isAstro3PreExp | 24 | 0x18 | bool | (num & 255) == 49 |
| isAstro3MainExp | 24 | 0x18 | bool | (num & 255) == 50 |
| exposureStep | 40 | 0x28 | uint16 |  |
| exposureMode | 184 | 0xB8 | uint32 | Standard exposure mode (P/Tv/Av/M/etc) |
| progMode | 188 | 0xBC | uint32 | LS program mode (for LS cameras) OR fallback exposure mode |
| exposureMode (non-LS) | 184 | 0xB8 | uint32 | Used when \!_isLS |
| progMode (non-LS) | 316 | 0x13C | uint32 | Exposure mode fallback when \!_isLS |
| userMode | 168 | 0xA8 | uint32 |  |
| shiftMode (Pixel Shift) | 320 | 0x140 | bool | Bit 10 (0x400) of uint32 at offset 320 |
| apertureKeep | 320 | 0x140 | bool | Bit 12 (0x1000) of same word |
| apertureKeepInhibit | 320 | 0x140 | bool | Bit 13 (0x2000) of same word |
| isAstro3MovementDetectFailed | 320 | 0x140 | bool | Bit 14 (0x4000) of same word |
| isAstro3ExposureTimeTooLong | 320 | 0x140 | bool | Bit 15 (0x8000) of same word |
| isAstroTracer3 | 528 | 0x210 | bool | Bit 9 (0x200) of uint32 at offset 528 |
| astroTracerTimeLimit | 528 | 0x210 | uint32 | Full uint32 at offset 528 |
| currentWritingSlot | 568 | 0x238 | int32 | Currently active SD card slot (0/1) |
| captureModeInfo | 26 | 0x1A | uint32 | Bitmask: Shooting/ImageProcessing/MovieMode/etc |
| modeEnableInfo | 126 | 0x7E | uint32 | Bitmask: AvChangeable/TvChangeable/etc |
| writingFormatSetting | 131 | 0x83 | uint32 |  |
| compositionAdjFlag | 133 | 0x85 | uint32 |  |
| compositionAdjX | 134 | 0x86 | uint32 |  |
| compositionAdjLimitX | 135 | 0x87 | uint32 |  |
| compositionAdjY | 136 | 0x88 | uint32 |  |
| compositionAdjLimitY | 137 | 0x89 | uint32 |  |
| compositionAdjR | 138 | 0x8A | uint32 |  |
| compositionAdjLimitR | 139 | 0x8B | uint32 |  |
| movieCardSlotSetting | 140 | 0x8C | uint32 |  |
| electroShutterFlag | 141 | 0x8D | uint32 |  |
| sd1CardState | 119 | 0x77 | uint32 |  |
| sd2CardState | 120 | 0x78 | uint32 |  |
| sd1Activity | 121 | 0x79 | uint32 |  |
| sd2Activity | 122 | 0x7A | uint32 |  |
| isoSetting | 27 | 0x1B | uint32 | ISO setting (standard PTP 0x500F) |
| wbMode | 30 | 0x1E | uint32 | White balance mode |
| slotWritingMode | 33 | 0x21 | uint32 |  |
| imageSize | 34 | 0x22 | uint32 |  |
| jpegQuality | 35 | 0x23 | uint32 |  |
| rawKind | 36 | 0x24 | byte | RAW format kind |
| ciMode | 37 | 0x25 | uint32 | Color index mode |
| exposureStep | 42 | 0x2A | uint32 |  |
| sensitivityStep | 43 | 0x2B | uint32 |  |
| exposureMode | 46 | 0x2E | uint32 |  |
| programMode | 47 | 0x2F | uint32 |  |
| afMode | 49 | 0x31 | uint32 | Auto focus mode |
| fixedDriveMode | 54 | 0x36 | uint32 |  |
| extraExpInfo | 55 | 0x37 | uint32 |  |
| rollSensor | 56 | 0x38 | uint32 |  |
| bracketShots | 58 | 0x3A | uint32 |  |
| colorTemperature | 63 | 0x3F | uint32 |  |
| tvCalculatedNum | 68 | 0x44 | uint32 |  |
| tvCalculatedDen | 69 | 0x45 | uint32 |  |
| avCalculatedNum | 70 | 0x46 | uint32 |  |
| avCalculatedDen | 71 | 0x47 | uint32 |  |
| xvCalculatedNum | 72 | 0x48 | uint32 |  |
| xvCalculatedDen | 73 | 0x49 | uint32 |  |
| tvCalculatedMaxNum | 76 | 0x4C | uint32 |  |
| tvCalculatedMaxDen | 77 | 0x4D | uint32 |  |
| svCalculated | 78 | 0x4E | uint32 |  |
| programModeCalculated | 79 | 0x4F | uint32 |  |
| exaeInfoCalculated | 80 | 0x50 | uint32 |  |
| avOpenNum | 82 | 0x52 | uint32 |  |
| avOpenDen | 83 | 0x53 | uint32 |  |
| batteryInfo | 92 | 0x5C | uint32 |  |
| focalLengthNum | 105 | 0x69 | uint32 |  |
| focalLengthDen | 106 | 0x6A | uint32 |  |
| flashState | 109 | 0x6D | uint32 |  |
| flashMode | 110 | 0x6E | uint32 |  |
| intFlashMode | 111 | 0x6F | uint32 |  |
| extFlashMode | 112 | 0x70 | uint32 |  |
| extFlashType | 113 | 0x71 | uint32 |  |
| sd1Remain | 114 | 0x72 | uint32 |  |
| sd2Remain | 115 | 0x73 | uint32 |  |
| temperatureWarning | 118 | 0x76 | uint32 |  |
| sd1CardState | 119 | 0x77 | uint32 |  |
| sd2CardState | 120 | 0x78 | uint32 |  |
| sd1Activity | 121 | 0x79 | uint32 |  |
| sd2Activity | 122 | 0x7A | uint32 |  |
| driveModeNew | 123 | 0x7B | uint32 |  |
| usblvCafInfo | 124 | 0x7C | uint32 |  |
| captureImgInfo | 125 | 0x7D | uint32 |  |
| cropInfo | 128 | 0x80 | uint32 |  |
| aspectRatioInfo | 129 | 0x81 | uint32 |  |
| cardSlotSetting | 130 | 0x82 | uint32 |  |
| writingFormatSetting | 131 | 0x83 | uint32 |  |
| compositionAdjFlag | 133 | 0x85 | uint32 |  |
| compositionAdjX | 134 | 0x86 | uint32 |  |
| compositionAdjLimitX | 135 | 0x87 | uint32 |  |
| compositionAdjY | 136 | 0x88 | uint32 |  |
| compositionAdjLimitY | 137 | 0x89 | uint32 |  |
| compositionAdjR | 138 | 0x8A | uint32 |  |
| compositionAdjLimitR | 139 | 0x8B | uint32 |  |
| movieCardSlotSetting | 140 | 0x8C | uint32 |  |
| electroShutterFlag | 141 | 0x8D | uint32 |  |
| currentWritingSlot | 142 | 0x8E | uint32 |  |

## 4. Opcode Table (MtpOpCode.cs)

All opcodes used by Image Transmitter 2. Standard PTP opcodes (4096-4125) are omitted here as they are standard and already handled by libgphoto2's generic PTP driver.

### Pentax Vendor Opcodes (0x9xxx range)

| Opcode (Dec) | Opcode (Hex) | Name | Data Phase | Notes |
|--------------|--------------|------|------------|-------|
| 36865 | 0x9001 | SetPentaxVendorMode | NO-DATA | Vendor mode enable/disable handshake |
| 36866 | 0x9002 | PentaxCameraShutdown | NO-DATA |  |
| 36867 | 0x9003 | ReceivedPentaxCreatedObject | NO-DATA | Acknowledge created object |
| 36868 | 0x9004 | SetCardWritingMode | NO-DATA |  |
| 36870 | 0x9006 | GetLiveViewFrameData | DATA-TO-READ | Returns live view frame bytes |
| 36871 | 0x9007 | GetPentaxSubImage | DATA-TO-READ | Post-view/thumbnail image |
| 36872 | 0x9008 | GetPentaxMainImage | DATA-TO-READ | Large post-view image |
| 36873 | 0x9009 | SetWarningMessageFlag | NO-DATA |  |
| 36874 | 0x900A | InvokeCameraTask | NO-DATA | **UNUSED** - no call sites found |
| 36875 | 0x900B | GetTransferCandidateFileInfo | DATA-TO-READ | Returns file format/quality/size/name |
| 36876 | 0x900C | GetCamFileOperationCommand | DATA-TO-READ | Returns file operation + info |
| 36877 | 0x900D | GetTranferFileDataBlock | DATA-TO-READ | File download block (returns transferred size) |
| 36878 | 0x900E | DeleteTransferCandidate | DATA-TO-READ |  |
| 36879 | 0x900F | GetAllConditions | DATA-TO-READ | Master status blob (see above) |
| 36881 | 0x9011 | InitiatePentaxCapture | NO-DATA | Triggers capture (release/focus/mwb/sync/apertureReset) |
| 36882 | 0x9012 | TerminatePentaxCapture | NO-DATA |  |
| 36883 | 0x9013 | InterruptFunction | NO-DATA |  |
| 36884 | 0x9014 | SetLightMeteringMode | NO-DATA | **UNUSED** - no call sites found |
| 36886 | 0x9016 | FocusControl | NO-DATA | Coarse/legacy focus drive |
| 36887 | 0x9017 | FocusControlNew | NO-DATA | Fine focus control (gated by _isNewFocusFineControl) |
| 36888 | 0x9018 | SetCompositionAdjustmentOffset | NO-DATA | Sets X/Y/R offset (see CompositionAdjustment property) |
| 36889 | 0x9019 | SetFlagToKeepAperturePosition | NO-DATA | Sets aperture keep position/value |
| 36897 | 0x9021 | MeasuringWhiteBalanceGain | NO-DATA | **UNEXPLORED** |
| 36898 | 0x9022 | ExecuteDustReduction | NO-DATA | Sensor cleaning |
| 38911 | 0x97FF | GeneralCommand | NO-DATA | Escape hatch: CommandNumber as param1, up to 4 additional params |

### Standard PTP Opcodes (used but not Pentax-specific)

These are standard PTP opcodes (4096-4125) used by the application but handled by libgphoto2's generic PTP driver:
- GetDeviceInfo (4097), OpenSession (4098), CloseSession (4099)
- GetStorageIDs (4100), GetStorageInfo (4101), GetNumObjects (4102)
- GetObjectHandles (4103), GetObjectInfo (4104), GetObject (4105)
- GetThumb (4106), DeleteObject (4107), SendObjectInfo (4108)
- SendObject (4109), InitiateCapture (4110), FormatStore (4111)
- ResetDevice (4112), SelfTest (4113), SetObjectProtection (4114)
- PowerDown (4115), GetDevicePropDesc (4116), GetDevicePropValue (4117)
- SetDevicePropValue (4118), ResetDevicePropValue (4119)
- TerminateOpenCapture (4120), MoveObject (4121), CopyObject (4122)
- GetPartialObject (4123), InitiateOpenCapture (4124), Undefined (4125)

## 5. Value Tables (LUT Files)

Extracted from individual `.cs` files for use in libgphoto2's config system.

### WBModeLUT.cs (White Balance Mode)
*(Empty in source - likely uses standard PTP values or another source)*

### CIModeLUT.cs (Color Index Mode)
*(Empty in source)*

### DriveModeLUT.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | Reset | 0x00 |
| 1 | NoReset | 0x01 |

### ApertureResetMode.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | AF Only | 0x00 |
| 1 | Release Only | 0x01 |
| 2 | AF_Release | 0x02 |

### ReleaseBtnMode.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | SyncReleaseComplete | 0x00 |
| 1 | NoSyncAF | 0x01 |
| 2 | NoSyncRelease | 0x02 |
| 3 | NoSyncAll | 0x03 |

### ReleaseSyncMode.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | SLR | 0 |
| 1 | LS_Norm | 1 |
| 2 | LS_Cals | 2 |

### ImgSizeCbxMode.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | JPEG | 0x00 |
| 1 | PEF | 0x01 |
| 2 | DNG | 0x02 |
| 3 | PEF_JPEG | 0x03 |
| 4 | DNG_JPEG | 0x04 |
| 5 | TIFF | 0x05 |

### ShootingFormat.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | Capture | 0x00 |
| 1 | JPG | 0x01 |
| 2 | PEF | 0x02 |
| 3 | DNG | 0x03 |
| 4 | TIFF | 0x04 |
| 5 | Full | 0xFD |
| 6 | Sub | 0xFE |
| 7 | Thumb | 0xFF |

### TransFileFormat.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | Standard | 0x00 |
| 1 | Macro | 0x01 |
| 2 | OneCmMacro | 0x02 |
| 3 | PanFocus | 0x03 |
| 4 | Infinity | 0x04 |
| 5 | Manual | 0x05 |
| 6 | FixedDistance | 0x06 |

### LSFocusModeVal.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | AutoPict | 0x00 |
| 1 | Program | 0x01 |
| 2 | Scene | 0x02 |
| 3 | User1 | 0x03 |
| 4 | User2 | 0x04 |
| 5 | Movie | 0x05 |
| 6 | Cals | 0x06 |

### LSModeDial.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | right | 0x00 |
| 1 | left | 0x01 |
| 2 | up | 0x02 |
| 3 | down | 0x03 |
| 4 | clockwise | 0x04 |
| 5 | counterclockwise | 0x05 |
| 6 | reset | 0x06 |

### CmpAdjDir.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | MovieRecording | 0x1A |
| 1 | AstroTracerType3PreExposing | 0x31 |
| 2 | AstroTracerType3MainExposing | 0x32 |

### DSPState2.cs
| Value | Name | Hex |
|-------|------|-----|
| 0 | TvOutOfLinkedRange | 0x02 |
| 1 | AvOutOfLinkedRange | 0x04 |
| 2 | AELock | 0x08 |
| 3 | FlashRequest | 0x10 |
| 4 | ApertureOpened | 0x80 |
| 5 | TvNotChangeable | 0x100 |
| 6 | AvNotChangeable | 0x200 |
| 7 | ProgramShift | 0x400 |
| 8 | SvOutOfLinkedRange | 0x800 |
| 9 | ApertureOperationFixed | 0x1000 |
| 10 | ApertureOperationFixedDisabled | 0x2000 |
| 11 | AT3MovementDetectionFailed | 0x4000 |
| 12 | AT3ExposureTimeTooLong | 0x8000 |

### ExtraCulculatedAEInfo.cs

## 6. Exposure Mode Tables

### ExpMode.cs (Exposure Mode Dial / Program Mode)
| Value | Name | Hex |
|-------|------|-----|
| 0 | P | 0x00 |
| 1 | AUTOPICT | 0x01 |
| 2 | HyP | 0x02 |
| 3 | Green | 0x03 |
| 4 | Tv | 0x04 |
| 5 | Av | 0x05 |
| 6 | HyP | 0x06 |
| 7 | HypAv | 0x07 |
| 8 | M | 0x08 |
| 9 | B | 0x09 |
| 10 | Av_Lens | 0x0A |
| 11 | M_Lens | 0x0B |
| 12 | TAv | 0x0C |
| 13 | TAv_Lens | 0x0D |
| 14 | Sv | 0x0E |
| 15 | X | 0x0F |
| 16 | X_Lens | 0x10 |
| 17 | LS | 0x11 |
| 18 | APL_P | 0x12 |
| 19 | SA_Auto | 0x13 |
| 20 | AstroTracer | 0x14 |
| 21 | SA_Auto | 0x15 |
| 22 | AdvHypP | 0x16 |
| 23 | AdvHypTv | 0x17 |
| 24 | AdvHypAv | 0x18 |
| 25 | AdvHypM | 0x19 |
| 26 | AdvHypTAv | 0x1A |
| 27 | AdvHypAv_Lens | 0x1B |
| 28 | AdvHypM_Lens | 0x1C |
| 29 | AdvHypTAv_Lens | 0x1D |
| 30 | AdvHypSv | 0x1E |
| 31 | HypSv | 0x1F |

### NewDriveMode.cs (Drive Mode - 52 values 0-51)
| Value | Name | Hex |
|-------|------|-----|
| 0 | SingleShot | 0x00 |
| 1 | ExpBracket | 0x01 |
| 2 | SelfTimerExpBracket | 0x02 |
| 3 | RemoteControlBracket | 0x03 |
| 4 | Continuous_High | 0x04 |
| 5 | Continuous_Middle | 0x05 |
| 6 | Continuous_Low | 0x06 |
| 7 | SelfTimer_12sec | 0x07 |
| 8 | SelfTimer_2sec | 0x08 |
| 9 | RemoteControl | 0x09 |
| 10 | RemoteControl_3sec | 0x0A |
| 11 | RemoteControlContinuous | 0x0B |
| 12 | MirrorUp | 0x0C |
| 13 | RemoteControlMirrorUp | 0x0D |
| 14 | MultiExposure | 0x0E |
| 15 | MultiExposureContinuous | 0x0F |
| 16 | SelfTimerMultiExposure | 0x10 |
| 17 | RemoteControlMultiExp | 0x11 |
| 18 | Interval | 0x12 |
| 19 | IntervalMultiExposure | 0x13 |
| 20 | IntervalMovie | 0x14 |
| 21 | StartStream | 0x15 |
| 22 | SelfTimerContinuous | 0x16 |
| 23 | DepthOfFieldBracket | 0x17 |
| 24 | DepthOfFieldBracket_Self | 0x18 |
| 25 | DepthOfFieldBracket_Self2 | 0x19 |
| 26 | MotionBracket | 0x1A |
| 27 | MotionBracket_Self | 0x1B |
| 28 | MotionBracket_Self2 | 0x1C |
| 29 | SelfTimer2secBracket | 0x1D |
| 30 | RemoteControl3secBracket | 0x1E |
| 31 | RemoteControl3secCont_Hi | 0x1F |
| 32 | RemoteControl3secCont_Mi | 0x20 |
| 33 | RemoteControl3secCont_Lo | 0x21 |
| 34 | RemoteControlCont_Hi | 0x22 |
| 35 | RemoteControlCont_Mi | 0x23 |
| 36 | RemoteControlCont_Lo | 0x24 |
| 37 | DepthOfFieldBktRemote3sec | 0x25 |
| 38 | DepthOfFieldBktRemote = 0x26 |
| 39 | MotionBracketRemote3sec = 0x27 |
| 40 | MotionBracketRemote = 0x28 |
| 41 | MirrorUpSelfTimer = 0x29 |
| 42 | MirrorUpSelfTimer2sec = 0x2A |
| 43 | MirrorUpRemoteControl3sec = 0x2B |
| 44 | SelfTimer2secMultiExp = 0x2C |
| 45 | RemoteControl3secMultiExp = 0x2D |
| 46 | SelfTimer2sContinuous_Hi = 0x2E |
| 47 | SelfTimer2sContinuous_Mi = 0x2F |
| 48 | SelfTimer2sContinuous_Lo = 0x30 |
| 49 | RemoteControl3secCont_Hi = 0x31 |
| 50 | RemoteControl3secCont_Mi = 0x32 |
| 51 | RemoteControl3secCont_Lo = 0x33 |

### CaptureModeInfo.cs (Status bitmask)
| Value | Name | Hex |
|-------|------|-----|
| 1 | Shooting | 0x00000001 |
| 2 | ImageProcessing | 0x00000002 |
| 256 | MovieMode | 0x00000100 |
| 512 | MovieRecording | 0x00000200 |
| 1024 | MirroUpMode | 0x00000400 |
| 2048 | MirrorUpping | 0x00000800 |
| 4096 | IntervalMode | 0x00001000 |
| 16384 | MultiCaptureMode | 0x00004000 |
| 32768 | MultiCapturing | 0x00008000 |
| 1048576 | SelfTimerCountdown | 0x00100000 |

### ModeEnableInfo.cs (Status bitmask - what's changeable)
| Value | Name | Hex |
|-------|------|-----|
| 1 | AvChangeable | 0x00000001 |
| 2 | TvChangeable | 0x00000002 |
| 4 | SvChangeable | 0x00000004 |
| 8 | XvChangeable | 0x00000008 |
| 16 | SvAutoSelectable | 0x00000010 |
| 32 | TaskChanging | 0x00000020 |
| 64 | AstroTraceTimer | 0x00000040 |
| 64 | BulbTimer | 0x00000040 (shared bit) |
| 384 | GPSState | 0x00000180 |
| 512 | AstroTracer3 | 0x00000200 |

### ConditionIndex.cs (Named indices into various blobs)
| Name | Value (Dec) | Value (Hex) | Notes |
|------|-------------|-------------|-------|
| DSP_STATE | 3 | 0x00000003 |  |
| CPU_STATE | 4 | 0x00000004 |  |
| DSP_STATE_2 | 6 | 0x00000006 |  |
| DSP_TEMPERATURE | 7 | 0x00000007 |  |
| TRANS_CD_OBJ_FLAG | 8 | 0x00000008 | Transfer candidate object flag |
| TRANS_CD_OBJ_HNDL | 9 | 0x00000009 | Transfer candidate object handle |
| USER_MODE | 10 | 0x0000000A |  |
| EXP_BRACKET_MODE | 20 | 0x00000014 |  |
| EXP_BRACKET_STEP_NUM | 21 | 0x00000015 |  |
| EXP_BRACKET_STEP_DEN | 22 | 0x00000016 |  |
| EXP_BRACKET_FRAMES | 23 | 0x00000017 |  |
| CAPTURE_MODE_INFO | 26 | 0x0000001A |  |
| ISO_SETTING | 27 | 0x0000001B |  |
| WB_MODE | 30 | 0x0000001E |  |
| SLOT_WRITING_MODE | 33 | 0x00000021 |  |
| IMAGE_SIZE | 34 | 0x00000022 |  |
| JPEG_QUALITY | 35 | 0x00000023 |  |
| RAW_KIND | 36 | 0x00000024 |  |
| CI_MODE | 37 | 0x00000025 |  |
| EXPOSURE_STEP | 42 | 0x0000002A |  |
| SENSITIVITY_STEP | 43 | 0x0000002B |  |
| EXPOSURE_MODE | 46 | 0x0000002E |  |
| PROGRAM_MODE | 47 | 0x0000002F |  |
| AF_MODE | 49 | 0x00000031 |  |
| FIXED_DRIVE_MODE | 54 | 0x00000036 |  |
| EXTRA_EXP_INFO | 55 | 0x00000037 |  |
| ROLL_SENSOR | 56 | 0x00000038 |  |
| BRACKET_SHOTS | 58 | 0x0000003A |  |
| COLOR_TEMPERATURE | 63 | 0x0000003F |  |
| TV_CULCULATED_NUM | 68 | 0x00000044 |  |
| TV_CULCULATED_DEN | 69 | 0x00000045 |  |
| AV_CULCULATED_NUM | 70 | 0x00000046 |  |
| AV_CULCULATED_DEN | 71 | 0x00000047 |  |
| XV_CULCULATED_NUM | 72 | 0x00000048 |  |
| XV_CULCULATED_DEN | 73 | 0x00000049 |  |
| TV_CULCULATED_MAX_NUM | 76 | 0x0000004C |  |
| TV_CULCULATED_MAX_DEN | 77 | 0x0000004D |  |
| SV_CULCULATED | 78 | 0x0000004E |  |
| PROGRAM_MODE_CULCULATED | 79 | 0x0000004F |  |
| EXAEINFO_CULCULATED | 80 | 0x00000050 |  |
| AV_OPEN_NUM | 82 | 0x00000052 |  |
| AV_OPEN_DEN | 83 | 0x00000053 |  |
| BATTERY_INFO | 92 | 0x0000005C |  |
| FOCAL_LENGTH_NUM | 105 | 0x00000069 |  |
| FOCAL_LENGTH_DEN | 106 | 0x0000006A |  |
| FLASH_STATE | 109 | 0x0000005D |  |
| FLASH_MODE | 110 | 0x0000005E |  |
| INT_FLASH_MODE | 111 | 0x0000005F |  |
| EXT_FLASH_MODE | 112 | 0x00000060 |  |
| EXT_FLASH_TYPE | 113 | 0x00000061 |  |
| SD1_REMAIN | 114 | 0x00000072 |  |
| SD2_REMAIN | 115 | 0x00000073 |  |
| TEMPERATURE_WARNING | 118 | 0x00000076 |  |
| SD1_CARD_STATE | 119 | 0x00000077 |  |
| SD2_CARD_STATE | 120 | 0x00000078 |  |
| SD1_ACTIVITY | 121 | 0x00000079 |  |
| SD2_ACTIVITY | 122 | 0x0000007A |  |
| DRIVE_MODE_NEW | 123 | 0x0000007B |  |
| USBLV_CAF_INFO | 124 | 0x0000007C |  |
| CAPTURE_IMG_INFO | 125 | 0x0000007D |  |
| MODE_ENABLE_INFO | 126 | 0x0000007E |  |
| CROP_INFO | 128 | 0x00000080 |  |
| ASPECT_RATIO_INFO | 129 | 0x00000081 |  |
| CARD_SLOT_SETTING | 130 | 0x00000082 |  |
| WRITING_FORMAT_SETTING | 131 | 0x00000083 |  |
| COMPOSITION_ADJ_FLAG | 133 | 0x00000085 |  |
| COMPOSITION_ADJ_X | 134 | 0x00000086 |  |
| COMPOSITION_ADJ_LMT_X | 135 | 0x00000087 |  |
| COMPOSITION_ADJ_Y | 136 | 0x00000088 |  |
| COMPOSITION_ADJ_LMT_Y | 137 | 0x00000089 |  |
| COMPOSITION_ADJ_R | 138 | 0x0000008A |  |
| COMPOSITION_ADJ_LMT_R | 139 | 0x0000008B |  |
| MOVIE_CARD_SLOT_SETTING | 140 | 0x0000008C |  |
| ELECTRO_SHUTTER_FLAG | 141 | 0x0000008D |  |
| CURRENT_WRITING_SLOT | 142 | 0x0000008E |  |

## 7. Recommended Integration Approach for libgphoto2

### Overall Strategy
Do **not** revive the `pentaxmodern` camlib as a separate module. Instead, extend the existing generic PTP driver (`ptp2/`) with Pentax vendor-specific extensions, following the same pattern as Canon/Nikon/Sony vendor code already present in `ptp.c` and `config.c`.

### Specific Implementation Plan

#### A. Property Support (High Priority)
Add the Pentax vendor property codes (0xD0xx range) to `ptp2/config.c`:
- Map each property code to its libgphoto2 equivalent (e.g. `GP_CAPTURE_MODE`, `GP_IMAGE_FORMAT`, etc.)
- Use the existing `ptp_decode_uintXX`/`ptp_encode_uintXX` helpers for scalar values
- For arrays/structs (like white balance, color matrices), use appropriate decoding
- The get/set pattern is already established in `config.c` for standard PTP props

#### B. Status Blob Parsing (Medium Priority)
Enhance the PTP driver's status handling to parse the `GetAllConditions` (0x900F) blob:
- Add a custom status parser for Pentax that extracts:
  - Battery level (if not already covered by standard PTP)
  - ISO, WB, exposure mode, focus mode, drive mode
  - Astrotracer phase/state/timer limit
  - Pixel shift status (read-only indicator)
  - Slot state, remaining space
  - Capture mode info (shooting/processing/movie/etc)
  - Mode enable info (what's currently changeable)
  - Etc. as needed for UI/display purposes

#### C. Opcode Implementation (Variable Priority)
Implement only the opcodes that have clear call sites and documented usage:
- **High Confidence (implement first):**
  - 0x9001 SetPentaxVendorMode (handshake - required before other vendor ops)
  - 0x9006 GetLiveViewFrameData (live view)
  - 0x9007 GetPentaxSubImage (post-view)
  - 0x9008 GetPentaxMainImage (large post-view)
  - 0x900B GetTransferCandidateFileInfo (file info for download)
  - 0x900C GetCamFileOperationCommand (file operation info)
  - 0x900D GetTranferFileDataBlock (file download chunks)
  - 0x900F GetAllConditions (master status)
  - 0x9011 InitiatePentaxCapture (trigger capture)
  - 0x9012 TerminatePentaxCapture (cleanup)
  - 0x9013 InterruptFunction (cancel operation)
  - 0x9016 FocusControl (coarse focus)
  - 0x9017 FocusControlNew (fine focus)
  - 0x9018 SetCompositionAdjustmentOffset (composition adjustment)
  - 0x9019 SetFlagToKeepAperturePosition (aperture keep)
- **Low Confidence (investigate further):**
  - 0x900A InvokeCameraTask (no call sites)
  - 0x9014 SetLightMeteringMode (no call sites)
  - 0x9021 MeasuringWhiteBalanceGain (unexplored)
  - 0x9022 ExecuteDustReduction (sensor cleaning)
  - 0x97FF GeneralCommand (escape hatch - only sub-command 1 seen for PreviewSw)

#### D. Model-Specific Handling
Use the `_camModelNo` approach from Image Transmitter 2:
1. Read standard PTP `GetDeviceInfo` to get the camera's `Model` string
2. Map the model string to a `_camModelNo` and capability flags (exactly as in `MtpDevice.cs`)
3. Use this to gate available features (e.g. only allow movie-related ops if `_isMovieSupported`)
4. This replaces hardcoding per-model tables in libgphoto2 - the camera reports its own capabilities

### 8. Open Questions / Validation Required

Before implementing, validate these against real hardware:
1. **Pixel Shift Resolution**: Confirmed as read-only status bit (offset 320, bit 1024) - no setter exists. Can be exposed as a read-only indicator but not controlled.
2. **HDR Mode**: Confirmed as LS-series-only program mode value - not a general Pentax K-series control. Only relevant for LS (Ricoh G900SE) family.
3. **Actual DriveMode Values**: The numeric values for "regular" vs "handheld" pixel shift modes are **camera-reported** via `GetDevicePropDesc(0xD013)` - not hardcodable. Must be discovered at runtime.
4. **InvokeCameraTask / GeneralCommand**: Need to determine if these are actually used or dead code in this build.
5. **USB PIDs for PTP Mode**: Confirm PTP-mode IDs for K-01 (none found), K-1 II (0x0183), K-3 III (0x018c believed).

### 9. Next Steps

1. **Short Term**: Implement the high-confidence opcodes and property mappings as a proof-of-concept against real K-3 III/K-1 II hardware.
2. **Medium Term**: Add model-specific capability gating using the `_camModelNo` approach.
3. **Long Term**: Add full status blob parsing for rich UI feedback in gphoto2 CLI/GUIs.
4. **Validation**: Test each feature against real hardware using `gphoto2 --summary`, `--get-config`, `--set-config`, `--trigger-capture`, `--wait-event`, and file download.

---
*Document generated from analysis of Pentax Image Transmitter 2 decompiled source (C#) and cross-referenced with libgphoto2 source structure.*
